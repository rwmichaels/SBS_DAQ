/* VHG. last modified Thu Feb 28 16:21:11 EST 2002  JLAB CODA*/
/* ported to IntelPC, Bob Michaels, May 2017.
   It's assumed that this is run in a readout code
   where vmeOpen* routines have been called already */


#ifdef VXWORKS
#include <vxWorks.h>
#include <sysLib.h>
#include <taskLib.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "jvme.h"

#include "libsfifb.h"


#define SFI_VME_ADDR (char*)0xe00000

#define BOARD_ID_MASK  0xffff0000
#define B1872          0x10480000
#define B1872A         0x10360000
#define B1875          0x10490000
#define B1875A         0x10370000
#define B1877          0x103c0000
#define B1877S         0x103d0000
#define B1881          0x104b0000
#define B1881M         0x104f0000


typedef struct csr_conf
{
  unsigned int csr0_1877;
  unsigned int csr1_1877;
  unsigned int csr18_1877;
  unsigned int csr0_1877S;
  unsigned int csr1_1877S;
  unsigned int csr18_1877S;
  unsigned int csr0_1881;
  unsigned int csr1_1881;
  unsigned int csr0_1872;
} CSR_CONF;
CSR_CONF csrdef;

char *dir_mapparms = "$CLON_PARMS/feconf/rocmap/previous";
char *dir_mapthis  = "$CLON_PARMS/fb_boards";
char *dir_csr      = "$CLON_PARMS/feconf/csrconf";

char map_this[26][10], map_parms[26][10], res[120];

int map_adc[26], map_tdc72[26], map_tdc75[26];
int map_tdc77[26], map_tdc77S[26];
int map_unknown[26], map_empty[26];

int  csr_adc[]    = {5, 0, 1, 3, 7, 16};
int  csr_tdc72[]  = {2, 0, 1}; /*
int  csr_tdc75[]  = {2, 0, 1}; */
int  csr_tdc77[]  = {6, 0, 1, 3, 7, 16, 18}; /*
int  csr_tdc77S[] = {7, 0, 1, 3, 5, 7, 16, 18}; */


/****** Function prototypes ******/
int  fb_diag ();
int  fb_map ();
char *fb_board_stats (int slot, int prn);
int   fb_72_75_bl_read (int slot);
char *fb_72_75_stats (int slot, int csr0, char b_type[10], int prn);
char *fb_77_stats    (int slot, int csr0, char b_type[10], int prn);
char *fb_81_stats    (int slot, int csr0, char b_type[10], int prn);
int  fb_board_csr_read ();

int  fb_diag ()
{
  int  slot;
  char *stats;

  for ( slot=0; slot<26; slot++) {
    if    ( (strcmp(map_this[slot],"Empty")    == 0) || 
	    (strcmp(map_this[slot],"Unknown")  == 0) )
      stats = " -";
    else
      stats = fb_board_stats(slot,1);
}
  return(0);
}

int isAdc1881(int slot) {
  return map_adc[slot];
}

int isTdc1877(int slot) {
  return (map_tdc77[slot] || map_tdc77S[slot]); 
}

int isTdc1875(int slot) {
  return (map_tdc72[slot] || map_tdc75[slot]); 
}



int fb_map () {
  int  slot, i, csr;
  int  val;
  unsigned long ladr;

/****** Initialization of SFI/fastbus ******/
  ladr = 0; /*
#ifdef VXWORKS
  sysBusToLocalAdrs (0x39, (char *)SFI_VME_ADDR, (char**)&ladr);
#else
  vmeBusToLocalAdrs (0x39, (char *)SFI_VME_ADDR, (char**)&ladr);
#endif

  printf("SFI_BaseAddress_local  = %8lX \n", ladr);
  fb_init_1 (ladr); */

/****** Initialization and filling map_this[26][10] ******/
  for (i=0;i<26;i++) strcpy(map_this[i],"");

/****** Loop over all the slots of the Fastbus crate ******/
  for ( slot=0; slot<26; slot++) {
      
/****** Reset the map array elements ******/
      map_adc[slot]     = 0;
      map_tdc72[slot]   = 0;
      map_tdc75[slot]   = 0;
      map_tdc77[slot]   = 0;
      map_tdc77S[slot]  = 0;
      map_unknown[slot] = 0;
      map_empty[slot]   = 0;
      
      if(fpac (slot, 0) != 0 ) {
	map_empty[slot] = 1;
	strcpy(map_this[slot],"Empty");
      }
      fprel();
      
      if(fprc (slot, 0, &csr) == 0) {
	val = csr&BOARD_ID_MASK;
	if ( val == B1881M || val == B1881 ) {
	  map_adc[slot] = 1;
	  strcpy(map_this[slot],"ADC1881");
	} else if ( val == B1872 || val == B1872A ) {
	  map_tdc72[slot] = 1;
	  strcpy(map_this[slot],"TDC1872");
	} else if ( val == B1875 || val == B1875A ) {
	  map_tdc75[slot] = 1;
	  strcpy(map_this[slot],"TDC1875");
	} else if ( val == B1877  ) {
	  map_tdc77[slot] = 1;
	  strcpy(map_this[slot],"TDC1877");
	} else if ( val == B1877S ) {
	  map_tdc77S[slot] = 1;
	  strcpy(map_this[slot],"TDC1877S");
	} else if ( map_empty[slot] == 0 ) {
	  map_unknown[slot] = 1;
	  strcpy(map_this[slot],"Unknown");
	}
      } 
      else {
	sfi_error_decode(3);
	map_empty[slot] = 1;
	strcpy(map_this[slot],"Empty");
      }
      fprel();
    }
  

  printf(\
          "Slot    ADC1881  TDC1872  TDC1875  TDC1877  TDC1877S  Unknown  Empty \n");
  printf(\
	  "-------------------------------------------------------------------- \n");
  for (i=0;i<26;i++) {
    printf("%3d %8d %8d %8d %8d %8d %8d %8d \n", i, \
	    map_adc[i], map_tdc72[i], map_tdc75[i], map_tdc77[i], \
	    map_tdc77S[i], map_unknown[i], map_empty[i]);
  }
  

/****** End of creating hardware map ******/
  return(0);
}


char *fb_board_stats (int slot, int prn) {
  int  csr0, id;
  char b_type[10];

  strcpy(res,"");  /*** return initialization ***/

/****** Check existence of the board ******/
  if (fpac(slot,0) != 0) {
    sfi_error_decode(3);
    sprintf(res, "Error: Slot#%d is empty.", slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }
  fprel();

/****** Check Board ID ******/
  if (fprc(slot,0,&csr0) != 0) {
    sprintf(res, "Error: Slot#%d, can't read CSR0.", slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }

  id  = csr0&BOARD_ID_MASK;
  if ( id == B1881 || id == B1881M ) {
    strcpy(b_type,"ADC1881");
    sprintf(res, "%s", fb_81_stats(slot,csr0,b_type,prn));
  } else if ( id == B1872 || id == B1872A ) {
    strcpy(b_type,"TDC1872");
    sprintf(res, "%s", fb_72_75_stats(slot,csr0,b_type,prn));
  } else if ( id == B1875 || id == B1875A ) {
    strcpy(b_type,"TDC1875");
    sprintf(res, "%s", fb_72_75_stats(slot,csr0,b_type,prn));
  } else if ( id == B1877  ) {
    strcpy(b_type,"TDC1877");
    sprintf(res, "%s", fb_77_stats(slot,csr0,b_type,prn));
  } else if ( id == B1877S ) {
    strcpy(b_type,"TDC1877S");
    sprintf(res, "%s", fb_77_stats(slot,csr0,b_type,prn));
  } else {
    sprintf(res, "Error: Slot#%d, can't define board type.", slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }

  return (res);
}



int fb_72_75_bl_read (int slot)
{
  unsigned long tdc[65];
  unsigned long rslt, dmaptr, lenb, rb, rlen, len=65;
  int ii;
  
  ii=rlen=0;
#ifdef VXWORKS
  if ((rslt = sysLocalToBusAdrs(0x09, (char*)tdc, (char**)&dmaptr)) != 0) {
    printf("\n Error: sysLocalToBusAdrs return = %d \n\n", (int)rslt);
    return(-1);
  }
#endif
  lenb = len<<2;
  rslt = fb_frdb_1(slot,0,dmaptr,lenb,&rb,1,0,1,0,0x0a,0,0,1);
  if ((rb > (lenb+4))||(rslt != 0)) {
    printf("\n Error: Block Read rslt=0x%x maxBytes=%d returnBytes=%d \n\n", \
	   (int)rslt, (int)lenb, (int)rb);
    return(-1);
  } /* else {
    rlen = rb>>2;
    printf("Block Read   rslt=0x%x, maxbytes=%d, returnBytes=%d; \n", \
	   (int)rslt, (int)lenb, (int)rb);
    printf(" %d words ", (int)rlen);
    for(ii=0;ii<rlen;ii++) {
      if ((ii % 5) == 0) printf("\n    ");
      printf("  0x%08x", (int)tdc[ii]);
    }
    printf("\n");
  }
    */
  rslt = fpwc(slot,0,0x800000);    /*** increment Event Counter ***/
  if (rslt != 0){ 
    printf("\n Error: Increment Event Counter \n\n");
    return(-1);
  }
  
  return(0);
}


char *fb_72_75_stats (int slot, int csr0, char b_type[10], int prn)
{
  int  csr0_back, csr1, csr1_II;
  int  n_cec, n_rec, n_cec2, n_rec2;
  char info[6][10], *inff;

  strcpy(res,"");  /*** return initialization ***/

/****** Read & Store CSR1 state ******/
  if (fprc(slot,1,&csr1) != 0) {
    sprintf(res, "Error: %s, slot#%d, can't read CSR1.", b_type, slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }

/****** Check buffer condition, BUSY or NOT ******/
  n_rec = csr1 & 0xf;
  n_cec = (csr1 & 0xf00) >> 8;

  if (fpwc(slot,0,0x31000640) != 0) {   /*** reprogram TDC ***/
    sprintf(res, "Error: %s, slot#%d, can't write to CSR0.", b_type, slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }
  fpwc(slot,0,0x800);    /*** internal COM, one more event -> buffer ***/
  taskDelay(3);
  fprc(slot,1,&csr1_II);
  n_rec2 = csr1_II & 0xf;
  n_cec2 = (csr1_II & 0xf00) >> 8;

  if (n_cec == n_cec2) sprintf(res, "Data buffer is full -> Board is BUSY");
  else if (fb_72_75_bl_read(slot) == -1) sprintf(res, "Error: Block read failed.");
  else sprintf(res, "OK");
  
/****** Back design of CSR0 settings ******/
  csr0_back = 0;
  if ( csr0 & 0x0040 ) {
    csr0_back = csr0_back | (1<<22);
    strcpy(info[0],"Enabled");
  } else {
    csr0_back = csr0_back | (1<< 6);
    strcpy(info[0],"Disabled");
  }
  if ( csr0 & 0x0100 ) {
    csr0_back = csr0_back | (1<< 8);
    strcpy(info[1],"Enabled");
  } else {
    csr0_back = csr0_back | (1<<24);
    strcpy(info[1],"Disabled");
  }
  if ( csr0 & 0x0200 ) {
    csr0_back = csr0_back | (1<< 9);
    strcpy(info[2],"Internal");
  } else {
    csr0_back = csr0_back | (1<<25);
    strcpy(info[2],"External");
  }
  if ( csr0 & 0x0400 ) {
    csr0_back = csr0_back | (1<<10);
    strcpy(info[3],"Enabled");
  } else {
    csr0_back = csr0_back | (1<<26);
    strcpy(info[3],"Disabled");
  }
  if ( csr0 & 0x1000 ) {
    csr0_back = csr0_back | (1<<12);
    strcpy(info[4],"Enabled");
  } else {
    csr0_back = csr0_back | (1<<28);
    strcpy(info[4],"Disabled");
  }
  if ( csr0 & 0x2000 ) {
    csr0_back = csr0_back | (1<<13);
    strcpy(info[5],"High");
  } else {
    csr0_back = csr0_back | (1<<29);
    strcpy(info[5],"Low");
  }

/****** Restore CSR0 settings ******/
  fpwc(slot,0,csr0_back);

/****** Print full decoded status (if prn=1) ******/
  if (prn == 1) {
    printf("\n Slot = %2d,  Board type = %s \n", slot, b_type);
    printf("------------------------------------ \n");
    printf(" CSR0 = 0x%8x \n", csr0);
    printf("    Wait Status          = %s \n", info[0]);
    printf("    CAT COM Status (TR5) = %s \n", info[1]);
    printf("    MPI Status           = %s \n", info[2]);
    printf("    Test Pulser Status   = %s \n", info[3]);
    printf("    Autorange Status     = %s \n", info[4]);
    printf("    Range Status         = %s \n\n", info[5]);
    printf(" CSR1 = 0x%8x \n", csr1);
    printf("    Readout    Event Counter (REC) = %d \n", n_rec);
    printf("    Conversion Event Counter (CEC) = %d \n\n", n_cec);
    printf(" Busy status check gives: %s \n", res);
    printf(" ======================== \n\n");
    inff = "different to";
    if (csr0_back == csrdef.csr0_1872) inff = "consistent with";
    printf(" Defined CSRs settings are %s default \n", inff);
    printf(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
    printf("  CSR0_defined  = 0x%8x  <->  0x%8x = CSR0_default \n\n\n", \
	   csr0_back, csrdef.csr0_1872);
  }

  return (res);
}



char *fb_77_stats (int slot, int csr0, char b_type[10], int prn)
{
  int  csr1, csr18, csr0_t, csr1_t, csr18_t;
  int  csr16, wbuf, rbuf;
  int  bit;
  char *info;

  strcpy(res,"");  /*** return initialization ***/

/****** Read & Store CSR16 state ******/
  if (fprc(slot,16,&csr16) != 0) {
    sprintf(res, "Error: %s, slot#%d, can't read CSR16.", b_type, slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }

/****** Check buffer pointers, BUSY or NOT ******/
  wbuf  =  csr16 & 0xf;
  rbuf  = (csr16 & 0xf00) >> 8;
  if (wbuf == rbuf) sprintf(res, "Pointer WB=RB=%d -> Board is BUSY", wbuf);
  else sprintf(res, "OK");

/****** Print full decoded status (if prn=1) ******/
  if (prn == 1) {
    printf("\n Slot = %2d,  Board type = %s \n", slot, b_type);
    printf("------------------------------------ \n");
    printf(" CSR0  = 0x%8x \n", csr0);

    if ( csr0 & 0x2 ) info = "Enabled";
    else info = "Disabled";
    printf("    Logical Address       = %s \n", info);

    if (strcmp(b_type,"TDC1877S") == 0) {
      if ( csr0 & 0x4 ) info = "Enabled";
      else info = "Disabled";
      printf("    Register Commons      = %s \n", info);
    }

    if ( csr0 & 0x40 ) info = "Enabled";
    else info = "Disabled";
    printf("    Memory Test Mode      = %s \n", info);

    if ( csr0 & 0x100 ) info = "Enabled";
    else info = "Disabled";
    printf("    Priming during LNE    = %s \n", info);

    if (strcmp(b_type,"TDC1877S") == 0) {
      if ( csr0 & 0x200 ) info = "Selected";
      else info = "Not selected";
      printf("    Simple Sparsification = %s \n", info);
    }

    bit = ( csr0 & 0x1800 ) >> 11;
    if      ( bit == 0 ) info = "Bypass";
    else if ( bit == 1 ) info = "Primary Link";
    else if ( bit == 2 ) info = "End Link";
    else if ( bit == 3 ) info = "Middle Link";
    printf("    Multi-Block Configur. = %s \n\n", info);

    if (fprc(slot,1,&csr1) != 0) {      /*** read & store CSR1 state ***/
      printf("\n Error: Can't read CSR1.\n\n");
      return ("Error: Can't read CSR1");
    } printf(" CSR1  = 0x%8x \n", csr1);

    if ( csr1 & 0x1 ) info = "Enabled";
    else info = "Disabled";
    printf("    BIP to TR7       = %s \n", info);

    if ( csr1 & 0x2 ) info = "Enabled";
    else info = "Disabled";
    printf("    COM from TR6     = %s \n", info);

    if ( csr1 & 0x8 ) info = "Enabled";
    else info = "Disabled";
    printf("    FC from TR5      = %s \n", info);
    
    bit = ( csr1 & 0xf000000 ) >> 24;
    if      (bit  <   3) bit = 1024*(bit+1);
    else if (bit == 0xe) bit = 262144;
    else if (bit == 0xf) bit = 524288;
    else                 bit = 1024*(4-(bit-1)%2) << (bit-2)/2;
    printf("    FCW set to       = %d ns \n", bit);

    if ( csr1 & 0x4 ) info = "Enabled";
    else info = "Disabled";
    printf("    FCW from TR5     = %s \n", info);

    if (strcmp(b_type,"TDC1877S") == 0) {
      if ( csr1 & 0x100000 ) info = "Enabled";
      else info = "Disabled";
      printf("    Sparsification   = %s \n", info);
    }

    if ( csr1 & 0x800000 ) {
      info = "Enabled";
      bit = 1 << ((csr1 & 0x30000) >> 16);
      printf("    Test Burst Count = %d \n", bit);
      bit = ( csr1 & 0xc0000 ) >> 18;
      if (bit < 2) bit =  125*(bit+1);
      else         bit = 1000*(bit-1);
      printf("    Test Pulse       = %d ns \n", bit);
    }
    else info = "Disabled";
    printf("    Internal Test    = %s \n", info);

    if ( csr1 & 0x20000000 ) info = "Enabled";
    else info = "Disabled";
    printf("    Trailing Edge    = %s \n", info);

    if ( csr1 & 0x40000000 ) info = "Enabled";
    else info = "Disabled";
    printf("    Leading Edge     = %s \n", info);

    if ( csr1 & 0x80000000 ) {
      info = "Start";
      bit = ( csr1 & 0xf0 ) >> 4;
      if      (bit == 0) printf("    Com.Start Timeout= Front Pnl \n");
      else if (bit  > 9) bit = 32768;
      else               bit = 32 << bit;
      if (bit != 0)
	printf("    Com.Start Timeout= %d ns \n", bit);
    }
    else info = "Stop";
    printf("    Common mode      = %s \n\n", info);

    printf(" CSR16 = 0x%8x \n", csr16);
    printf("    Read  Buffer Pointer = %d \n", rbuf);
    printf("    Write Buffer Pointer = %d \n\n", wbuf);

    if (fprc(slot,18,&csr18) != 0) {     /*** read & store CSR18 state ***/
      printf("\n Error: Can't read CSR18.\n\n");
      return ("Error: Can't read CSR18");
    } printf(" CSR18 = 0x%8x \n", csr18);

    bit = csr18 & 0xf;
    if (bit == 0) bit = 16;
    printf("    LIFO Depth                  = %d \n", bit);

    bit = 8*((csr18 & 0xfff0) >> 4);
    printf("    Full Scale Time Measurement = %d ns \n\n", bit);

    printf(" Busy status check gives: %s \n", res);
    printf(" ======================== \n\n");

    info = "different to";
    if (strcmp(b_type,"TDC1877S") == 0) {
      csr0_t  = csrdef.csr0_1877S;
      csr1_t  = csrdef.csr1_1877S;
      csr18_t = csrdef.csr18_1877S;
      if ( ((csr0&0xffff) == csrdef.csr0_1877S) && \
	   ( csr1         == csrdef.csr1_1877S) && \
	   ( csr18        == csrdef.csr18_1877S) ) info = "consistent with";
    }
    else {
      csr0_t  = csrdef.csr0_1877;
      csr1_t  = csrdef.csr1_1877;
      csr18_t = csrdef.csr18_1877;
      if ( ((csr0&0xffff) == csrdef.csr0_1877) && \
	   ( csr1         == csrdef.csr1_1877) && \
	   ( csr18        == csrdef.csr18_1877) ) info = "consistent with";
    }
    printf(" Defined CSRs settings are %s default \n", info);
    printf(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
    printf("  CSR0_defined  = 0x%8x  <->  0x%8x = CSR0_default \n", \
	   (csr0&0xffff), csr0_t);
    printf("  CSR1_defined  = 0x%8x  <->  0x%8x = CSR1_default \n", \
	   csr1, csr1_t);
    printf("  CSR18_defined = 0x%8x  <->  0x%8x = CSR18_default \n\n\n", \
	   csr18, csr18_t);
  }

  return (res);
}


char *fb_81_stats (int slot, int csr0, char b_type[10], int prn)
{
  int  csr1, csr16, wbuf, rbuf;
  int  bit;
  char *info;

  strcpy(res,"");  /*** return initialization ***/

/****** Read & Store CSR16 state ******/
  if (fprc(slot,16,&csr16) != 0) {
    sprintf(res, "Error: %s, slot#%d, can't read CSR16.", b_type, slot);
    if (prn != 2) printf("\n %s \n\n", res);
    return (res);
  }

/****** Check buffer pointers, BUSY or NOT ******/
  wbuf  =  csr16 & 0x3f;
  rbuf  = (csr16 & 0x3f00) >> 8;
  if (wbuf == rbuf) sprintf(res, "Pointer WB=RB=%d -> Board is BUSY", wbuf);
  else sprintf(res, "OK");

/****** Print full decoded status (if prn=1) ******/
  if (prn == 1) {
    printf("\n Slot = %2d,  Board type = %s \n", slot, b_type);
    printf("------------------------------------ \n");
    printf(" CSR0  = 0x%8x \n", csr0);

    if ( csr0 & 0x2 ) info = "Enabled";
    else info = "Disabled";
    printf("    Logical Address       = %s \n", info);

    if ( csr0 & 0x4 ) info = "Enabled";
    else info = "Disabled";
    printf("    Gate                  = %s \n", info);

    if ( csr0 & 0x40 ) info = "Enabled";
    else info = "Disabled";
    printf("    Memory Test Mode      = %s \n", info);

    if ( csr0 & 0x100 ) info = "Enabled";
    else info = "Disabled";
    printf("    Priming on LNE        = %s \n", info);

    bit = ( csr0 & 0x1800 ) >> 11;
    if      ( bit == 0 ) info = "Bypass";
    else if ( bit == 1 ) info = "Primary Link";
    else if ( bit == 2 ) info = "End Link";
    else if ( bit == 3 ) info = "Middle Link";
    printf("    Multi-Block Configur. = %s \n\n", info);

    if (fprc(slot,1,&csr1) != 0) {      /*** read & store CSR1 state ***/
      printf("\n Error: Can't read CSR1.\n\n");
      return ("Error: Can't read CSR1");
    } printf(" CSR1  = 0x%8x \n", csr1);

    if ( csr1 & 0x1 ) info = "Enabled";
    else info = "Disabled";
    printf("    CIP to TR7      = %s \n", info);

    if ( csr1 & 0x2 ) info = "Enabled";
    else info = "Disabled";
    printf("    Gate from TR6   = %s \n", info);

    if ( csr1 & 0x8 ) info = "Enabled";
    else info = "Disabled";
    printf("    FC from TR5     = %s \n", info);

    bit = 2048*(((csr1 & 0xf000000) >> 24) +1);
    printf("    FCW set to      = %d ns \n", bit);

    if ( csr1 & 0x4 ) info = "Enabled";
    else info = "Disabled";
    printf("    FCW from TR5    = %s \n", info);

    if ( csr1 & 0x10 ) info = "Enabled";
    else info = "Disabled";
    printf("    Overrun Detect  = %s \n", info);

    bit = ( csr1 & 0xc0 ) >> 6;
    if      ( bit == 1 ) info = "13 bit";
    else if ( bit == 2 ) info = "12 bit";
    else info = "Unpredictable";
    printf("    Conversion Mode = %s \n", info);

    if ( csr1 & 0x20000000 ) info = "Enabled";
    else info = "Disabled";
    printf("    Internal Tester = %s \n", info);

    if ( csr1 & 0x40000000 ) info = "Enabled";
    else info = "Disabled";
    printf("    Sparsification  = %s \n\n", info);

    printf(" CSR16 = 0x%8x \n", csr16);
    printf("    Read  Buffer Pointer = %d \n", rbuf);
    printf("    Write Buffer Pointer = %d \n\n", wbuf);

    printf(" Busy status check gives: %s \n", res);
    printf(" ======================== \n\n");

    info = "different to";
    if ( ((csr0&0xffff) == csrdef.csr0_1881) && \
	 ( csr1         == csrdef.csr1_1881) ) info = "consistent with";
    printf(" Defined CSRs settings are %s default \n", info);
    printf(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
    printf("  CSR0_defined  = 0x%8x  <->  0x%8x = CSR0_default \n", \
	   (csr0&0xffff), csrdef.csr0_1881);
    printf("  CSR1_defined  = 0x%8x  <->  0x%8x = CSR1_default \n\n\n", \
	   csr1, csrdef.csr1_1881);
  }

  return (res);
}




int  fb_board_csr_read ()
{
  int  slot, csr_tmp;
  int  i_csr, *csr;
  
  for ( slot=0; slot<26; slot++) {
    if ( ( strcmp(map_this[slot],"Empty")   != 0) && 
	 ( strcmp(map_this[slot],"Unknown") != 0) ) { 
      if        (strcmp(map_this[slot],"ADC1881")  == 0)   csr=csr_adc;
      else if ( (strcmp(map_this[slot],"TDC1872")  == 0) ||
		(strcmp(map_this[slot],"TDC1875")  == 0) ) csr=csr_tdc72;
      else if ( (strcmp(map_this[slot],"TDC1877")  == 0) ||
		(strcmp(map_this[slot],"TDC1877S") == 0) ) csr=csr_tdc77;
      else  {
	printf("\n Error: Slot type is corrupted.\n\n");
	return (-1);
      }

      printf("\n Slot = %2d,  Board type = %s \n", slot, map_this[slot]);
      for ( i_csr=1; i_csr<=csr[0]; i_csr++) {
	if ( fprc(slot, csr[i_csr], &csr_tmp) != 0) {
	  printf("\n Error: Can't read CSR%d for %s in slot %d.\n\n", \
		 csr[i_csr], map_this[slot], slot);
	  return (-1);
	}
	printf("  CSR%2d = 0x%8x\n", csr[i_csr], csr_tmp);
      }
    }
  }
  return(0);
}

