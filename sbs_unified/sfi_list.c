/*************************************************************************
 *
 *  sfi_list.c - Readout list for an SFI-based Fastbus crate with
 *               a CODA-3 style TI board.
 *
 *  author: R. Michaels, based on code linuxvme_list.c from B. Moffit
 *             with the SFI/Fastbus stuff added.
 *
 *   4dec2020: updated to CODA 3
 *
 *  Build with Makefile:
 *     make                      # Build Both Master and Slave rols
 *     make sfi_list.so          # Master only
 *     make sfi_slave_list.so    # Slave only
 *
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     128  /* Recommended >= 2 * BUFFERLEVEL */
#define MAX_EVENT_LENGTH  20000 /* Size in Bytes; 6500 would be 10 ADCs + 10 TDCs */
#define DS_TIMEOUT          50   /* how long to wait for datascan */

/* TI Type (TI_MASTER or TI_SLAVE)
   Should be defined by Makefile.  Otherwise: TI_MASTER */
#if (!defined(TI_MASTER) & !defined(TI_SLAVE))
#warning "TI_MASTER defined"
#define TI_MASTER
#endif

#ifdef TI_MASTER
/* EXTernal trigger source (e.g. front panel ECL input), POLL for available data */
#define TI_READOUT TI_READOUT_EXT_POLL
#else
/* TS trigger source (e.g. fiber), POLL for available data */
#define TI_READOUT TI_READOUT_TS_POLL
#endif

/* TI VME address, or 0 for Auto Initialize (search for TI by slot) */
#define TI_ADDR 0x80000

/* Measured longest fiber length in system */
#define FIBER_LATENCY_OFFSET 0x4A

#include "dmaBankTools.h"   /* Macros for handling CODA banks */
#define USE_SLAVE_WINDOW
#include "tiprimary_list-test.c" /* Source required for CODA readout lists using the TI */

int readout_ti=0;
int readout_fastbus=1;
int trig_mode=1;
/* Define initial blocklevel and buffering level */
#define BLOCKLEVEL 1
#define BUFFERLEVEL 1

static int padr, sadr;

/* Define the Bank of uint32 containing all inserted data from readout event */
#define FB_BANK 1

int branch_num=1;

/* pick multiblock(1) or not(0).  If not, then use default*dcCsr0 */

static int use_multiblock=1;
unsigned int defaultAdcCsr0=0x00000104;
unsigned int defaultTdcCsr0=0x00000104;

/* Back plane gate: 8B.  Front panel: 81    */
unsigned int defaultAdcCsr1=0x0000008B;

/* Need to load the fb_diag_cl.o library for these functions */
#include "fb_diag_cl.c"

int topAdc=0;
int bottomAdc=0;
int topTdc=0;
int bottomTdc=0;

#define MAXSLOTS 26

#include "libsfifb.h"

#include "usrstrutils.c"
#include "threshold.c"

unsigned int scan_mask;
int adcslots[MAXSLOTS];
int tdcslots[MAXSLOTS];
/* caution: nmodules is used by threshold.c and nmodule=nadc there */
int nadc=0;
int ntdc=0;

/* function prototype */
void rocTrigger(int arg);

/****************************************
 *  DOWNLOAD
 ****************************************/
void
rocDownload()
{

  unsigned int res;
  unsigned long laddr;
  int jj, sfi_addr;

  /* Define Block Level */
  blockLevel = BLOCKLEVEL;

  /*****************
   *   TI SETUP
   *****************/

  tiDisableBusError();

#ifdef TI_MASTER
  /*
   * Set Trigger source
   *    For the TI-Master, valid sources:
   *      TI_TRIGGER_FPTRG     2  Front Panel "TRG" Input
   *      TI_TRIGGER_TSINPUTS  3  Front Panel "TS" Inputs
   *      TI_TRIGGER_TSREV2    4  Ribbon cable from Legacy TS module
   *      TI_TRIGGER_PULSER    5  TI Internal Pulser (Fixed rate and/or random)
   */


#if (defined (INTFIXEDPULSER) | defined(INTRANDOMPULSER))
  tiSetTriggerSource(TI_TRIGGER_PULSER); /* TS Inputs enabled */
#else
  tiSetTriggerSource(TI_TRIGGER_FPTRG); /* Front Panel TRG */
#endif

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
#endif

  tiStatus(0);

  /***************************
   *   SFI and FASTBUS SETUP
   ***************************/

#ifdef VXWORKS
  if (sysLocalToBusAdrs(0x09,0,&sfi_cpu_mem_offset)) {
    printf("**ERROR** in sysLocalToBusAdrs() call \n");
    printf("sfi_cpu_mem_offset=0 FB Block Reads may fail \n");
  } else {
    printf("sfi_cpu_mem_offset = 0x%x \n",sfi_cpu_mem_offset);
  }
#endif
  sfi_addr=0xe00000;
#ifdef VXWORKS
  res = (unsigned int) sysBusToLocalAdrs(0x39,sfi_addr ,&laddr);
#endif
#ifdef LINUX
  res = (unsigned int) vmeBusToLocalAdrs(0x39,(char *)(unsigned long)sfi_addr ,(char **)&laddr);
#endif
  if (res != 0) {
    printf("Error Initializing SFI res=%d \n",res);
  } else {
    printf("Calling InitSFI() routine with laddr=0x%lx.\n",laddr);
    InitSFI(laddr);
  }

  InitFastbus(0x20,0x33);


  printf("Map of Fastbus Modules \n");
  fb_map();

  /*   load_cratemap(); */

  nmodules=0;
  nadc = 0;
  ntdc = 0;

  for (jj=0; jj<MAXSLOTS; jj++) {
    adcslots[jj] = -1;  /* init */
    tdcslots[jj] = -1;
  }
  for (jj=0; jj<MAXSLOTS; jj++) {
    if (isAdc1881(jj)) {
      adcslots[nadc]=jj;
      nmodules++;
      nadc++;
    }
    if (isTdc1877(jj)) {
      tdcslots[ntdc]=jj;
      ntdc++;
    }
  }
  scan_mask = 0;
  topAdc=-1;
  bottomAdc=MAXSLOTS+1;
  topTdc=-1;
  bottomTdc=MAXSLOTS+1;
  for (jj=0; jj< nadc ; jj++) {
    if (adcslots[jj] >= 0) {
      scan_mask |= (1<<adcslots[jj]);
      if (adcslots[jj]>topAdc) topAdc=adcslots[jj];
      if (adcslots[jj]<bottomAdc) bottomAdc=adcslots[jj];
    }
  }
  for (jj=0; jj< ntdc ; jj++) {
    if (tdcslots[jj] >= 0) {
      scan_mask |= (1<<tdcslots[jj]);
      if (tdcslots[jj]>topTdc) topTdc=tdcslots[jj];
      if (tdcslots[jj]<bottomTdc) bottomTdc=tdcslots[jj];
    }
  }
  printf ("constructed Crate Scan mask = %x\n",scan_mask);
  if (topAdc > -1) {
    printf ("topAdc %d   bottomAdc %d\n",topAdc,bottomAdc);
  } else {
    printf ("No ADCs in this crate \n");
  }
  if (topTdc > -1) {
    printf ("topTdc %d   bottomTdc %d\n",topTdc,bottomTdc);
  } else {
    printf ("No TDCs in this crate \n");
  }

  printf("rocDownload: User Download Executed\n");

}

/****************************************
 *  PRESTART
 ****************************************/
void
rocPrestart()
{
  unsigned int pedsuppress, csrvalue;
  int kk, padr, sadr;

#ifdef TI_MASTER
  /* Set number of events per block (broadcasted to all connected TI Slaves)*/
  tiSetBlockLevel(blockLevel);
  printf("rocPrestart: Block Level set to %d\n",blockLevel);
#endif

  tiStatus(0);

  fb_init_1(0);

  /* reset ADCs */
  for (kk=0; kk<nadc; kk++) {
    padr   = adcslots[kk];
    if (padr >= 0) fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,0,0);
  }
  sfi_error_decode(0);

  pedsuppress = 0;
  /* normally would use usrstrutils to get this flag; later */
  printf("ped suppression ? %d \n",pedsuppress);
  if(pedsuppress) {
    load_thresholds();
    set_thresholds();
  }
  sfi_error_decode(0);

  /* program the ADC and TDC modules.  top slot and bottom are book-ends to
     form a multiblock.  This means there should be at least 3 modules of each type. */

  printf("Programming the Fastbus modules.\n");

  for (kk=0; kk<nadc; kk++) {
    padr   = adcslots[kk];
    if (padr >= 0) {

      if (use_multiblock) {
	csrvalue = 0x00001904;
	if (padr == topAdc) csrvalue = 0x00000904;
	if (padr == bottomAdc) csrvalue = 0x00001104;
      } else {
	csrvalue = defaultAdcCsr0;
      }

      printf("ADC slot %d  %d   csr0 0x%x \n",kk,padr,csrvalue);
      fb_fwc_1(padr,0,csrvalue,1,1,0,1,0,1,1);

      sadr = 1;
      csrvalue = defaultAdcCsr1;
      if (pedsuppress == 1) csrvalue |= 0x40000000;
      printf("ADC csr1 0x%x \n",csrvalue);
      fb_fwc_1(0,sadr,csrvalue,1,1,1,0,0,1,1);

      sadr = 7 ;
      fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
      fprel();
      sfi_error_decode(0);
    }
  }

  for (kk=0; kk<ntdc; kk++) {
    padr   = tdcslots[kk];
    if (padr >= 0) {
       fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,1,1); 
       fprel();
    }
  }
  sfi_error_decode(0);

  for (kk=0; kk<ntdc; kk++) {
    padr   = tdcslots[kk];
    if (padr >= 0) {

      if (use_multiblock) {
	csrvalue = 0x00001904;
	if (padr == topTdc) csrvalue = 0x00000904;
	if (padr == bottomTdc) csrvalue = 0x00001104;
      } else {
	csrvalue = defaultTdcCsr0;
      }

      printf("TDC slot %d  %d   csr0 0x%x \n",kk,padr,csrvalue);
      fb_fwc_1(padr,0,csrvalue,1,1,0,1,0,1,1);
      sadr = 1 ;
      fb_fwc_1(0,sadr,0x40000003,1,1,1,0,0,1,1);
      sadr = 18 ; 
      fb_fwc_1(0,sadr,0xbb6,1,1,1,0,0,1,1);
      sadr = 7 ;
      fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
      fprel();
      sfi_error_decode(0);
    }
  }

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

  /* Get the broadcasted Block and Buffer Levels from TS or TI Master */
  blockLevel = tiGetCurrentBlockLevel();
  bufferLevel = tiGetBroadcastBlockBufferLevel();
  printf("rocGo: Block Level set to %d  Buffer Level set to %d\n",blockLevel,bufferLevel);

  /* In case of slave, set TI busy to be enabled for full buffer level */
  tiUseBroadcastBufferLevel(1);

#ifdef TI_MASTER
  /* Enable/Set Block Level on modules, if needed, here */
#if (defined (INTFIXEDPULSER) | defined(INTRANDOMPULSER))
  printf("************************************************************\n");
  printf("%s: TI Configured for Internal Pulser Triggers\n",
	 __func__);
  printf("************************************************************\n");
#endif

  /* Example: How to start internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Enable Random at rate 500kHz/(2^7) = ~3.9kHz */
  tiSetRandomTrigger(1,0x7);
#elif defined (INTFIXEDPULSER)
  /*
    Enable fixed rate with period (ns)
    120 +30*700*(1024^0) = 21.1 us (~47.4 kHz)
     - arg2 = 0xffff - Continuous
     - arg2 < 0xffff = arg2 times
  */
  tiSoftTrig(1,0xffff,700,0);
#endif
#endif

  printf("rocGo: User Go Executed\n");
}

/****************************************
 *  END
 ****************************************/
void
rocEnd()
{
#ifdef TI_MASTER
  /* Example: How to stop internal pulser trigger */
#ifdef INTRANDOMPULSER
  /* Disable random trigger */
  tiDisableRandomTrigger();
#elif defined (INTFIXEDPULSER)
  /* Disable Fixed Rate trigger */
  tiSoftTrig(1,0,700,0);
#endif
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
  //printf("sfi list::roc trigger, arg %d\n", arg);

  int ii, res, islot, rlen, kcnt;
  int dCnt;
  static int verbose=0;
  static int evcnt=0;

  unsigned int datascan, fbres, numBranchdata;
  unsigned int lenb = (((MAX_EVENT_LENGTH>>4)-2)<<2);

  static int ldebug=1;

  evcnt++;

#ifdef LINUX
  unsigned long dmaptr;
  static unsigned int *pfbdata;
  unsigned int rb;
  pfbdata = (unsigned int *)dma_dabufp;
#endif

  tiSetOutputPort(1,0,0,0);

  numBranchdata=1;

  /* Setup Address and data modes for DMA transfers
   *
   *  vmeDmaConfig(addrType, dataType, sstMode);
   *
   *  addrType = 0 (A16)    1 (A24)    2 (A32)
   *  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
   *  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
   */
  vmeDmaConfig(2,3,1);

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

  BANKOPEN(FB_BANK, BT_UI4, blockLevel);

  ii=0;
  datascan = 0;

  if (branch_num==numBranchdata ) {
    while ((ii<DS_TIMEOUT) && ((datascan&scan_mask) != scan_mask)) {
      fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
      ii++;
    }
  }

  /* I wonder why *(rol->dabufp)++ works for VXWORKS but seg faults for LINUX.
     Instead, for LINUX we must *dma_dabufp++ and also do LSWAP */
#ifdef VXWORKS
  *(rol->dabufp)++ = 0xda000011;
  *(rol->dabufp)++ = datascan;
  *(rol->dabufp)++ = branch_num;
  *(rol->dabufp)++ = numBranchdata;
  *(rol->dabufp)++ = lenb;
  *(rol->dabufp)++ = ii;
#endif
#ifdef LINUX
  *dma_dabufp++ = LSWAP(0xda000011);
  *dma_dabufp++ = LSWAP(datascan);
  *dma_dabufp++ = LSWAP(branch_num);
  *dma_dabufp++ = LSWAP(numBranchdata);
  *dma_dabufp++ = LSWAP(lenb);
  *dma_dabufp++ = LSWAP(ii);
#endif


  if (ii<DS_TIMEOUT && readout_fastbus) {


    fb_fwcm_1(0x15,0,0x400,1,0,1,0,0,0);

    /* First read the ADCs */

    if (topAdc >= 0) {
#ifdef VXWORKS
      fpbr(topAdc,lenb);
      *(rol->dabufp)++ = 0xda000022;
#endif
#ifdef LINUX
      res=-1;
      if (use_multiblock) {
	dmaptr = vmeDmaLocalToVmeAdrs((unsigned long)dma_dabufp);
	res = fb_frdb_1(topAdc,0,(unsigned int *)dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
	rlen = rb>>2;
	if(ldebug) logMsg("multiblock Adc rlen %d  %d  %d   dma_dabufp 0x%x \n",rb,rlen,res,dma_dabufp);
	if(res == 0) dma_dabufp += rlen;
      } else {  /* read ADCs from each slot, the old slow way */
	for (islot=bottomAdc; islot<=topAdc; islot++) {
	  if(ldebug) logMsg("non multiblock Adc read loop, ev %d, smask 0x%x  islot %d \n",evcnt,datascan,islot);
	  dmaptr = vmeDmaLocalToVmeAdrs((unsigned long)dma_dabufp);
	  res = fb_frdb_1(islot,0,(unsigned int *)dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
	  rlen = rb>>2;
	  if(res == 0) dma_dabufp += rlen;
	  if(ldebug) logMsg("individual Adc slot %d rlen %d  %d %d dma_dabufp 0x%x \n",islot, rb,rlen,res, dma_dabufp);
	  if (rlen < 0 || rlen > MAX_EVENT_LENGTH) rlen=MAX_EVENT_LENGTH;
	  for(ii=0;ii<rlen;ii++) *dma_dabufp++ = LSWAP(pfbdata[ii]);
	}

      }
      *dma_dabufp++ = LSWAP(0xda000022);
#endif
    }

    /* ----------------------------------------- */
    /* Now read the TDCs */

    if (topTdc >= 0) {
#ifdef VXWORKS
      fpbr(topTdc,lenb);
      *(rol->dabufp)++ = ii;
      *(rol->dabufp)++ = 0xda000033;
#endif
#ifdef LINUX
      *dma_dabufp++ = LSWAP(use_multiblock);  /* AAAAAAAAAAAA */

      res=-1;
      if (use_multiblock) {
	dmaptr = vmeDmaLocalToVmeAdrs((unsigned long)dma_dabufp);
	res = fb_frdb_1(topTdc,0,(unsigned int *)dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
	/* the above may return res= 0x1c080 */
	rlen = rb>>2;
	if(ldebug) logMsg("multiblock Tdc rb= %d  rlen= %d  res= 0x%x   dma_dabufp= 0x%x \n",rb,rlen,res,dma_dabufp);
	if(res == 0) dma_dabufp += rlen;
        *dma_dabufp++ = LSWAP(rb);  /* AAAAAAAAAAAA */
        *dma_dabufp++ = LSWAP(pfbdata[0]);  /* AAAAAAAAAAAA */
        *dma_dabufp++ = LSWAP(0xaaaaaaaa);  /* AAAAAAAAAAAA */
      } else { /* read TDCs from each slot, the old slow way */
        kcnt=0;
	dmaptr = vmeDmaLocalToVmeAdrs((unsigned long)dma_dabufp);
	for (islot=bottomTdc; islot<=topTdc; islot++) {
	  if(ldebug) logMsg("non multiblock Tdc read loop, ev %d, smask 0x%x  islot %d \n",evcnt,datascan,islot);
	  res = fb_frdb_1(islot,0,(unsigned int *)dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1); 
	  rlen = rb>>2;
	  if(ldebug) logMsg("Tdc(nmb) rlen %d  %d  %d \n",rb,rlen,kcnt);
	  if (rlen < 0 || rlen > MAX_EVENT_LENGTH) rlen=MAX_EVENT_LENGTH;
          for(ii=0;ii<rlen;ii++) *dma_dabufp++ = LSWAP(pfbdata[ii]);
          *dma_dabufp++ = LSWAP(rb);  /* AAAAAAAAAAAA */
          *dma_dabufp++ = LSWAP(kcnt);  /* AAAAAAAAAAAA */
          *dma_dabufp++ = LSWAP(pfbdata[0]);  /* AAAAAAAAAAAA */
          *dma_dabufp++ = LSWAP(0xbbbbbbbb);  /* AAAAAAAAAAAA */	  
	}
      }
      *dma_dabufp++ = LSWAP(ii);
      *dma_dabufp++ = LSWAP(0xda000033);
#endif
    }

  } else {



#ifdef VXWORKS
    *(rol->dabufp)++ = 0xdabb00ff;
#endif
#ifdef LINUX
    *dma_dabufp++ = LSWAP(0xdabb00ff);
#endif
  }
  BANKCLOSE;


  datascan = 0;
  fbres = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
  if (fbres && verbose) logMsg("fbres = 0x%x scan_mask 0x%x datascan 0x%x\n",fbres,scan_mask,datascan,0,0,0);
  if ((datascan != 0) && (datascan&~scan_mask)) {
    logMsg("Error: Read data but More data available after readout datascan = 0x%08x fbres = 0x%x\n",datascan,fbres,0,0,0,0);
  }

  tiSetOutputPort(0,0,0,0);

  //printf("event closed, end of routine sfi list::roc trigger\n");

}

void
rocCleanup()
{
  printf("%s: Reset all Modules\n",__FUNCTION__);
#ifdef TI_MASTER
  tiResetSlaveConfig();
#endif
}

/*
  Local Variables:
  compile-command: "make -k -B sfi_list.so sfi_slave_list.so"
  End:
 */
