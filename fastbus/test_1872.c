/*************************************************************** 
 * Test code for SFI reading out a Lecroy 1872A or 1875A TDC   *
 *                                                             *
 *    Written By : David Abbott                                *
 *                 Data Acquisition Group                      *
 *                 Jefferson Lab                               *
 *    Date: 24-JUN-96                                          *
 *                                                             *
 *    Inputs:  slot  -  Fastbus slot number (0-25)             *
 *             count -  number of Block Read                   *
 *                      cycles to execute                      * 
 ***************************************************************/

#include <vxWorks.h>
#include <stdio.h>
#include <sysLib.h>
#include <taskLib.h>


#define MAX_LENGTH 65
#define SFI_VME_ADDR 0xe00000
#define MOD_ID1 0x10360000
#define MOD_ID2 0x10370000


extern unsigned long fpwc();
extern unsigned long fprc();
extern unsigned long fprcm();
extern unsigned long fb_frdb_1();


void 
test_1872(slot, count)
     int slot;
     long count;
{

  unsigned long datascan, scanmask, ii, iz, len;
  unsigned long tdc[MAX_LENGTH];
  unsigned long res, moduleID;
  unsigned long ladr, dmaptr, lenb, rb, rlen;
    

/* Initialize SFI interface and other variables. Get local address *
 * from which to access the SFI (A24/D32 VME slave AM = 0x39       */
  res = (unsigned long) sysBusToLocalAdrs(0x39,SFI_VME_ADDR,&ladr);
  if(res != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    InitSFI(ladr);
    InitFastbus(0x20,0x33);
  }

  scanmask = (1<<slot);
  len = MAX_LENGTH;
  if (count <= 0) count= 1;

  /* SFI uses VME AM = 0x0b or 0x09 (Non-privledged  Data transfer)     *
   * to transfer fastbus data to local CPU memory                  */
  if ((res = sysLocalToBusAdrs(0x09,tdc,&dmaptr)) != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    printf("Block Read data stored at address 0x%x ,dmaptr = 0x%x\n",&tdc[0],dmaptr);
  }

/****** Check Board ID  *********/
  res = fprc(slot,0,&moduleID);
  if (res != 0){ 
    printf("ERROR: Read Module ID\n");
    goto fooy;
  } else {
    switch (moduleID&0xffff0000) {
    case MOD_ID1:
    case MOD_ID2:
      break;
    default:
      printf("ERROR: Read Invalid Module ID 0x%x\n",moduleID);
      goto fooy;
    }
  }
   
/****** Reset/Clear TDC *********/
  res = fpwc(slot,0,0x08000000);
  if (res != 0){ 
    printf("ERROR: Reset TDC\n");
    goto fooy;
  }
  res = fpwc(slot,0,0x40000000);
  if (res != 0){ 
    printf("ERROR: Clear TDC\n");
    goto fooy;
  }

/****** Program TDC *******/
  res = fpwc(slot,0,0x31000640);
  if (res != 0){ 
    printf("ERROR: Program TDC CSR 0\n");
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
  printf("Executing Readout loop %d times...\n",count);
  for(iz=0;iz<count;iz++) {

    res = fpwc(slot,0,0x800);
    if (res != 0){ 
      printf("ERROR: Test Common TDC\n");
      goto fooy;
    }

    ii=0;
    datascan = 0;
    while ((ii<50) && ((datascan&scanmask) != scanmask)) {
      res = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
      if (res != 0){ 
	printf("ERROR: Sparse Data Scan\n");
	goto fooy;
      }
      ii++;
    }
 
    if (ii<50) {
    
      lenb = len<<2;
      res = fb_frdb_1(slot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
      if ((rb > (lenb+4))||(res != 0)) {
	printf("ERROR: Block Read   res = 0x%x maxbytes = %d returnBytes = %d \n",res,lenb,rb);
	goto fooy;
      } else {
	rlen = rb>>2;
	printf("DATA %d: %d words ",(iz+1),rlen);
	for(ii=0;ii<rlen;ii++) {
	  if ((ii % 5) == 0) printf("\n    ");
	  printf("  0x%08x",tdc[ii]);
	}
	printf("\n");
      }

      res = fpwc(slot,0,0x800000);
      if (res != 0){ 
	printf("ERROR: Increment Event Counter\n");
	goto fooy;
      }
    } else {
      printf("Sparse Data scan indicates no Conversion after %d tries\n",ii);
    }

    res = fpwc(slot,0,0x400);
    if (res != 0){ 
      printf("ERROR: Enable Test Pulser \n");
      goto fooy;
    }
    
    taskDelay(30); /* wait a little before next trigger */

  } /* End for(iz=0....) */

  printf("Done with %d loop(s)\n",iz);
  return;

 fooy:
  sfi_error_decode();
  return;

}

int stop1872 = 0;

void 
check_1872( int slot, int active_depth)
{

  unsigned long datascan, scanmask, ii, len;
  unsigned long tdc[MAX_LENGTH];
  unsigned long res, moduleID, tdcchan, tdcval;
  unsigned long ladr, dmaptr, lenb, rb, rlen;
    

/* Initialize SFI interface and other variables. Get local address *
 * from which to access the SFI (A24/D32 VME slave AM = 0x39       */
  res = (unsigned long) sysBusToLocalAdrs(0x39,SFI_VME_ADDR,&ladr);
  if(res != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    InitSFI(ladr);
    InitFastbus(0x20,0x33);
  }

  scanmask = (1<<slot);
  len = MAX_LENGTH;

  /* SFI uses VME AM = 0x0b or 0x09 (Non-privledged  Data transfer)     *
   * to transfer fastbus data to local CPU memory                  */
  if ((res = sysLocalToBusAdrs(0x09,tdc,&dmaptr)) != 0) {
    printf("Error Initializing SFI res=%d\n",res);
    return;
  } else {
    printf("Block Read data stored at address 0x%x ,dmaptr = 0x%x\n",&tdc[0],dmaptr);
  }

/****** Check Board ID  *********/
  res = fprc(slot,0,&moduleID);
  if (res != 0){ 
    printf("ERROR: Read Module ID\n");
    goto fooy;
  } else {
    switch (moduleID&0xffff0000) {
    case MOD_ID1:
    case MOD_ID2:
      break;
    default:
      printf("ERROR: Read Invalid Module ID 0x%x\n",moduleID);
      goto fooy;
    }
  }
   
/****** Reset/Clear TDC *********/
  res = fpwc(slot,0,0x08000000);
  if (res != 0){ 
    printf("ERROR: Reset TDC\n");
    goto fooy;
  }
  res = fpwc(slot,0,0x40000000);
  if (res != 0){ 
    printf("ERROR: Clear TDC\n");
    goto fooy;
  }

/****** Program TDC *******/
  res = fpwc(slot,0,0x35000240);
  if (res != 0){ 
    printf("ERROR: Program TDC CSR 0\n");
    goto fooy;
  }

  if((active_depth > 0)&&(active_depth <= 0x40)) {
    res = fpwc(slot,1,(active_depth<<24));
    if (res != 0){ 
      printf("ERROR: Program TDC CSR 1\n");
      goto fooy;
    }
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
  printf(" Polling for Gates...\n");
  do {

    datascan = 0;
    res = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
    if (res != 0){ 
      printf("ERROR: Sparse Data Scan\n");
      goto fooy;
    }
 
    if ((datascan&scanmask) == scanmask) {
    
      lenb = len<<2;
      res = fb_frdb_1(slot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
      if ((rb > (lenb+4))||(res != 0)) {
	printf("ERROR: Block Read   res = 0x%x maxbytes = %d returnBytes = %d \n",res,lenb,rb);
	goto fooy;
      } else {
	rlen = rb>>2;
	printf("TDC DATA: %d words ",rlen);
	for(ii=0;ii<rlen;ii++) {
	  if ((ii % 4) == 0) printf("\n    ");
	  tdcchan = (tdc[ii]&0x3f0000)>>16;
	  tdcval  = (tdc[ii]&0xfff);
	  printf("  Ch %d: %04d (0x%08x)",tdcchan,tdcval,tdc[ii]);
	}
	printf("\n");
      }

      res = fpwc(slot,0,0x800000);
      if (res != 0){ 
	printf("ERROR: Increment Event Counter\n");
	goto fooy;
      }
    }

    taskDelay(60); /* wait a little before checking again */

  } while (stop1872 == 0);

  printf("Exiting polling loop\n");
  stop1872=0;
  return;

 fooy:
  sfi_error_decode();
  return;


}
