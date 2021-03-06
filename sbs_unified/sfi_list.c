/*************************************************************************
 *
 *  sfi_list.c - Readout list for an SFI-based Fastbus crate with
 *               a CODA-3 style TI board.
 *
 *  author: R. Michaels, based on code linuxvme_list.c from B. Moffit
 *             with the SFI/Fastbus stuff added.
 */

/* Event Buffer definitions */
#define MAX_EVENT_POOL     128  /* Recommended >= 2 * BUFFERLEVEL */
#define MAX_EVENT_LENGTH  6500  /* Size in Bytes; assumes 10 ADCs + 10 TDCs */
#define DS_TIMEOUT          50   /* how long to wait for datascan */

/* Global variables here */

/* parameters that configure the crate */

/* This is the thumbwheel switch on the TI module */
int TI_ADDR=0x80000;  

int readout_ti=1;
int readout_fastbus=1;
int trig_mode=1;  
int BLOCKLEVEL=1;
int BUFFERLEVEL=1;

int branch_num=1;  

/* pick multiblock(1) or not(0).  If not, then use defaultAdcCsr0 */
static int use_multiblock=1;
unsigned long defaultAdcCsr0=0x00000104;

/* Back plane gate: 8B.  Front panel: 81    */
unsigned long defaultAdcCsr1=0x0000008B;

/* Need to load the fb_diag_cl.o library for these functions */
extern int isAdc1881(int slot);
extern int isTdc1877(int slot);
extern int isTdc1875(int slot);
extern int fb_map();

int topAdc=0;
int bottomAdc=0;
int topTdc=0;
int bottomTdc=0;

#define FIBER_LATENCY_OFFSET 0x4A  /* measured longest fiber length */
#define TI_READOUT TI_READOUT_TS_INT

#include "tiprimary_list.c" /* source required for CODA */

#define MAXSLOTS 26

/* SFI includes are in /site/coda/2.6.2/common/include */
#include "SFI_source.h"
#include "sfi_fb_macros.h"
#include "sfi.h"

#include "usrstrutils.c"
#include "threshold.c"

unsigned long scan_mask;
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

  unsigned int res, laddr;
  int jj, sfi_addr;

  /*****************
   *   TI SETUP
   *****************/

  /* note, tiInit is done in tiprimary_list.c */

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
  res = (unsigned long) sysBusToLocalAdrs(0x39,sfi_addr ,&laddr);
#endif
#ifdef LINUX
  res = (unsigned long) vmeBusToLocalAdrs(0x39,sfi_addr ,&laddr);
#endif
  if (res != 0) {
     printf("Error Initializing SFI res=%d \n",res);
  } else {
     printf("Calling InitSFI() routine with laddr=0x%x.\n",laddr);
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
  unsigned long pedsuppress, csrvalue;
  int stat, kk;

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
       csrvalue = 0x00001900; 
       if (padr == topTdc) csrvalue = 0x00000900;
       if (padr == bottomTdc) csrvalue = 0x00001100;
       printf("TDC slot %d  %d   csr0 0x%x \n",kk,padr,csrvalue);
       fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,1,1);
       fb_fwc_1(0,0,csrvalue,1,1,1,1,0,1,1);
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
  int ii, res, islot, rlen;
  int stat, dCnt, len=0, idata;
  int ev_type = 0;
  static int verbose=0;

  unsigned long datascan, jj, fbres, numBranchdata;
  unsigned long lenb = (((MAX_EVENT_LENGTH>>4)-2)<<2);

  static int ldebug=0;

#ifdef LINUX
  unsigned int *dmaptr;
  static unsigned int *pfbdata;
  unsigned long rb;
  pfbdata = (unsigned int *)dma_dabufp;
#endif



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

  /* test words. */

#ifdef VXWORKS
  *dma_dabufp++ = 0xb0b0b044;
  *dma_dabufp++ = 0xb0b0b055;
  *dma_dabufp++ = 0xb0b0b066;
  *dma_dabufp++ = 0xb0b0b077;
#endif
#ifdef LINUX
  *dma_dabufp++ = LSWAP(0xb0b0b044);
  *dma_dabufp++ = LSWAP(0xb0b0b055);
  *dma_dabufp++ = LSWAP(0xb0b0b066);
  *dma_dabufp++ = LSWAP(0xb0b0b077);
#endif


  numBranchdata=0;

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

        numBranchdata=(dma_dabufp[4]&(0xF0000000))>>28;  
        if(numBranchdata <= 0) {
	  /* problem here, use default */
          numBranchdata = branch_num;
	}


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

  BANKOPEN(5,BT_UI4,0);

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
          dmaptr = (unsigned int *)vmeDmaLocalToVmeAdrs((unsigned int)dma_dabufp);
          res = fb_frdb_1(topAdc,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);      
          rlen = rb>>2;
          if(ldebug) logMsg("multiblock Adc rlen %d  %d  %d   dma_dabufp 0x%x \n",rb,rlen,res,dma_dabufp);
          if(res == 0) dma_dabufp += rlen;
	} else {  /* read ADCs from each slot, the old slow way */
          for (islot=bottomAdc; islot<=topAdc; islot++) {
            if(ldebug) logMsg("Adc read loop, islot %d \n",islot);
            dmaptr = (unsigned int *)vmeDmaLocalToVmeAdrs((unsigned int)dma_dabufp);
	    res = fb_frdb_1(islot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
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
        if (use_multiblock) {
          res = fb_frdb_1(topTdc,0,dmaptr,lenb,&rb,1,1,1,0,0x0a,1,1,1);      
	} else { /* read TDCs from each slot, the old slow way */
          for (islot=bottomTdc; islot<=topTdc; islot++) {
            if(ldebug) logMsg("Tdc read loop, islot %d \n",islot);
            res = fb_frdb_1(islot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
            rlen = rb>>2;
            if(ldebug) logMsg("Tdc rlen %d  %d \n",rb,rlen);
            if (rlen < 0 || rlen > MAX_EVENT_LENGTH) rlen=MAX_EVENT_LENGTH;
            for(ii=0;ii<rlen;ii++) {
               *dma_dabufp++ = LSWAP(pfbdata[ii]);
	    }
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

 
  datascan = 0;
  fbres = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
  if (fbres && verbose) logMsg("fbres = 0x%x scan_mask 0x%x datascan 0x%x\n",fbres,scan_mask,datascan,0,0,0);
  if ((datascan != 0) && (datascan&~scan_mask)) { 
        logMsg("Error: Read data but More data available after readout datascan = 0x%08x fbres = 0x%x\n",datascan,fbres,0,0,0,0);   
  }

  BANKCLOSE;

  EVENTCLOSE;

  tiSetOutputPort(0,0,0,0);

}

void
rocCleanup()
{
}
