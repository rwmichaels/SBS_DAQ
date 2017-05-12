/*****************************************************************
 * 
 * linuxsfi_list.c - "Primary" Readout list routines for using
 *                   a linux controller with an SFI
 *
 * Usage:
 *
 *    #include "linuxsfi_list.c"
 *
 *  then define the following routines:
 * 
 *    void rocDownload(); 
 *    void rocPrestart(); 
 *    void rocGo(); 
 *    void rocEnd(); 
 *    void rocTrigger();
 *
 * SVN: $Rev: 420 $
 *
 */

/* SFI address */
#define SFI_ADDR 0xe00000

/* A24 Address for TI Module (e.g. Slot number = 2) */
#define TRIG_ADDR 0x080000
/* TRIG_MODE=0 for interrupt mode (External input) */
# define TRIG_MODE 1
# define READOUT_TI 0
# define BUFFERLEVEL 1

#define ROL_NAME__ "SFIGEN"
#ifndef MAX_EVENT_LENGTH
/* Check if an older definition is used */
#ifdef MAX_SIZE_EVENTS
#define MAX_EVENT_LENGTH MAX_SIZE_EVENTS
#else
#define MAX_EVENT_LENGTH 4096
#endif /* MAX_SIZE_EVENTS */
#endif /* MAX_EVENT_LENGTH */
#ifndef MAX_EVENT_POOL
/* Check if an older definition is used */
#ifdef MAX_NUM_EVENTS
#define MAX_EVENT_POOL MAX_NUM_EVENTS
#else
#define MAX_EVENT_POOL   400
#endif /* MAX_NUM_EVENTS */
#endif /* MAX_EVENT_POOL */
/* POLLING_MODE */
#define POLLING___
#define POLLING_MODE
/* INIT_NAME should be defined by roc_### (maybe at compilation time - check Makefile-rol) */
#ifndef INIT_NAME
/*#warn "INIT_NAME undefined. Setting to linuxsfi_list__init"*/
#define INIT_NAME linuxsfi_list__init
#endif
#include <stdio.h>
#include <rol.h>
#include "jvme.h"
#include "tiLib.h"
#include <GEN_source.h>
#include <LINUXSFI_source.h>
#include "libsfifb.h"
extern int bigendian_out;

extern DMANODE *the_event;
extern unsigned int *dma_dabufp;

extern unsigned int sfiDoAck;
extern unsigned int sfiNeedAck;

/* NOTE: These are not defined... might not use them */
/* #define ISR_INTLOCK INTLOCK */
/* #define ISR_INTUNLOCK INTUNLOCK */

/* ROC Function prototypes defined by the user */
void rocDownload();
void rocPrestart();
void rocGo();
void rocEnd();
void rocTrigger();
int  getOutQueueCount();

/* Asynchronous (to linuxsfi rol) trigger routine, connects to rocTrigger */
void asyncTrigger();

/* Input and Output Partitions for VME Readout */
DMA_MEM_ID vmeIN, vmeOUT;


static void __download()
{
  int res,status;
  unsigned long laddr;

  daLogMsg("INFO","SBS Readout list compiled %s", DAYTIME);
#ifdef POLLING___
  rol->poll = 1;
#endif
  *(rol->async_roc) = 0; /* Normal ROC */

  bigendian_out=1;
 
  /* Open the default VME windows */
  vmeOpenDefaultWindows();

 /* Open Slave Window for SFI Initiated Block transfers */
  vmeOpenSlaveA32(0x18000000,0x00400000);
  dmaPUseSlaveWindow(1);

  /* Initialize memory partition library */
  dmaPartInit();

  /* Setup Buffer memory to store events */
  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",MAX_EVENT_LENGTH,MAX_EVENT_POOL,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);

  /* Reinitialize the Buffer memory */
  dmaPReInitAll();
  dmaPStatsAll();

  /* Initialize the SFI and Fastbus */

   res = vmeBusToLocalAdrs(0x39,(char *)SFI_ADDR,(char **)&laddr);
   if (res != 0) {
      printf("SFI: ERROR: vmeBusToLocalAdrs  0x%x 0x%x\n",SFI_ADDR,laddr);
   } else {
      InitSFI(laddr);
   }

   SFI_ShowStatusReg(); 
   fb_init_1(0);


  /* TI Setup */
  tiInit(TRIG_ADDR,TRIG_MODE,0);

 /* measured longest fiber length */
  tiSetFiberLatencyOffset_preInit(0x40); 
 /* Set crate ID */
  tiSetCrateID_preInit(0x4); /* ROC 4 */

  tiDisableBusError();


  if(READOUT_TI==0) /* Disable data readout */
    {
      tiDisableDataReadout();

      /* Disable A32... where that data would have been stored on the TI */
      tiDisableA32();
    }

  tiSetEventFormat(1);
  
  tiSetBlockBufferLevel(BUFFERLEVEL);

  /* Override default BUSY level - Use if TI is not in a VXS crate*/
  tiSetBusySource(0,1);
  tiSetTriggerSource(TI_TRIGGER_PART_1);
  tiStatus(0);
  tiDisableVXSSignals();
  tiSetEventFormat(2);

  /* Execute User defined download */
  rocDownload();
 
  daLogMsg("INFO","SBS DAQ: Download Executed");


} /*end download */     

static void __prestart()
{
  CTRIGINIT;
  *(rol->nevents) = 0;
  unsigned long jj, adc_id;
  daLogMsg("INFO","SBS: Entering Prestart");

  GEN_INIT;
  CTRIGRSS(GEN,1,usrtrig,usrtrig_done);
  CRTTYPE(1,GEN,1);

  tiEnableVXSSignals();

  /* Execute User defined prestart */
  rocPrestart();

  /* Connect User Trigger Routine */
  tiIntConnect(TI_INT_VEC,asyncTrigger,0);


  daLogMsg("INFO","SBS: Prestart Executed");

  if (__the_event__) WRITE_EVENT_;
  *(rol->nevents) = 0;
  rol->recNb = 0;

} /*end prestart */     

static void __end()
{
  int ii, ievt, rem_count;
  int len, type, lock_key;
  DMANODE *outEvent;
  int oldnumber;

  /* Disable Interrupts */
  sfiTrigDisable();
  sfiTrigIntDisconnect();

  /* Execute User defined end */
  rocEnd();

  CDODISABLE(GEN,1,0);
 
  /* we need to make sure all events taken by the
     VME are collected from the vmeOUT queue */

  rem_count = getOutQueueCount();
  if (rem_count > 0) 
    {
      printf("linuxsfi_list End: %d events left on vmeOUT queue (will now de-queue them)\n",rem_count);
      /* This wont work without a secondary readout list (will crash EB or hang the ROC) */
      for(ii=0;ii<rem_count;ii++) 
	{
	  __poll();
	}
    }
  else
    {
      printf("linuxsfi_list End: vmeOUT queue Empty\n");
    }
      
  daLogMsg("INFO","SBS: End Executed");

  if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  CDODISABLE(GEN,1,0);
  daLogMsg("INFO","SBS: Pause Executed");
  
  if (__the_event__) WRITE_EVENT_;
} /*end pause */

static void __go()
{
  daLogMsg("INFO","SBS: Entering Go");
  
  CDOENABLE(GEN,1,0); 
  rocGo();
  
  /* Enable Interrupts here.. */
  /* maybe dont want ? sfiTrigEnable(1); */
  
  if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
  long EVENT_LENGTH;
  int ii, len, data, type, lock_key;
  DMANODE *outEvent;
  static int evcnt=0;
  static int verbose=0;

  evcnt++;

  if (verbose) logMsg("Into usrtrig %d %d \n",EVTYPE,evcnt,0,0,0,0);
 
/*   unsigned int syncFlag, lateFail; */

/*   ISR_INTLOCK; */ 
  outEvent = dmaPGetItem(vmeOUT);

  /* outEvent is always NULL, so why do we need it ? */

#ifdef WHOKNOWS
  if(outEvent != NULL) 
    {
      len = outEvent->length;
      data = outEvent->type;
      /* FIXME: Decode trigger data here... if necessary */
      sfiTrigDecodeData(data, &type, &syncFlag, &lateFail);

      logMsg("usrtrig:  Got event %d %d %d %d \n",EVTYPE, evcnt, len, data,0,0);

      CEOPEN(type, BT_UI4);
      
      if(rol->dabufp != NULL) 
	{
	  for(ii=0;ii<len;ii++) 
	    {
	      *rol->dabufp++ = LSWAP(outEvent->data[ii]);
	    }
	} 
      else 
	{
	  printf("linuxsfi_list: ERROR rol->dabufp is NULL -- Event lost\n");
	}

      /* Free up the trigger after freeing up the buffer - do it
	 this way to prevent triggers from coming in after freeing
	 the buffer and before the Ack */
      if(sfiNeedAck > 0)
	{
	  outEvent->part->free_cmd = *sfiTrigAck;
	}
      else
	{
	  outEvent->part->free_cmd = NULL;
	}
      
      CECLOSE;
      dmaPFreeItem(outEvent); /* IntAck performed in here, if NeedAck */
    }
  else
    {
      logMsg("Error: no Event in vmeOUT queue\n",0,0,0,0,0,0);
    }
#endif
  
  
  
/*   ISR_INTUNLOCK; */
 
} /*end trigger */

void asyncTrigger()
{
  int intCount=0;
  int length,size;

  intCount = sfiTrigGetIntCount();

  /* grap a buffer from the queue */
  GETEVENT(vmeIN,intCount);
  if(the_event->length!=0) 
    {
      printf("Interrupt Count = %d\t",intCount);
      printf("the_event->length = %d\n",the_event->length);
    }

  /* Execute user defined Trigger Routine */
  rocTrigger();

  /* Put this event's buffer into the OUT queue. */
  PUTEVENT(vmeOUT);
  /* Check if the event length is larger than expected */
  length = (((int)(dma_dabufp) - (int)(&the_event->length))) - 4;
  size = the_event->part->size - sizeof(DMANODE);

  if(length>size) 
    {
      printf("rocLib: ERROR: Event length > Buffer size (%d > %d).  Event %d\n",
	     length,size,the_event->nevent);
    }
  if(dmaPEmpty(vmeIN)) 
    {
      printf("WARN: vmeIN out of event buffers.\n");
      sfiDoAck = 0;
      sfiNeedAck = 1;
    }

}

void usrtrig_done()
{
  static int evcnt=0;
  static int verbose=0;

  evcnt++;

  if(verbose) logMsg("... usrtrig_done %d \n",evcnt,0,0,0,0,0);

} /*end done */

void __done()
{
  poolEmpty = 0; /* global Done, Buffers have been freed */
} /*end done */

static void __status()
{
} /* end status */

int
getOutQueueCount()
{
  if(vmeOUT) 
    return(vmeOUT->list.c);
  else
    return(0);
}

/* This routine is automatically executed just before the shared libary
   is unloaded.

   Clean up memory that was allocated 
*/
__attribute__((destructor)) void end (void)
{
  printf("ROC Cleanup\n");
  dmaPFreeAll();

  vmeCloseA32Slave();
  vmeCloseDefaultWindows();
}
