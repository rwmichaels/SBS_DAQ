/*************************************************************************
 *
 *  vme_list.c - Library of routines for readout and buffering of
 *                events using a JLAB Trigger Interface V3 (TI) with
 *                a Linux VME controller.
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     128  /* Recommended >= 2 * BUFFERLEVEL */
#define MAX_EVENT_LENGTH   512  /* Size in Bytes */

/* Define Interrupt source and address */
#define TI_READOUT TI_READOUT_EXT_POLL
#define TI_ADDR    0          /* GEO slot 21 */

#define FIBER_LATENCY_OFFSET 0x4A  /* measured longest fiber length */

/* Default block level and buffer level */
unsigned int BLOCKLEVEL=1;
unsigned int BUFFERLEVEL=1;

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

  /* Set crate ID */
  tiSetCrateID(0x01); /* ROC 1 */

  tiSetTriggerSource(TI_TRIGGER_PULSER);

  /* Set needed TS input bits */
  tiEnableTSInput( TI_TSINPUT_1 );

  /* Load the trigger table that associates
     pins 21/22 | 23/24 | 25/26 : trigger1
     pins 29/30 | 31/32 | 33/34 : trigger2
  */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

#ifdef TI_MASTER
  /* Set number of events per block */
  tiSetBlockLevel(BLOCKLEVEL);
#endif

  tiSetEventFormat(1);

  tiSetBlockBufferLevel(BUFFERLEVEL);

  tiStatus(0);


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
  /* Get the current block and buffer level */
  BLOCKLEVEL = tiGetCurrentBlockLevel();
  BUFFERLEVEL = tiGetBroadcastBlockBufferLevel();
  printf("%s: Current Block Level = %d   Buffer Level = %d\n",
	 __FUNCTION__, BLOCKLEVEL, BUFFERLEVEL);

  tiSetRandomTrigger(1,0x7);

  printf("rocGo: User Go Executed\n");
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{
  tiDisableRandomTrigger();
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

  BANKCLOSE;

  EVENTCLOSE;

  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
}
