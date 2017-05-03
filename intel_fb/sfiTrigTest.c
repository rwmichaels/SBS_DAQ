/*
 * File:
 *    sfiLibTest.c
 *
 * Description:
 *    Test SFI TI interrupts with GEFANUC Linux Driver
 *    and TIR library
 *
 *
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jvme.h"
#include "libsfifb.h"

/* access tirLib global variables */
extern unsigned int sfiIntCount;
extern unsigned int sfiDoAck;
extern unsigned int sfiNeedAck;
/* Interrupt Service routine */
void
mytirISR(int arg)
{
  volatile unsigned short reg;
  

  if(sfiIntCount%100==0)
    printf("Received %6d triggers\n",sfiIntCount);

  if(sfiIntCount==2000)
    {
      sfiDoAck=0;
      sfiNeedAck=1;
    }

}


int 
main(int argc, char *argv[]) {

    int stat;

    printf("\nJLAB SFI Trigger Interface Tests (0x%x)\n",SSWAP(0x1234));
    printf("----------------------------\n");

    vmeOpenDefaultWindows();

    InitSFI(0xe00000);
    InitFastbus(0x20,0x33);

/*     gefVmeSetDebugFlags(vmeHdl,0x0); */
    sfiTrigInit(SFI_EXT_POLL);

    stat = sfiTrigIntConnect(0, mytirISR, 0);
    if (stat != OK) {
      printf("ERROR: sfiTrigIntConnect failed \n");
      goto CLOSE;
    } else {
      printf("INFO: Attached SFI TI Interrupt\n");
    }

    sfiTrigStatus(1);

    printf("Hit any key to enable Triggers...\n");
    getchar();

    /* Enable the TI and clear the trigger counter */
    sfiTrigEnable(1);


    printf("Hit any key to Disable TI and exit.\n");
    getchar();

    sfiTrigDisable();

    sfiNeedAck=0;
/*     getchar(); */

    sfiTrigIntDisconnect();


 CLOSE:
    
/*     printf("%s: sfiDoAck=0... sfIntCount = %d\n",__FUNCTION__,sfiIntCount); */
/*     sfiTrigAck(); */


    printf("%s: sfiDoAck=0... sfIntCount = %d\n",__FUNCTION__,sfiIntCount);

    sfiTrigStatus(1);


    vmeCloseDefaultWindows();

    exit(0);
}

