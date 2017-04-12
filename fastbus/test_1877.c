/**************************************************** 
 * Test code for SFI reading out a Lecroy 1877 TDC  *
 *                                                  *
 *    Written By : David Abbott                     *
 *                 Data Acquisition Group           *
 *                 Jefferson Lab                    *
 *    Date: 24-JUN-96                               *
 ****************************************************/

#include <vxWorks.h>
#include <stdio.h>
#include <sysLib.h>
#include <taskLib.h>


#define MAX_CHAN  128
#define MAX_LENGTH 1537
#define SFI_VME_ADDR 0xe00000
#define MOD_ID1 0x103c0000
#define MOD_ID2 0x103d0000

extern STATUS sysBusToLocalAdrs();
extern STATUS sysLocalToBusAdrs();

extern unsigned long fpwc();
extern unsigned long fprc();
extern unsigned long fprcm();
extern unsigned long fb_frdb_1();


void 
test_1877(slot, count, tflag)
     int slot;
     unsigned int count;
     int tflag;
{

  unsigned int datascan, scanmask, ii, iz, len;
  unsigned int tdc[MAX_LENGTH], hits[MAX_CHAN];
  unsigned int res, moduleID, chan;
  unsigned int ladr, dmaptr, lenb, rb, rlen;
  float rate;
    

/* Initialize SFI interface and other variables */
  res = (unsigned long) sysBusToLocalAdrs(0x39,(char *)SFI_VME_ADDR,(char **)&ladr);
  if(res != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    InitSFI(ladr);
    InitFastbus(0x20,0x33);
  }

  scanmask = (1<<slot);
  len = MAX_LENGTH;
  if (count <=0) count= 1;

  for(ii=0;ii<MAX_CHAN;ii++) {
    hits[ii] = 0;
  }

  if ((res = sysLocalToBusAdrs(0x09,(char *)tdc,(char **)&dmaptr)) != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    printf("Block Read data stored at address 0x%x dmaptr = 0x%x\n",(int) &tdc[0],dmaptr);
  }
   
/****** Check Board ID  *********/
  res = fprc(slot,0,&moduleID);
  if (res != 0){ 
    printf("ERROR: Read Module ID\n");
    goto fooy;
  } else {
    switch (moduleID&0xffff0000) {
    case MOD_ID1:
      break;
    case MOD_ID2:
      break;
    default:
      printf("ERROR: Read Invalid Module ID 0x%x\n",moduleID);
      goto fooy;
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

  if(tflag) {
    res = fpwc(slot,1,0x40000000);
  } else {
    res = fpwc(slot,1,0x60800000);
  }
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
	printf("ERROR: Sparse Data Scan\n");
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
    
      lenb = len<<2;
      res = fb_frdb_1(slot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
      if ((rb > (lenb+4))||(res != 0)) {
	printf("ERROR: Block Read   res = 0x%x maxbytes = %d returnBytes = %d \n",res,lenb,rb);
	goto fooy;
      }else{
	rlen = rb>>2;
	if(tflag) {
	  for(ii=1;ii<rlen;ii++) {  /* skip over header */
	    chan = (tdc[ii]&0x00fe0000)>>17;
	    hits[chan] += 1;
	  }
	}else{
	  printf("DATA %d: %d words",(iz+1),rlen);
	  for(ii=0;ii<rlen;ii++) {
	    if ((ii % 8) == 0) printf("\n    ");
	    printf("  0x%08x",tdc[ii]);
	  }
	  printf("\n");
	}
      }

    } else {
      printf("Sparse Data scan indicates no Conversion after %d tries\n",ii);
    }


    if(tflag) {
      taskDelay(1); /* wait a little before next trigger */
    }else{
      taskDelay(30); /* wait a little before next trigger */
    }

  } /* end of for(iz=0.... */

  printf("Done with %d loops(s)\n\n",iz);
  if(tflag) {
     printf("HIT DATA: %d triggers (rate estimate in kHz: for 0 hits value is an upper limit)",count);
     for(ii=0;ii<96;ii++) {
       if (hits[ii] > (count*15)) {
	 rate = 500.00;
       }else if (hits[ii] > 0) {
	 rate = (float) hits[ii] / (count*0.032);
       }else {
	 rate = 31.25/(float) count;
       }
       if ((ii % 4) == 0) printf("\n    ");       
       printf(" %2d: %6d (%6.2f)  ",ii,hits[ii],rate);
     }
     printf("\n");
  }
  return;

 fooy:
   sfi_error_decode();
   return;

}


