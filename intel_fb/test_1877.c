/**************************************************** 
 * Test code for SFI reading out a Lecroy 1877 TDC  *
 *                                                  *
 *    Written By : David Abbott                     *
 *                 Data Acquisition Group           *
 *                 Jefferson Lab                    *
 *    Date: 24-JUN-96                               *
 *    Ported to Linux By : Bryan Moffit             *
 *    Date: September 2011                          *
 ****************************************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "libsfifb.h"


#define MOD_ID1 0x103c0000
#define MOD_ID2 0x103d0000
/* #define MAX_LENGTH 1537 */
#define MAX_LENGTH 200

#define BUFFER_SIZE 1024*1024*3
#define NBUFFER     1

extern void *a32slave_window;
extern DMANODE *the_event;
extern unsigned int *dma_dabufp;

DMA_MEM_ID vmeIN,vmeOUT;

int 
main(int argc, char *argv[]) {

  int stat;
  unsigned int datascan, scanmask, *dmaptr=0;;
  unsigned int lenb, len=MAX_LENGTH, rlen;
  unsigned int rb;
  unsigned int *tdc;
  unsigned int addr=0x18000000;  /* VME Slave address of the controller */
  unsigned int slot=21;

  if(argc>1) {
    slot=atoi(argv[1]);
  }

  printf("\n1877 test, slot %d \n",slot);
  printf("----------------------------\n");

  stat = vmeOpenDefaultWindows();
  vmeSetDebugFlags(0xffffffff);
  stat = vmeOpenSlaveA32(addr,0x00400000);
  if(stat != OK)
    {
      printf("%s: ERROR: failed to open slave window (%d)\n",
	     __FUNCTION__,stat);
      /* FIXME  return ERROR; */
    }

  dmaPUseSlaveWindow(1);
  
  /* INIT dmaPList */
  dmaPFreeAll();
  vmeIN  = dmaPCreate("vmeIN",BUFFER_SIZE,NBUFFER,0);
  vmeOUT = dmaPCreate("vmeOUT",0,0,0);
  
  dmaPStatsAll();
  
  dmaPReInitAll();

  
  GETEVENT(vmeIN,0); 
  tdc = (unsigned int *)dma_dabufp;
 
  dmaptr = (unsigned int *)vmeDmaLocalToVmeAdrs((unsigned int)dma_dabufp);

  InitSFI(0xe00000);
  InitFastbus(0x20,0x33);

  scanmask = (1<<slot);
  unsigned int res, moduleID=0;
  /****** Check Board ID  *********/
  res = fprc(slot,0,&moduleID);
  if (res != 0){ 
    printf("ERROR: Read Module ID, res=0x%x\n",(unsigned int)res);
    sfi_error_decode(2);
    goto CLOSE;
  } else {
    switch (moduleID&0xffff0000) {
    case MOD_ID1:
    case MOD_ID2:
      printf("moduleID = 0x%08x\n",(unsigned int)moduleID);
      break;
    default:
      printf("ERROR: Read Invalid Module ID 0x%x\n",(unsigned int)moduleID);
      goto CLOSE;
    }
  }

  /****** Reset/Clear TDC *********/
  res = fpwc(slot,0,0x40000000);
  if (res != 0){ 
    printf("ERROR: Reset TDC\n");
    goto fooy;
  }

  /****** Program TDC *******/
  /* Enable leading/Trailing edge
     Common Stop
     Single test pulse (125ns)
  */
  res = fpwc(slot,1,0x60800000);
  if (res != 0){ 
    printf("ERROR: Program TDC CSR 1\n");
    goto fooy;
  }
  /* 32 Microsec window, 16 hit lifo */
  res = fpwc(slot,18,0xfff0);
  if (res != 0){ 
    printf("ERROR: Program TDC CSR 18\n");
    goto fooy;
  }

/****** Read TDC Module ID *********/
  res = fprc(slot,0,&moduleID);
  if (res != 0){ 
    printf("ERROR: Read TDC ID\n");
    goto fooy;
  } else {
    printf("Module ID = 0x%x\n",moduleID);
  }


  int iz,ii, count=1;
 /****** Readout Loop *******/
  for(iz=0;iz<count;iz++) {

    res = fpwc(slot,0,0x80);
    if (res != 0){ 
      printf("ERROR: Test Stobe TDC\n");
      goto fooy;
    }
    res = fpwc(slot,0,0x800000);
    if (res != 0){ 
      printf("ERROR: Test Common TDC\n");
      goto fooy;
    }

    ii=0;
    datascan = 0;
    while ((ii<20) & ((datascan&scanmask) != scanmask)) {
      res = fprcm(9,0,&datascan);
      if (res != 0){ 
	printf("ERROR: Sparse Data Scan (res=0x%x)\n",(unsigned int)res);
	goto fooy;
      }
      ii++;
    }
 
    if (ii<20) {
      res = fpwc(slot,0,0x400);
      if (res != 0){ 
	printf("ERROR: Load Next Event\n");
	goto fooy;
      }
    
      printf("pause before blk transfer\n");
      getchar();

      lenb = len<<2;
      printf("just before\n");
      res = fb_frdb_1(slot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
      if ((rb > (lenb+4))||(res != 0)) {
	printf("ERROR: Block Read   res = 0x%x maxbytes = %d returnBytes = %d \n",
	       (unsigned int)res,lenb,(int)rb);
	goto fooy;
      }else{
	rlen = rb>>2;
	printf("pause before print data\n");
	getchar();
	printf("DATA %d: %d words, rb = %d",(iz+1),rlen,rb);
	for(ii=0;ii<rlen;ii++) {
	  if ((ii % 4) == 0) printf("\n    ");
	  printf("  0x%08x",LSWAP(tdc[ii]));
	}
	printf("\n");
      }

    } else {
      printf("Sparse Data scan indicates no Conversion after %d tries\n",ii);
    }


    taskDelay(30); /* wait a little before next trigger */

  } /* end of for(iz=0.... */

  printf("Done with %d loops(s)\n",iz);
   
 CLOSE:

  vmeCloseA32Slave();
  vmeCloseDefaultWindows();

  exit(0);

 fooy:
  sfi_error_decode(1);
  vmeCloseDefaultWindows();

  exit(0);
}

