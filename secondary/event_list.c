#define ROL_NAME__ "ROL2"
#define MAX_EVENT_LENGTH 1024
#define MAX_EVENT_POOL   100
/* POLLING_MODE */
#define POLLING___
#define POLLING_MODE
#define EVENT_MODE 
#define INIT_NAME event_list__init
#include "myrol.h"
#include <EVENT_source.h>
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
#ifdef POLLING___
   rol->poll = 1;
#endif
    *(rol->async_roc) = 0; /* Normal ROC */
  {  /* begin user */
    daLogMsg("INFO","User Download 2 Executed");

  }  /* end user */
} /*end download */     

static void __prestart()
{
CTRIGINIT;
    *(rol->nevents) = 0;
  {  /* begin user */
unsigned long jj, adc_id;
    daLogMsg("INFO","Entering User Prestart 2");

    EVENT_INIT;
    CTRIGRSS(EVENT,1,davetrig,davetrig_done);
    CRTTYPE(1,EVENT,1);
  rol->poll = 1;
    daLogMsg("INFO","User Prestart 2 executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
    *(rol->nevents) = 0;
    rol->recNb = 0;
} /*end prestart */     

static void __end()
{
  {  /* begin user */
    daLogMsg("INFO","User End 2 Executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  {  /* begin user */
    daLogMsg("INFO","User Pause 2 Executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /*end pause */
static void __go()
{

  {  /* begin user */
    daLogMsg("INFO","Entering User Go 2");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void davetrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
    int ii,kk;
    long jj;
    EVENT_GET; 
{/* inline c-code */
 
   if (rol->dabufp != NULL) {          /* Output Pointer should be set by CODA 2.1 ROC */
     kk=0;
 /* this will truncate the event, but seems to be necessary */
     if(EVENT_LENGTH>MAX_EVENT_LENGTH) {
        logMsg("Event size %d too big, max is %d.  Truncating!\n",EVENT_LENGTH,MAX_EVENT_LENGTH);
        EVENT_LENGTH=MAX_EVENT_LENGTH;
     }  
     for (ii=-2;ii<EVENT_LENGTH;ii++) { /* Copy event, including Header from Input to Output */
       jj=rol->dabufp;
       jj = INPUT[ii];
       *rol->dabufp++ = INPUT[ii];  /* (0xb0b00000 + kk) ; kk++;  test */
     }

 }else{
   printf("ROL2: ERROR rol->dabufp is NULL -- Event lost\n");
 }
 
 }/*end inline c-code */

  }  /* end user */
} /*end trigger */

void davetrig_done()
{
  {  /* begin user */
  }  /* end user */
} /*end done */

void __done()
{
poolEmpty = 0; /* global Done, Buffers have been freed */
  {  /* begin user */
  }  /* end user */
} /*end done */

static void __status()
{
  {  /* begin user */
  }  /* end user */
} /* end status */

