/*************************************************************************
 *
 *  ti_master_list.c - Library of routines for readout and buffering of
 *                     events using a JLAB Trigger Interface V3 (TI) with
 *                     a Linux VME controller in CODA 3.0.
 *
 *                     This for a TI in Master Mode controlling multiple ROCs
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     10
#define MAX_EVENT_LENGTH   1024*60      /* Size in Bytes */

/* Define TI Type (TI_MASTER or TI_SLAVE) */
#define TI_MASTER
/* EXTernal trigger source (e.g. front panel ECL input), POLL for available data */
#define TI_READOUT TI_READOUT_EXT_POLL
/* TI VME address, or 0 for Auto Initialize (search for TI by slot) */
#define TI_ADDR 0

//#define TI_ADDR 0x80000

/* Measured longest fiber length in system */
#define FIBER_LATENCY_OFFSET 0

#include "dmaBankTools.h"   /* Macros for handling CODA banks */
#include "tiprimary_list.c" /* Source required for CODA readout lists using the TI */
#include "fadcLib.h"
#include "sdLib.h"
#include "usrstrutils.c" // utils to pick up CODA ROC config parameter line


/* Define initial blocklevel and buffering level */
unsigned int BLOCKLEVEL = 1;
int BUFFERLEVEL=1;


/* FADC Defaults/Globals */
#define FADC_DAC_LEVEL    3100 //was 3100
#define FADC_THRESHOLD    500 //was 500
#define FADC_WINDOW_WIDTH 100 //was 20
#define FADC_MODE          10 //was 10 

#define FADC_LATENCY       220 // was 88 was 47
#define FADC_NSB           3  // # of samples *before* Threshold crossing (TC) to include in sum
#define FADC_NSA           15 // 20//15 // was 60 // # of samples *after* Threshold crossing (TC) to include in sum
#define FADC_SH_THRESHOLD     9 // changed 8/6/2017 from 300 : cosmic signals are not large enough to be above threshold
#define chan_mask  0x0000 // chan mask for threshold setting


extern int fadcA32Base;
extern int nfadc;

#define NFADC 1
/* Address of first fADC250 */
#define FADC_ADDR 0x280000
/* Increment address to find next fADC250 */
#define FADC_INCR 0x080000

//wether or not to use threshold
#define WANT_THRESHOLD 0

int sync_or_unbuff;
static int buffered;
static int event_cnt = 0;
static int icnt = 0;

unsigned int fadcSlotMask=0;

/* for the calculation of maximum data words in the block transfer */
/* this is calculated below, depending on the mode */
unsigned int MAXFADCWORDS=0;

unsigned int fadc_threshold=FADC_THRESHOLD;
unsigned int fadc_window_lat=FADC_LATENCY, fadc_window_width=FADC_WINDOW_WIDTH;




/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{
  int stat;
  int iFlag, ifa,ichan1;
  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */

  init_strings();
  buffered = getflag(BUFFERED);
  if (!buffered) BUFFERLEVEL=1 ;
  printf ("Buffer flag : %d\n",buffered);


  vmeDmaConfig(2,5,1);

  /* Define BLock Level */
  blockLevel = BLOCKLEVEL;


  /*****************
   *   TI SETUP
   *****************/

  /* Set crate ID */
  tiSetCrateID(0x04);		/* e.g. ROC 4 */

  /*
   * Set Trigger source
   *    For the TI-Master, valid sources:
   *      TI_TRIGGER_FPTRG     2  Front Panel "TRG" Input
   *      TI_TRIGGER_TSINPUTS  3  Front Panel "TS" Inputs
   *      TI_TRIGGER_TSREV2    4  Ribbon cable from Legacy TS module
   *      TI_TRIGGER_PULSER    5  TI Internal Pulser (Fixed rate and/or random)
   */

  tiSetTriggerSource(TI_TRIGGER_FPTRG); 

  /* Enable set specific TS input bits (1-6) */
  tiEnableTSInput( TI_TSINPUT_1 | TI_TSINPUT_2 );

  /* Load the trigger table that associates
   *  pins 21/22 | 23/24 | 25/26 : trigger1
   *  pins 29/30 | 31/32 | 33/34 : trigger2
   */
  tiLoadTriggerTable(0);

  tiSetTriggerHoldoff(1,10,0);
  tiSetTriggerHoldoff(2,10,0);

  /* Set initial number of events per block */
  tiSetBlockLevel(blockLevel);

  /* Set Trigger Buffer Level */
  tiSetBlockBufferLevel(BUFFERLEVEL);

  tiStatus(0);

  //Added by Maria to test delays March 17 2021
  // tiSetTriggerPulse(1,25,3,0);

  /*******************
   *   FADC250 SETUP
   *******************/
  fadcA32Base=0x09000000;
  iFlag = 0;
  iFlag |= FA_INIT_EXT_SYNCRESET; /* External (VXS) SyncReset*/
  iFlag |= FA_INIT_VXS_TRIG;      /* VXS Input Trigger */
  iFlag |= FA_INIT_VXS_CLKSRC;    /* Internal Clock Source (Will switch later) */
  int thrshflag;
  //int R1thrshold[16] = {610,500,415,615,643,685,292,350,350,350,350,350,350,350,350,350};
  int R1thrshold[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  vmeSetQuietFlag(1);
  faInit(FADC_ADDR, FADC_INCR, NFADC, iFlag);
  /* undefined symbol in CODA 3   faGDataInsertAdcParameters(1); */
  vmeSetQuietFlag(0);


  if(nfadc>1)
    faEnableMultiBlock(1);

  fadcSlotMask=faScanMask();

  MAXFADCWORDS = 100 + nfadc * (4 + blockLevel * (4 + 16 * (1 + (FADC_WINDOW_WIDTH / 2))) + 18);

  printf("nfadc here %d  maxfadcwords %d \n",nfadc,MAXFADCWORDS);


  for(ifa = 0; ifa< nfadc; ifa++) {
      faEnableBusError(faSlot(ifa));

      /* Set the internal DAC level */
      faSetDAC(faSlot(ifa), FADC_DAC_LEVEL, 0xffff);

      /* Set the threshold for data readout */
      faSetThreshold(faSlot(ifa), fadc_threshold, 0xffff);


      /* here can go a lot of tailored DAC and threshold settings */



/**********************************************************************************
* faSetProcMode(int id, int pmode, unsigned int PL, unsigned int PTW,
*    int NSB, unsigned int NSA, unsigned int NP,
*    unsigned int NPED, unsigned int MAXPED, unsigned int NSAT);
*
*  id    : fADC250 Slot number
*  pmode : Processing Mode
*          9 - Pulse Parameter (ped, sum, time)
*         10 - Debug Mode (9 + Raw Samples)
*    PL : Window Latency
*   PTW : Window Width
*   NSB : Number of samples before pulse over threshold
*   NSA : Number of samples after pulse over threshold
*    NP : Number of pulses processed per window
*  NPED : Number of samples to sum for pedestal
*MAXPED : Maximum value of sample to be included in pedestal sum
*  NSAT : Number of consecutive samples over threshold for valid pulse
*/

     faSetProcMode(faSlot(ifa),
		    FADC_MODE,
		    FADC_LATENCY,
		    FADC_WINDOW_WIDTH,
		    FADC_NSB,   /* NSB */
		    FADC_NSA,   /* NSA */
		    4,   /* NP */
		    4,   /* NPED */
		    320, /* MAXPED */
		    2);  /* NSAT */
 

  } /* loop over slots of FADC */

  faGStatus(0);

  /*****************
   *   SD SETUP
   *****************/
  sdInit(0);
  sdSetActiveVmeSlots(fadcSlotMask);
  sdStatus(1);

  printf("Download: FADC setup parameters mode %d  latency %d  window %d \n",FADC_MODE, FADC_LATENCY,FADC_WINDOW_WIDTH);

  printf("rocDownload: User Download Executed\n");


}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{


   int ifa;

  /* Program/Init VME Modules Here */
  for(ifa=0;ifa<nfadc;ifa++)
    {
      faEnableSyncSrc(faSlot(ifa));
      faSoftReset(faSlot(ifa),0);
      faResetToken(faSlot(ifa));
      faResetTriggerCount(faSlot(ifa));
    }


  /* Set number of events per block (broadcasted to all connected TI Slaves)*/
  tiSetBlockLevel(blockLevel);
  printf("rocPrestart: Block Level set to %d\n",blockLevel);

  tiStatus(0);

#ifdef EXAMPLE_UE
  /* EXAMPLE: User bank of banks added to prestart event */
  UEOPEN(500,BT_BANK,0);

  /* EXAMPLE: Bank of data in User Bank 500 */
  CBOPEN(1,BT_UI4,0);
  *rol->dabufp++ = 0x11112222;
  *rol->dabufp++ = 0x55556666;
  *rol->dabufp++ = 0xaabbccdd;
  CBCLOSE;

  UECLOSE;
#endif

    printf("PreStart: FADC setup parameters mode %d  latency %d  window %d \n",FADC_MODE, FADC_LATENCY,FADC_WINDOW_WIDTH);


  printf("rocPrestart: User Prestart Executed\n");

}

/****************************************
 *  GO
 ****************************************/
void
rocGo()
{

  /* Print out the Run Number and Run Type (config id) */
  printf("rocGo: Activating Run Number %d, Config id = %d\n",
	 rol->runNumber,rol->runType);

  /* Get the current Block Level */
  blockLevel = tiGetCurrentBlockLevel();
  printf("rocGo: Block Level set to %d\n",blockLevel);


  /* Example: How to start internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Enable Random at rate 500kHz/(2^7) = ~3.9kHz */
  tiSetRandomTrigger(1,0x7);
#elif defined (INTFIXEDPULSER)
  /* Enable fixed rate with period (ns) 120 +30*700*(1024^0) = 21.1 us (~47.4 kHz)
     - Generated 1000 times */
  tiSoftTrig(1,1000,700,0);
#endif

  int ifa;


  for(ifa=0;ifa<nfadc;ifa++)
    {
      faChanDisable(faSlot(ifa),0x0);
    }

  faGStatus(0);
  faGDisable(0);

  /* Enable/Set Block Level on modules here */
  faGSetBlockLevel(blockLevel);

  if(FADC_MODE == 9)
    MAXFADCWORDS = nfadc * (100 + 2 + 4 + 16* blockLevel * 16);
  else /* FADC_MODE == 10 */
    MAXFADCWORDS = 100 + nfadc * (4 + blockLevel * (4 + 16 * (1 + (FADC_WINDOW_WIDTH / 2))) + 18);

      /* nfadc * (100 + 2 + 4 + 16 * blockLevel * (16 + FADC_WINDOW_WIDTH/2)); */ /* add the 100 "hedge" as per hall C */

  faGEnable(0, 0); 
  /* Interrupts/Polling enabled after conclusion of rocGo() */
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{

  faGDisable(0);
  faGStatus(0);
  sdStatus(1);


  /* Example: How to stop internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Disable random trigger */
  tiDisableRandomTrigger();
#elif defined (INTFIXEDPULSER)
  /* Disable Fixed Rate trigger */
  tiSoftTrig(1,0,700,0);
#endif

  tiStatus(0);

  printf("rocEnd: Ended after %d blocks\n",tiGetIntCount());

}

/****************************************
 *  TRIGGER
 ****************************************/
void
rocTrigger(int arg)
{
  int dCnt;
  int ev_type = 0;
  int ifa, stat, nwords;
  int roCount = 0, blockError = 0;
  int rval = OK;
  int sync_flag = 0, late_fail = 0;
  unsigned int datascan = 0, scanmask = 0;
  unsigned int event_ty = 1, event_no = 0;
  unsigned int tmp_evheader = 0;
  int islot;
  int errFlag = 0;


  /* Set TI output 1 high for diagnostics */
  tiSetOutputPort(1,0,0,0);

  roCount = tiGetIntCount();

  /* Readout the trigger block from the TI
     Trigger Block MUST be readout first */
  dCnt = tiReadTriggerBlock(dma_dabufp);

  if(dCnt<=0)
    {
      printf("No TI Trigger data or error.  dCnt = %d\n",dCnt);
    }
  else
    { /* TI Data is already in a bank structure.  Bump the pointer */
      dma_dabufp += dCnt;
    }

 /* fADC250 Readout */
  BANKOPEN(250,BT_UI4,blockLevel);

  /* Mask of initialized modules */
  scanmask = faScanMask();
  /* Check scanmask for block ready up to 100 times */
  datascan = faGBlockReady(scanmask, 100);
  stat = (datascan == scanmask);

  if(stat)
    {
      /*         nwords = faReadBlock(faSlot(0), dma_dabufp, MAXFADCWORDS, 2); */

     nwords = faReadBlock(0, dma_dabufp, MAXFADCWORDS, 1); 

      /* Check for ERROR in block read */
      blockError = faGetBlockError(1);

      if(blockError)
	{
	  printf("ERROR: in transfer (event = %d), nwords = 0x%x\n",
		 roCount, nwords);
	  for(ifa = 0; ifa < nfadc; ifa++)
	    faResetToken(faSlot(ifa));

	  if(nwords > 0)
	    dma_dabufp += nwords;
	}
      else
	{
	  dma_dabufp += nwords;
	  faResetToken(faSlot(0));
	}
    }
  else
    {
      printf("ERROR: Event %d: Datascan != Scanmask  (0x%08x != 0x%08x)\n",
	     roCount, datascan, scanmask);
    }

  BANKCLOSE;


 BANKOPEN(0xfabc,BT_UI4,0);		//Sync checks
  event_cnt = event_cnt + 1;
  icnt = icnt + 1;
  if(icnt > 20000) icnt = 0;
  *dma_dabufp++ = LSWAP(0xfabc0004);
  *dma_dabufp++ = LSWAP(event_ty);
  *dma_dabufp++ = LSWAP(event_cnt);
  *dma_dabufp++ = LSWAP(icnt);
  *dma_dabufp++ = LSWAP(syncFlag);
  *dma_dabufp++ = LSWAP(0xfaaa0001);
  BANKCLOSE;

  if(tiGetSyncEventFlag() == 1|| !buffered)
    {
      /* Flush out TI data, if it's there (out of sync) */
      int davail = tiBReady();
      if(davail > 0)
	{
	  printf("%s: ERROR: TI Data available (%d) after readout in SYNC event \n",
		 __func__, davail);

	  while(tiBReady())
	    {
	      vmeDmaFlush(tiGetAdr32());
	    }
	}

      /* Flush out other modules too, if necessary */
      // flush FADCs
      scanmask = faScanMask();
      /* Check scanmask for block ready up to 100 times */
      datascan = faGBlockReady(scanmask, 100);
      stat = (datascan == scanmask);
      stat =0;
      if (stat > 0)
	{
	  printf("data left in FADC FIFO at sync event\n");
	  //FADC sync event bank
	  // nwords = faReadBlock(0, dma_dabufp, 5000, 2);	//changed rflag = 2 for Multi Block transfer 5/25/17
	  BANKOPEN(0xbad,BT_UI4,0);
	  *dma_dabufp++ = LSWAP(0xfadc250);
	  nwords = faReadBlock(faSlot(0), dma_dabufp, 7200, 2);
	  // nwords = 0;
	  // nwords = 0;
	  if (nwords < 0)
	    {
	      printf("ERROR: in transfer (event = %d), nwords = 0x%x\n",
		    event_cnt, nwords);
	        *dma_dabufp++ = LSWAP(0xda000bad);

	    }
	  else
	    {
	      dma_dabufp += nwords;
	    }
	    BANKCLOSE;
	  for (islot = 0; islot < nfadc; islot++)	// 5/25/17
	    faResetToken(faSlot(islot));
	  for(islot = 0; islot < nfadc; islot++)
	    {
	      int davail = faBready(faSlot(islot));
	      if(davail > 0)
		{
		  printf("%s: ERROR: fADC250 Data available after readout in SYNC event \n",
			 __func__, davail);

		  while(faBready(faSlot(islot)))
		    {
		      vmeDmaFlush(faGetA32(faSlot(islot)));
		    }
		}

	    }
	}

    } /* if(tiGetSyncEventFlag() == 1|| !buffered) */

  /* Set TI output 0 low */
  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{

  printf("%s: Reset all FADCs\n",__FUNCTION__);
  faGReset(1);

}
