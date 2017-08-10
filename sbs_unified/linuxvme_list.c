/*****************************************************************
 * 
 * linuxvme_list.c - "Primary" Readout list routines for linuxvme
 *
 * Usage:
 *
 *    #include "linuxvme_list.c"
 *
 *  then define the following routines:
 * 
 *    void rocDownload(); 
 *    void rocPrestart(); 
 *    void rocGo(); 
 *    void rocEnd(); 
 *    void rocTrigger();
 *
 * SVN: $Rev: 474 $
 *
 */


#define ROL_NAME__ "LINUXVME"
#ifndef MAX_EVENT_LENGTH
/* Check if an older definition is used */
#ifdef MAX_SIZE_EVENTS
#define MAX_EVENT_LENGTH MAX_SIZE_EVENTS
#else
#define MAX_EVENT_LENGTH 10240
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
#if (MAX_EVENT_LENGTH>40*1024)
#error "Event Length too large.  Reduce the size of MAX_EVENT_LENGTH to less than 40kB."
#endif

/* POLLING_MODE */
#define POLLING___
#define POLLING_MODE
/* INIT_NAME should be defined by roc_### (maybe at compilation time - check Makefile-rol) */
#ifndef INIT_NAME
#warning "INIT_NAME undefined. Setting to linuxvme_list__init"
#define INIT_NAME linuxvme_list__init
#endif
#include <stdio.h>
#include <rol.h>
#include "jvme.h"
#include <LINUXVME_source.h>
#include "tirLib.h"
#include "tsUtil.h"
#include "dmaBankTools.h"
extern int bigendian_out;

extern DMANODE *the_event;
extern unsigned int *dma_dabufp;

extern unsigned int tirDoAck;
extern unsigned int tirNeedAck;
extern unsigned int tsDoAck;
extern unsigned int tsNeedAck;

/* At the moment, they point to the same mutex.. should probably consolidate */
#ifdef TIR_SOURCE
#define ISR_INTLOCK INTLOCK
#define ISR_INTUNLOCK INTUNLOCK
#elif defined(TS_SOURCE)
#define ISR_INTLOCK TSINTLOCK
#define ISR_INTUNLOCK TSINTUNLOCK
#endif

/* Mutex and condition variables to properly handle Readout Acknowledge */
pthread_mutex_t ack_mutex=PTHREAD_MUTEX_INITIALIZER;
#define ACKLOCK {				\
    if(pthread_mutex_lock(&ack_mutex)<0)	\
      perror("pthread_mutex_lock");		\
}
#define ACKUNLOCK {				\
    if(pthread_mutex_unlock(&ack_mutex)<0)	\
      perror("pthread_mutex_unlock");		\
}
pthread_cond_t ack_cv = PTHREAD_COND_INITIALIZER;
#define ACKWAIT {					\
    if(pthread_cond_wait(&ack_cv, &ack_mutex)<0)	\
      perror("pthread_cond_wait");			\
}
#define ACKSIGNAL {					\
    if(pthread_cond_signal(&ack_cv)<0)			\
      perror("pthread_cond_signal");			\
  }
int ack_runend=0;

/* ROC Function prototypes defined by the user */
void rocDownload();
void rocPrestart();
void rocGo();
void rocEnd();
void rocTrigger();
void rocCleanup();
int  getOutQueueCount();

/* Asynchronous (to linuxvme rol) trigger routine, connects to rocTrigger */
void asyncTrigger();

/* Input and Output Partitions for VME Readout */
DMA_MEM_ID vmeIN, vmeOUT;


static void __download()
{
  int status;

  daLogMsg("INFO","Readout list compiled %s", DAYTIME);
#ifdef POLLING___
  rol->poll = 1;
#endif
  *(rol->async_roc) = 0; /* Normal ROC */

  bigendian_out=1;
 
  /* Open the default VME windows */
  vmeOpenDefaultWindows();

  /* Initialize memory partition library */
  dmaPartInit();

  /* Setup Buffer memory to store events */
  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",MAX_EVENT_LENGTH,MAX_EVENT_POOL,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);

  /* Reinitialize the Buffer memory */
  dmaPReInitAll();

  /* Initialize VME Interrupt interface - use defaults */
#ifdef TIR_SOURCE
  tirIntInit(TIR_ADDR,TIR_MODE,1);
#endif
#ifdef TS_SOURCE
  tsInit(TS_ADDR,0);
#endif

  /* Execute User defined download */
  rocDownload();
 
  daLogMsg("INFO","Download Executed");


} /*end download */     

static void __prestart()
{
  ACKLOCK;
  ack_runend=0;
  ACKUNLOCK;
  CTRIGINIT;
  *(rol->nevents) = 0;
  unsigned long jj, adc_id;
  daLogMsg("INFO","Entering Prestart");

  LINUXVME_INIT;
  CTRIGRSS(LINUXVME,1,usrtrig,usrtrig_done);
  CRTTYPE(1,LINUXVME,1);

  /* Check the health of the VME Mutex and repair it if needed */
  vmeCheckMutexHealth(10);

  /* Execute User defined prestart */
  rocPrestart();

  /* Initialize VME Interrupt variables */
#ifdef TIR_SOURCE
  tirClearIntCount();
#endif

  /* Connect User Trigger Routine */
#ifdef TIR_SOURCE
  tirIntConnect(TIR_INT_VEC,asyncTrigger,0);
#elif defined(TS_SOURCE)
  tsIntConnect(0,asyncTrigger,0,TS_MODE);
#endif

  daLogMsg("INFO","Prestart Executed");

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

  /* If the asyncTrigger is waiting for a signal, give it one... but be sure
     no more triggers are accepted */
  ACKLOCK;
#ifdef TIR_SOURCE
  tirNeedAck=1;
#elif defined(TS_SOURCE)
  tsNeedAck=1;
#endif
  ACKSIGNAL;
  ACKUNLOCK;

  /* Disable Interrupts */
#ifdef TIR_SOURCE
  tirIntDisable();
  tirIntDisconnect();
  tirIntStatus(1);
#elif defined(TS_SOURCE)
  tsStop(1);
  tsIntDisable();
  tsIntDisconnect();
#endif

  /* Execute User defined end */
  rocEnd();

  CDODISABLE(LINUXVME,1,0);
 
  /* we need to make sure all events taken by the
     VME are collected from the vmeOUT queue */

  rem_count = getOutQueueCount();
  if (rem_count > 0) 
    {
      printf("linuxvme_list End: %d events left on vmeOUT queue (will now de-queue them)\n",rem_count);
      /* This wont work without a secondary readout list (will crash EB or hang the ROC) */
      for(ii=0;ii<rem_count;ii++) 
	{
	  __poll();
	}
    }
  else
    {
      printf("linuxvme_list End: vmeOUT queue Empty\n");
    }
      
  daLogMsg("INFO","End Executed");

  if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  CDODISABLE(LINUXVME,1,0);
  daLogMsg("INFO","Pause Executed");
  
  if (__the_event__) WRITE_EVENT_;
} /*end pause */

static void __go()
{
  daLogMsg("INFO","Entering Go");
  ACKLOCK;
  ack_runend=0;
  ACKUNLOCK;
  
  CDOENABLE(LINUXVME,1,1);
  rocGo();
  
#ifdef TIR_SOURCE
  tirIntEnable(TIR_CLEAR_COUNT); 
#elif defined(TS_SOURCE)
  tsIntEnable(1);
  tsGo(1);
#endif
  
  if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
  long EVENT_LENGTH;
  int ii, len, lock_key;
  DMANODE *outEvent;
 
  outEvent = dmaPGetItem(vmeOUT);
  if(outEvent != NULL) 
    {
      len = outEvent->length;
      CEOPEN(2, BT_UI4);
      
      if(rol->dabufp != NULL) 
	{
	  /* Will write over Event header/length with data from outEvent */
	  rol->dabufp -= 2;
	  for(ii=0;ii<len;ii++) 
	    {
	      *rol->dabufp++ = LSWAP(outEvent->data[ii]);
	    }
	} 
      else 
	{
	  printf("linuxvme_list: ERROR rol->dabufp is NULL -- Event lost\n");
	}

      CECLOSE;

      ACKLOCK;

      dmaPFreeItem(outEvent);

#ifdef TIR_SOURCE
      if(tirNeedAck>0)
#elif defined(TS_SOURCE)
      if(tsNeedAck > 0) 
#endif
	{
#ifdef TIR_SOURCE
	  tirNeedAck=0;
#elif defined(TS_SOURCE)
	  tsNeedAck=0;
#endif
	  ACKSIGNAL;
	}
      ACKUNLOCK;
    }
  else
    {
      logMsg("Error: no Event in vmeOUT queue\n",0,0,0,0,0,0);
    }
  
} /*end trigger */

unsigned long EVTYPE, EVNUM;

void asyncTrigger()
{
  int intCount=0;
  int length,size;
  unsigned int evData;

#ifdef TIR_SOURCE
  intCount = tirGetIntCount();
#elif defined(TS_SOURCE)
  intCount = tsGetIntCount();
#endif
  EVNUM = intCount;

  /* grap a buffer from the queue */
  GETEVENT(vmeIN,intCount);
  if(the_event->length!=0) 
    {
      printf("Interrupt Count = %d\t",intCount);
      printf("the_event->length = %d\n",the_event->length);
    }

#ifdef TIR_SOURCE
  /*Get RAW TIR data (event type and Sync) check for data and start transfer*/
  evData = tirReadData();
  tirDecodeTrigData(evData, &EVTYPE, &syncFlag, &lateFail);
#elif defined(TS_SOURCE)
  /* Get RAW TS data  */
  evData = tsIntLRocData();
  tsDecodeTrigData(evData, &EVTYPE, &syncFlag, &lateFail);
#endif
  

  /* Execute user defined Trigger Routine */
  rocTrigger();

  /* Put this event's buffer into the OUT queue. */
  ACKLOCK;
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

      /* Set the NeedAck for Ack after a buffer is freed */
#ifdef TIR_SOURCE
      tirNeedAck = 1;
#elif defined(TS_SOURCE)
      tsNeedAck = 1;
#endif

      /* Wait for the signal indicating that a buffer has been freed */
      ACKWAIT;

      /* Buffer should be freed now */
      ACKUNLOCK;
    }
  else
    {
      ACKUNLOCK;
    }
  ACKUNLOCK;
  
}

void usrtrig_done()
{
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
  static int ended=0;

  if(ended==0)
    {
      printf("ROC Cleanup\n");

      rocCleanup();

      dmaPFreeAll();
      vmeCloseDefaultWindows();

      ended = 1;
    }
}
