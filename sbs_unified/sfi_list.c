/*************************************************************************
 *
 *  sfi_list.c - Readout list for an SFI-based Fastbus crate with
 *               a CODA-3 style TI board.
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     128  /* Recommended >= 2 * BUFFERLEVEL */
#define MAX_EVENT_LENGTH   512  /* Size in Bytes */


/* Global variables here */

/* parameters that configure the crate */

/* This is the thumbwheel switch on the TI module */
int TI_ADDR=0x80000;  

int readout_ti=1;
int trig_mode=1;  
int BLOCKLEVEL=1;
int BUFFERLEVEL=1;

int branch_num=1;  

#define FIBER_LATENCY_OFFSET 0x4A  /* measured longest fiber length */
#define TI_READOUT TI_READOUT_EXT_POLL

#include "tiprimary_list.c" /* source required for CODA */

/* function prototype */
void rocTrigger(int arg);

/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{
  /*****************
   *   TI SETUP
   *****************/

  tiSetFiberLatencyOffset_preInit(0x40); 
  tiSetCrateID_preInit(0x1);  
  tiInit(TI_ADDR ,trig_mode ,0);
  tiDisableBusError();
  if(readout_ti==0)  
    {
      tiDisableDataReadout();
      tiDisableA32();
    }
  tiSetBlockBufferLevel(BUFFERLEVEL );
  tiSetBusySource(0,1);
  tiSetTriggerSource(6 );
  tiStatus(0);
  tiDisableVXSSignals();
  tiSetEventFormat(2);

  printf("rocDownload: User Download Executed\n");

}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{
  unsigned short iflag;
  int stat;
  int islot;

  tiStatus(0);

  printf("rocPrestart: User Prestart Executed\n");

}

/****************************************
 *  GO
 ****************************************/
void
rocGo()
{
  printf("%s: Block Level = %d   Buffer Level = %d\n",
	 __FUNCTION__, BLOCKLEVEL, BUFFERLEVEL);

  printf("rocGo: User Go Executed\n");
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{
  tiStatus(0);

  printf("rocEnd: Ended after %d blocks\n",tiGetIntCount());

}

/****************************************
 *  TRIGGER
 ****************************************/
void
rocTrigger(int arg)
{
  int ii, islot;
  int stat, dCnt, len=0, idata;
  int ev_type = 0;

  EVENTOPEN(ev_type, BT_BANK);

  tiSetOutputPort(1,0,0,0);

  BANKOPEN(4,BT_UI4,0);


  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,3,1);

  /* test words */
  *dma_dabufp++ = 0xb0b0b044;
  *dma_dabufp++ = 0xb0b0b055;
  *dma_dabufp++ = 0xb0b0b066;
  *dma_dabufp++ = 0xb0b0b077;


  if (readout_ti==1) {
    dCnt = tiReadBlock((volatile unsigned int *)dma_dabufp,
		     8+(5*BLOCKLEVEL), 1);
    if(dCnt<=0)
      {
        logMsg("No data or error.  dCnt = %d\n",
	     dCnt, 2, 3, 4, 5, 6);
      }
    else
      {
        ev_type = tiDecodeTriggerType((volatile unsigned int *)dma_dabufp, dCnt, 1);
        if(ev_type <= 0)
	  {
	     /* Could not find trigger type */
	    ev_type = 1;
	  }

      /* CODA 2.x only allows for 4 bits of trigger type */
      ev_type &= 0xF;

      RESET_EVTYPE(ev_type);

      dma_dabufp += dCnt;
    }
  }

  BANKCLOSE;

  EVENTCLOSE;

  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
}
