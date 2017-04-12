/* globals */

  int trig_addr=0x080000;  


/* # 1 "sfi_gen.c" */
/* # 1 "/site/coda/2.6.2/common/include/rol.h" 1 */
typedef void (*VOIDFUNCPTR) ();
typedef int (*FUNCPTR) ();
typedef unsigned long time_t;
typedef struct semaphore *SEM_ID;
static void __download ();
static void __prestart ();
static void __end ();
static void __pause ();
static void __go ();
static void __done ();
static void __status ();
static int theIntHandler ();
/* # 1 "/site/coda/2.6.2/common/include/libpart.h" 1 */
/* # 1 "/site/coda/2.6.2/common/include/mempart.h" 1 */
typedef struct danode			       
{
  struct danode         *n;	               
  struct danode         *p;	               
  struct rol_mem_part   *part;	                   
  int                    fd;		       
  char                  *current;	       
  unsigned long          left;	               
  unsigned long          type;                 
  unsigned long          source;               
  void                   (*reader)();          
  long                   nevent;               
  unsigned long          length;	       
  unsigned long          data[1];	       
} DANODE;
typedef struct alist			       
{
  DANODE        *f;		               
  DANODE        *l;		               
  int            c;			       
  int            to;
  void          (*add_cmd)(struct alist *li);      
  void          *clientData;                 
} DALIST;
typedef struct rol_mem_part *ROL_MEM_ID;
typedef struct rol_mem_part{
    DANODE	 node;		 
    DALIST	 list;		 
    char	 name[40];	 
    void         (*free_cmd)();  
    void         *clientData;     
    int		 size;		 
    int		 incr;		 
    int		 total;		 
    long         part[1];	 
} ROL_MEM_PART;
/* # 62 "/site/coda/2.6.2/common/include/mempart.h" */
/* # 79 "/site/coda/2.6.2/common/include/mempart.h" */
/* # 106 "/site/coda/2.6.2/common/include/mempart.h" */
extern void partFreeAll();  
extern ROL_MEM_ID partCreate (char *name, int size, int c, int incr);
/* # 21 "/site/coda/2.6.2/common/include/libpart.h" 2 */
/* # 67 "/site/coda/2.6.2/common/include/rol.h" 2 */
/* # 1 "/site/coda/2.6.2/common/include/rolInt.h" 1 */
typedef struct rolParameters *rolParam;
typedef struct rolParameters
  {
    char          *name;	      
    char          tclName[20];	       
    char          *listName;	      
    int            runType;	      
    int            runNumber;	      
    VOIDFUNCPTR    rol_code;	      
    int            daproc;	      
    void          *id;		      
    int            nounload;	      
    int            inited;	      
    long          *dabufp;	      
    long          *dabufpi;	      
    ROL_MEM_PART  *pool;              
    ROL_MEM_PART  *output;	      
    ROL_MEM_PART  *input;             
    ROL_MEM_PART  *dispatch;          
    volatile ROL_MEM_PART  *dispQ;    
    unsigned long  recNb;	      
    unsigned long *nevents;           
    int           *async_roc;         
    char          *usrString;	      
    void          *private;	      
    int            pid;               
    int            poll;              
    int primary;		      
    int doDone;			      
  } ROLPARAMS;
/* # 70 "/site/coda/2.6.2/common/include/rol.h" 2 */
extern ROLPARAMS rolP;
static rolParam rol;
extern int global_env[];
extern long global_env_depth;
extern char *global_routine[100];
extern long data_tx_mode;
extern int cacheInvalidate();
extern int cacheFlush();
static int syncFlag;
static int lateFail;
/* # 1 "/site/coda/2.6.2/common/include/BankTools.h" 1 */
static int EVENT_type;
long *StartOfEvent[32 ],event_depth__, *StartOfUEvent;
/* # 78 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 95 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 107 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 120 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 152 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 186 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 198 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 217 "/site/coda/2.6.2/common/include/BankTools.h" */
/* # 92 "/site/coda/2.6.2/common/include/rol.h" 2 */
/* # 1 "/site/coda/2.6.2/common/include/trigger_dispatch.h" 1 */
static unsigned char dispatch_busy; 
static int intLockKey,trigId;
static int poolEmpty;
static unsigned long theEvMask, currEvMask, currType, evMasks[16 ];
static VOIDFUNCPTR wrapperGenerator;
static FUNCPTR trigRtns[32 ], syncTRtns[32 ], doneRtns[32 ], ttypeRtns[32 ];
static unsigned long Tcode[32 ];
static DANODE *__the_event__, *input_event__, *__user_event__;
/* # 50 "/site/coda/2.6.2/common/include/trigger_dispatch.h" */
/* # 141 "/site/coda/2.6.2/common/include/trigger_dispatch.h" */
static void cdodispatch()
{
  unsigned long theType,theSource;
  int ix, go_on;
  DANODE *theNode;
  dispatch_busy = 1;
  go_on = 1;
  while ((rol->dispQ->list.c) && (go_on)) {
{ ( theNode ) = 0; if (( &rol->dispQ->list )->c){ ( &rol->dispQ->list )->c--; ( theNode ) = ( 
&rol->dispQ->list )->f; ( &rol->dispQ->list )->f = ( &rol->dispQ->list )->f->n; }; if (!( &rol->dispQ->list 
)->c) { ( &rol->dispQ->list )->l = 0; }} ;     theType = theNode->type;
    theSource = theNode->source;
    if (theEvMask) { 
      if ((theEvMask & (1<<theSource)) && (theType == currType)) {
	theEvMask = theEvMask & ~(1<<theSource);
	intUnlock(intLockKey); ;
	(*theNode->reader)(theType, Tcode[theSource]);
	intLockKey = intLock(); ;
	if (theNode)
	if (!theEvMask) {
	 if (wrapperGenerator) {event_depth__--; *StartOfEvent[event_depth__] = (long) (((char *) 
(rol->dabufp)) - ((char *) StartOfEvent[event_depth__]));	if ((*StartOfEvent[event_depth__] & 1) != 0) { 
(rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfEvent[event_depth__] += 1; }; if 
((*StartOfEvent[event_depth__] & 2) !=0) { *StartOfEvent[event_depth__] = *StartOfEvent[event_depth__] + 2; (rol->dabufp) = 
((long *)((short *) (rol->dabufp))+1);; };	*StartOfEvent[event_depth__] = ( 
(*StartOfEvent[event_depth__]) >> 2) - 1;}; ;	 	 rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ; 	}
      } else {
	{if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ; 	go_on = 0;
      }
    } else { 
      if ((1<<theSource) & evMasks[theType]) {
	currEvMask = theEvMask = evMasks[theType];
	currType = theType;
      } else {
        currEvMask = (1<<theSource);
      }
      if (wrapperGenerator) {
	(*wrapperGenerator)(theType);
      }
      (*(rol->nevents))++;
      intUnlock(intLockKey); ;
      (*theNode->reader)(theType, Tcode[theSource]);
      intLockKey = intLock(); ;
      if (theNode)
	{ if (( theNode )->part == 0) { free( theNode ); theNode = 0; } else { {if(! ( & theNode ->part->list 
)->c ){( & theNode ->part->list )->f = ( & theNode ->part->list )->l = ( theNode );( theNode )->p = 0;} 
else {( theNode )->p = ( & theNode ->part->list )->l;( & theNode ->part->list )->l->n = ( theNode );( & 
theNode ->part->list )->l = ( theNode );} ( theNode )->n = 0;( & theNode ->part->list )->c++;	if(( & 
theNode ->part->list )->add_cmd != ((void *) 0) ) (*(( & theNode ->part->list )->add_cmd)) (( & theNode 
->part->list )); } ; } if( theNode ->part->free_cmd != ((void *) 0) ) { (*( theNode ->part->free_cmd)) ( theNode 
->part->clientData); } } ;       if (theEvMask) {
	theEvMask = theEvMask & ~(1<<theSource);
      } 
      if (!theEvMask) {
	rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;       }
    }  
  }
  dispatch_busy = 0;
}
static int theIntHandler(int theSource)
{
  if (theSource == 0) return(0);
  {  
    DANODE *theNode;
    intLockKey = intLock(); ;
{{ ( theNode ) = 0; if (( &( rol->dispatch ->list) )->c){ ( &( rol->dispatch ->list) )->c--; ( 
theNode ) = ( &( rol->dispatch ->list) )->f; ( &( rol->dispatch ->list) )->f = ( &( rol->dispatch ->list) 
)->f->n; }; if (!( &( rol->dispatch ->list) )->c) { ( &( rol->dispatch ->list) )->l = 0; }} ;} ;     theNode->source = theSource;
    theNode->type = (*ttypeRtns[theSource])(Tcode[theSource]);
    theNode->reader = trigRtns[theSource]; 
{if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ;     if (!dispatch_busy)
      cdodispatch();
    intUnlock(intLockKey); ;
  }
}
static int cdopolldispatch()
{
  unsigned long theSource, theType;
  int stat = 0;
  DANODE *theNode;
  if (!poolEmpty) {
    for (theSource=1;theSource<trigId;theSource++){
      if (syncTRtns[theSource]){
	if ( theNode = (*syncTRtns[theSource])(Tcode[theSource])) {
	  stat = 1;
	  {  
	    intLockKey = intLock(); ;
	    if (theNode) 
	 {{ ( theNode ) = 0; if (( &( rol->dispatch ->list) )->c){ ( &( rol->dispatch ->list) )->c--; ( 
theNode ) = ( &( rol->dispatch ->list) )->f; ( &( rol->dispatch ->list) )->f = ( &( rol->dispatch ->list) 
)->f->n; }; if (!( &( rol->dispatch ->list) )->c) { ( &( rol->dispatch ->list) )->l = 0; }} ;} ; 	    theNode->source = theSource; 
	    theNode->type = (*ttypeRtns[theSource])(Tcode[theSource]); 
	    theNode->reader = trigRtns[theSource]; 
	 {if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ; 	    if (!dispatch_busy) 
	      cdodispatch();
	    intUnlock(intLockKey); ;
	  }
	}
      }
    }   
  } else {
    stat = -1;
  }
  return (stat);
}
/* # 99 "/site/coda/2.6.2/common/include/rol.h" 2 */
static char rol_name__[40];
static char temp_string__[132];
static void __poll()
{
    {cdopolldispatch();} ;
}
void sfi_gen__init (rolp)
     rolParam rolp;
{
      if ((rolp->daproc != 7 )&&(rolp->daproc != 6 )) 
	printf("rolp->daproc = %d\n",rolp->daproc);
      switch(rolp->daproc) {
      case 0 :
	{
	  char name[40];
	  rol = rolp;
	  rolp->inited = 1;
	  strcpy(rol_name__, "GEN_USER" );
	  rolp->listName = rol_name__;
	  printf("Init - Initializing new rol structures for %s\n",rol_name__);
	  strcpy(name, rolp->listName);
	  strcat(name, ":pool");
	  rolp->pool  = partCreate(name, 4096  , 512 ,1);
	  if (rolp->pool == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  strcpy(name, rolp->listName);
	  strcat(name, ":dispatch");
	  rolp->dispatch  = partCreate(name, 0, 32, 0);
	  if (rolp->dispatch == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  strcpy(name, rolp->listName);
	  strcat(name, ":dispQ");
	  rolp->dispQ = partCreate(name, 0, 0, 0);
	  if (rolp->dispQ == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  rolp->inited = 1;
	  printf("Init - Done\n");
	  break;
	}
      case 9 :
	  rolp->inited = 0;
	break;
      case 1 :
	__download();
	break;
      case 2 :
	__prestart();
	break;
      case 4 :
	__pause();
	break;
      case 3 :
	__end();
	break;
      case 5 :
	__go();
	break;
      case 6 :
	__poll();
	break;
      case 7 :
	__done();
	break;
      default:
	printf("WARN: unsupported rol action = %d\n",rolp->daproc);
	break;
      }
}
/* # 6 "sfi_gen.c" 2 */
/* # 1 "/site/coda/2.6.2/common/include/SFI_source.h" 1 */
unsigned long sfiTrigSource;
unsigned long sfiAuxPort;
unsigned long sfi_vme_base_address, sfi_cpu_mem_offset;
/* # 1 "/site/coda/2.6.2/common/include/sfi.h" 1 */
struct sfiStruct {
   volatile unsigned long VMESlaveAddress;
   volatile unsigned long* lca1Vout;      
   volatile unsigned long* outregWrite;  
   volatile unsigned long* outregRead;
   volatile unsigned long* dfctrlReg;
   volatile unsigned long* fbProtReg;
   volatile unsigned long* fbArbReg;
   volatile unsigned long* fbCtrlReg;
   volatile unsigned long* lca1Reset;
   volatile unsigned long* lca2Reset;
   volatile unsigned long* fastbusreadback;
   volatile unsigned long* sequencerRamAddressReg;
   volatile unsigned long* sequencerFlowControlReg;
   volatile unsigned long* resetVme2SeqFifo;
   volatile unsigned long* readClockVme2SeqFifo;
   volatile unsigned long* resetSeq2VmeFifo;
   volatile unsigned long* writeClockSeq2VmeFifo;
   volatile unsigned long* writeVme2SeqFifoBase;
   volatile unsigned long* readSeq2VmeFifoBase;
   volatile unsigned long* readSeqFifoFlags;
   volatile unsigned long* readVme2SeqAddressReg;
   volatile unsigned long* readVme2SeqDataReg;
   volatile unsigned long  sequencerOutputFifoSize;
   volatile unsigned long* sequencerReset;
   volatile unsigned long* sequencerEnable;
   volatile unsigned long* sequencerDisable;
   volatile unsigned long* sequencerRamLoadEnable;
   volatile unsigned long* sequencerRamLoadDisable;
   volatile unsigned long* sequencerStatusReg;
   volatile unsigned long* FastbusStatusReg1;
   volatile unsigned long* FastbusStatusReg2;
   volatile unsigned long* FastbusTimeoutReg;
   volatile unsigned long* FastbusArbitrationLevelReg;
   volatile unsigned long* FastbusProtocolInlineReg;
   volatile unsigned long* sequencerFifoFlagAndEclNimInputReg;
   volatile unsigned long* nextSequencerRamAddressReg;
   volatile unsigned long* lastSequencerProtocolReg;
   volatile unsigned long* VmeIrqLevelAndVectorReg;
   volatile unsigned long* VmeIrqMaskReg;
   volatile unsigned long* resetRegisterGroupLca2;
   volatile unsigned long* sequencerTryAgain;
   volatile unsigned long* readVmeTestReg;
   volatile unsigned long* readLocalFbAdBus;
   volatile unsigned long* readVme2SeqDataFifo;
   volatile unsigned long* writeVmeOutSignalReg;
   volatile unsigned long* clearBothLca1TestReg;
   volatile unsigned long* writeVmeTestReg;
   volatile unsigned long* writeAuxReg;
   volatile unsigned long* generateAuxB40Pulse;
};
/* # 21 "/site/coda/2.6.2/common/include/SFI_source.h" 2 */
/* # 1 "/site/coda/2.6.2/common/include/sfi_fb_macros.h" 1 */
extern volatile struct sfiStruct sfi;
extern unsigned long sfiSeq2VmeFifoVal;
extern volatile unsigned long *fastPrimDsr;
extern volatile unsigned long *fastPrimCsr;
extern volatile unsigned long *fastPrimDsrM;
extern volatile unsigned long *fastPrimCsrM;
extern volatile unsigned long *fastPrimHmDsr;
extern volatile unsigned long *fastPrimHmCsr;
extern volatile unsigned long *fastPrimHmDsrM;
extern volatile unsigned long *fastPrimHmCsrM;
extern volatile unsigned long *fastSecadR;
extern volatile unsigned long *fastSecadW;
extern volatile unsigned long *fastSecadRDis;
extern volatile unsigned long *fastSecadWDis;
extern volatile unsigned long *fastRndmR;
extern volatile unsigned long *fastRndmW;
extern volatile unsigned long *fastRndmRDis;
extern volatile unsigned long *fastRndmWDis;
extern volatile unsigned long *fastDiscon;
extern volatile unsigned long *fastDisconRm;
extern volatile unsigned long *fastStartFrdbWithClearWc;
extern volatile unsigned long *fastStartFrdb;
extern volatile unsigned long *fastStoreFrdbWc;
extern volatile unsigned long *fastStoreFrdbAp;
extern volatile unsigned long *fastLoadDmaAddressPointer;
extern volatile unsigned long *fastDisableRamMode;
extern volatile unsigned long *fastEnableRamSequencer;
extern volatile unsigned long *fastWriteSequencerOutReg;
extern volatile unsigned long *fastDisableSequencer;
/* # 22 "/site/coda/2.6.2/common/include/SFI_source.h" 2 */
/* # 1 "/site/coda/2.6.2/common/include/sfi_triglib.h" 1 */
unsigned int sfiAuxVersion = 0;  
static inline void
sfiAuxWrite(val32)
     unsigned long val32;
{
  *sfi.sequencerDisable = 0;
  *sfi.writeAuxReg = val32;
  *sfi.writeVmeOutSignalReg = 0x4000;
  *sfi.generateAuxB40Pulse = 0;
  *sfi.writeVmeOutSignalReg = 0x40000000;
}
unsigned long
sfiAuxRead()
{
  int xx=0;
  unsigned long val32 = 0xffffffff;
  *sfi.sequencerDisable = 0;
  *sfi.writeVmeOutSignalReg = 0x2000;
  while ( (xx<10) && (val32 == 0xffffffff) ) {
    val32 = *sfi.readLocalFbAdBus;
    xx++;
  }
  *sfi.writeVmeOutSignalReg = 0x20000000;
  return (val32);
}
int
sfiAuxInit()
{
  unsigned long rval = 0;
  rval = sfiAuxRead();
  if (rval == 0xffffffff) {
    printf("sfiAuxInit: ERROR: AUX card not addressable\n");
    return(-1);
  } else {
    sfiAuxWrite(0x8000);  
    sfiAuxVersion = (0x00006000  & rval);
    printf("sfiAuxInit: sfiAuxVersion = 0x%x\n",sfiAuxVersion);
  }
  return(0);
}
static inline void 
sfitriglink(int code, VOIDFUNCPTR isr)
{
    *sfi.VmeIrqLevelAndVectorReg = (0x800 | (5 <<8) | 0xec );
  if((intDisconnect(((VOIDFUNCPTR *) ( 0xec  )) ) !=0))
     printf("Error disconnecting Interrupt\n");
  intConnect(((VOIDFUNCPTR *) ( 0xec  )) ,isr,0);
}
static inline void 
sfitenable(int code, unsigned int intMask)
{
 int lock_key;
  lock_key = intLock();
  *sfi.VmeIrqMaskReg  = (intMask<<8);           
  *sfi.VmeIrqMaskReg  = intMask;               
  sysIntEnable(5 );                     
  intEnable(11);        
  if(intMask == 0x10) {
    if(sfiAuxVersion == 0x00004000 )
      sfiAuxWrite(0x300);
    else
      sfiAuxWrite(0x180);
  }
  intUnlock(lock_key);
/* # 184 "/site/coda/2.6.2/common/include/sfi_triglib.h" */
}
static inline void 
sfitdisable(int code, unsigned int intMask)
{
  *sfi.VmeIrqMaskReg  = (intMask<<8);       
  sysIntDisable(5 );         
  if(intMask == 0x10) {
    sfiAuxWrite(0x8000);
  }
}
static inline void 
sfitack(int code, unsigned int intMask)
{
  if(intMask == 0x10) {
    *sfi.writeVmeOutSignalReg = 0x1000;        
    *sfi.VmeIrqMaskReg  = intMask;             
    *sfi.writeVmeOutSignalReg = 0x10000000;    
  } else {
    *sfi.VmeIrqMaskReg  = intMask;             
  }
}
static inline unsigned long 
sfittype(int code)
{
  unsigned long tt;
/* # 237 "/site/coda/2.6.2/common/include/sfi_triglib.h" */
  tt=1;
  return(tt);
}
static inline int 
sfittest(int code)
{
  int ii=0;
  cacheInvalidate(1,sfi.VmeIrqLevelAndVectorReg,4);
  ii = (((*sfi.VmeIrqLevelAndVectorReg) & 0x4000) != 0);
  return(ii);
}
/* # 23 "/site/coda/2.6.2/common/include/SFI_source.h" 2 */
static int padr, sadr;
static int SFI_handlers,SFIflag;
static unsigned long SFI_isAsync;
static inline void 
fpbr(int pa, long len) 
{
  unsigned long bufp, rb, lenb;
  int res;
 if (len <= 0) len = (4096 >>4) - 2;
 bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
 lenb = (len<<2);
 if (pa >= 0) {
   res = fb_frdb_1(pa,0,bufp,lenb,&rb,1,0,1,0,0x0a,0,0,1);
 } else {
   res = fb_frdb_1(0,0,bufp,lenb,&rb,1,1,1,0,0x0a,1,1,1);
 }
 if ((rb > (lenb+4))||(res != 0)) {
   *rol->dabufp++ = 0xfb000bad;
logMsg("fpbr error pa=%d res=0x%x maxBytes=%d retBytes=%d 
fifo=0x%x\n",pa,res,lenb,rb,sfiSeq2VmeFifoVal,0);    sfi_error_decode(0x3);
 }else{
   rol->dabufp += (rb>>2);
 }
 return;   
 fooy:
  sfi_error_decode(0) ;
  return;
}
static inline void 
fpbr2(int pa, int sa, long len) 
{
  unsigned long bufp, rb, lenb;
  int res;
 if (len <= 0) len = (4096 >>4) - 2;
 bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
 lenb = (len<<2);
 if (pa >= 0) {
   if (sa > 0) {
     res = fb_frdb_1(pa,sa,bufp,lenb,&rb,1,0,0,0,0x0a,0,0,1);
   }else{
     res = fb_frdb_1(pa,0,bufp,lenb,&rb,1,0,1,0,0x0a,0,0,1);
   }
 } else {
     res = fb_frdb_1(0,0,bufp,lenb,&rb,1,1,1,0,0x0a,1,1,1);
 }
 if ((rb > (lenb+4))||(res != 0)) {
   *rol->dabufp++ = 0xfb000bad;
logMsg("fpbr error pa=%d res=0x%x maxBytes=%d retBytes=%d \n",pa,res,lenb,rb,0,0);    sfi_error_decode(0x3);
 }else{
   rol->dabufp += (rb>>2);
 }
 return;   
 fooy:
  sfi_error_decode(0) ;
  return;
}
static inline void 
fpbrf(int pa, long len) 
{
  unsigned long bufp, rb, lenb;
  int res;
  if (len <= 0) len = (4096 >>4) - 2;
  lenb = (len<<2);
  if( ( (unsigned long) (rol->dabufp)&0x7) ) {
    *rol->dabufp++ = 0xdaffffff;
    bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
  } else {
    bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
  }
  if (pa >= 0) {
    res = fb_frdb_1(pa,0,bufp,lenb,&rb,1,0,1,0,0x08,0,0,1);
  } else {
    res = fb_frdb_1(0,0,bufp,lenb,&rb,1,1,1,0,0x08,1,1,1);
  }
  if ((rb > (lenb+4))||(res != 0)) {
    *rol->dabufp++ = 0xfb000bad;
logMsg("fpbrf error pa = %d res = 0x%x maxbytes = %d returnBytes = %d \n",pa,res,lenb,rb);     sfi_error_decode(0x3);
  }else{
    rol->dabufp += (rb>>2);
  }
  return;   
 fooy:
  sfi_error_decode(0) ;
  return;
}
static inline void 
fpbrf2(int pa, int sa, long len) 
{
  unsigned long bufp, rb, lenb;
  int res;
  if (len <= 0) len = (4096 >>4) - 2;
  lenb = (len<<2);
  if( ( (unsigned long) (rol->dabufp)&0x7) ) {
    *rol->dabufp++ = 0xdaffffff;
    bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
  } else {
    bufp = (unsigned long) (rol->dabufp) + sfi_cpu_mem_offset;
  }
  if (pa >= 0) {
    if (sa > 0) {
      res = fb_frdb_1(pa,sa,bufp,lenb,&rb,1,0,0,0,0x08,0,0,1);
    }else{
      res = fb_frdb_1(pa,0,bufp,lenb,&rb,1,0,1,0,0x08,0,0,1);
    }
  } else {
    res = fb_frdb_1(0,0,bufp,lenb,&rb,1,1,1,0,0x08,1,1,1);
  }
  if ((rb > (lenb+4))||(res != 0)) {
    *rol->dabufp++ = 0xfb000bad;
logMsg("fpbrf error pa = %d res = 0x%x maxbytes = %d returnBytes = %d \n",pa,res,lenb,rb);     sfi_error_decode(0x3);
  }else{
    rol->dabufp += (rb>>2);
  }
  return;   
 fooy:
  sfi_error_decode(0) ;
  return;
}
void 
SFI_int_handler()
{
  *sfi.VmeIrqMaskReg  = (sfiTrigSource<<8);       
  theIntHandler(SFI_handlers);                    
}
/* # 7 "sfi_gen.c" 2 */
/* # 1 "GEN_source.h" 1 */
static int GEN_handlers,GENflag;
static int GEN_isAsync;
static unsigned int *GENPollAddr = ((void *) 0) ;
static unsigned int GENPollMask;
static unsigned int GENPollValue;
static unsigned long GEN_prescale = 1;
static unsigned long GEN_count = 0;
/* # 1 "tiLib.h" 1 */
extern int intLock();
extern int intUnlock();
int intLockKeya;
struct TI_A24RegStruct
{
    volatile unsigned int boardID;
    volatile unsigned int fiber;
    volatile unsigned int intsetup;
    volatile unsigned int trigDelay;
    volatile unsigned int adr32;
    volatile unsigned int blocklevel;
    volatile unsigned int dataFormat;
    volatile unsigned int vmeControl;
    volatile unsigned int trigsrc;
    volatile unsigned int sync;
    volatile unsigned int busy;
    volatile unsigned int clock;
    volatile unsigned int trig1Prescale;
    volatile unsigned int blockBuffer;
    volatile unsigned int triggerRule;
    volatile unsigned int triggerWindow;
             unsigned int blank0;
    volatile unsigned int tsInput;
             unsigned int blank1;
    volatile unsigned int output;
    volatile unsigned int fiberSyncDelay;
             unsigned int blank_prescale[(0x74-0x54)/4];
             unsigned int inputPrescale;
    volatile unsigned int syncCommand;
    volatile unsigned int syncDelay;
    volatile unsigned int syncWidth;
    volatile unsigned int triggerCommand;
    volatile unsigned int randomPulser;
    volatile unsigned int fixedPulser1;
    volatile unsigned int fixedPulser2;
    volatile unsigned int nblocks;
    volatile unsigned int syncHistory;
    volatile unsigned int runningMode;
    volatile unsigned int fiberLatencyMeasurement;
    volatile unsigned int fiberAlignment;
    volatile unsigned int livetime;
    volatile unsigned int busytime;
    volatile unsigned int GTPStatusA;
    volatile unsigned int GTPStatusB;
    volatile unsigned int GTPtriggerBufferLength;
    volatile unsigned int inputCounter;
    volatile unsigned int blockStatus[4];
    volatile unsigned int adr24;
    volatile unsigned int syncEventCtrl;
    volatile unsigned int eventNumber_hi;
    volatile unsigned int eventNumber_lo;
             unsigned int blank2[(0xEC-0xE0)/4];
    volatile unsigned int rocEnable;
             unsigned int blank3[(0xFC-0xF0)/4];
    volatile unsigned int blocklimit;
    volatile unsigned int reset;
    volatile unsigned int fpDelay[2];
             unsigned int blank4[(0x110-0x10C)/4];
             unsigned int busy_scaler1[7];
             unsigned int blank5[(0x140-0x12C)/4];
    volatile unsigned int trigTable[(0x180-0x140)/4];
    volatile unsigned int ts_scaler[6];
             unsigned int blank6;
    volatile unsigned int busy_scaler2[9];
             unsigned int blank7[(0x1D0-0x1C0)/4];
    volatile unsigned int hfbr_tiID[8];
    volatile unsigned int master_tiID;
             unsigned int blank8[(0x2000-0x1F4)/4];
    volatile unsigned int SWB_status[(0x2200-0x2000)/4];
             unsigned int blank9[(0x2800-0x2200)/4];
    volatile unsigned int SWA_status[(0x3000-0x2800)/4];
             unsigned int blank10[(0xFFFC-0x3000)/4];
    volatile unsigned int eJTAGLoad;
    volatile unsigned int JTAGPROMBase[(0x20000-0x10000)/4];
    volatile unsigned int JTAGFPGABase[(0x30000-0x20000)/4];
    volatile unsigned int SWA[(0x40000-0x30000)/4];
    volatile unsigned int SWB[(0x50000-0x40000)/4];
};
int  tiSetFiberLatencyOffset_preInit(int flo);
int  tiSetCrateID_prIinit(int cid);
int  tiInit(unsigned int tAddr, unsigned int mode, int force);
unsigned int tiFind();
int  tiCheckAddresses();
void tiStatus(int pflag);
int  tiSetSlavePort(int port);
int  tiGetSlavePort();
void tiSlaveStatus(int pflag);
int  tiGetFirmwareVersion();
int  tiReload();
unsigned int tiGetSerialNumber(char **rSN);
int  tiClockResync();
int  tiReset();
int  tiSetCrateID(unsigned int crateID);
int  tiGetCrateID(int port);
int  tiGetPortTrigSrcEnabled(int port);
int  tiGetSlaveBlocklevel(int port);
int  tiSetBlockLevel(int blockLevel);
int  tiBroadcastNextBlockLevel(int blockLevel);
int  tiGetNextBlockLevel();
int  tiGetCurrentBlockLevel();
int  tiSetInstantBlockLevelChange(int enable);
int  tiGetInstantBlockLevelChange();
int  tiSetTriggerSource(int trig);
int  tiSetTriggerSourceMask(int trigmask);
int  tiEnableTriggerSource();
int  tiDisableTriggerSource(int fflag);
int  tiSetSyncSource(unsigned int sync);
int  tiSetEventFormat(int format);
int tiSoftTrig(int trigger, unsigned int nevents, unsigned int period_inc, int range); int  tiSetRandomTrigger(int trigger, int setting);
int  tiDisableRandomTrigger();
int  tiReadBlock(volatile unsigned int *data, int nwrds, int rflag);
int  tiReadTriggerBlock(volatile unsigned int *data);
int  tiEnableFiber(unsigned int fiber);
int  tiDisableFiber(unsigned int fiber);
int  tiSetBusySource(unsigned int sourcemask, int rFlag);
int  tiSetTriggerLock(int enable);
int  tiGetTriggerLock();
void tiEnableBusError();
void tiDisableBusError();
int  tiPayloadPort2VMESlot(int payloadport);
unsigned int  tiPayloadPortMask2VMESlotMask(unsigned int ppmask);
int  tiVMESlot2PayloadPort(int vmeslot);
unsigned int  tiVMESlotMask2PayloadPortMask(unsigned int vmemask);
int  tiSetPrescale(int prescale);
int  tiGetPrescale();
int  tiSetInputPrescale(int input, int prescale);
int  tiGetInputPrescale(int input);
int  tiSetTriggerPulse(int trigger, int delay, int width);
int  tiSetPromptTriggerWidth(int width);
int  tiGetPromptTriggerWidth();
void tiSetSyncDelayWidth(unsigned int delay, unsigned int width, int widthstep);
void tiTrigLinkReset();
int  tiSetSyncResetType(int type);
void tiSyncReset(int bflag);
void tiSyncResetResync();
void tiClockReset();
int  tiSetAdr32(unsigned int a32base);
int  tiDisableA32();
int  tiResetEventCounter();
unsigned long long int tiGetEventCounter();
int  tiSetBlockLimit(unsigned int limit);
unsigned int  tiGetBlockLimit();
unsigned int  tiBReady();
int  tiGetSyncEventFlag();
int  tiGetSyncEventReceived();
int  tiGetReadoutEvents();
int  tiEnableVXSSignals();
int  tiDisableVXSSignals();
int  tiSetBlockBufferLevel(unsigned int level);
int  tiEnableTSInput(unsigned int inpMask);
int  tiDisableTSInput(unsigned int inpMask);
int tiSetOutputPort(unsigned int set1, unsigned int set2, unsigned int set3, unsigned int 
set4); int  tiSetClockSource(unsigned int source);
int  tiGetClockSource();
void  tiSetFiberDelay(unsigned int delay, unsigned int offset);
int  tiAddSlave(unsigned int fiber);
int  tiSetTriggerHoldoff(int rule, unsigned int value, int timestep);
int  tiGetTriggerHoldoff(int rule);
int  tiDisableDataReadout();
int  tiEnableDataReadout();
void tiResetBlockReadout();
int  tiTriggerTableConfig(unsigned int *itable);
int  tiGetTriggerTable(unsigned int *otable);
int  tiTriggerTablePredefinedConfig(int mode);
int  tiDefineEventType(int trigMask, int hwTrig, int evType);
int  tiLoadTriggerTable(int mode);
void tiPrintTriggerTable(int showbits);
int  tiSetTriggerWindow(int window_width);
int  tiGetTriggerWindow();
int  tiSetTriggerInhibitWindow(int window_width);
int  tiGetTriggerInhibitWindow();
int  tiSetTrig21Delay(int delay);
int  tiGetTrig21Delay();
int  tiLatchTimers();
unsigned int tiGetLiveTime();
unsigned int tiGetBusyTime();
int  tiLive(int sflag);
unsigned int tiGetTSscaler(int input, int latch);
unsigned int tiBlockStatus(int fiber, int pflag);
int  tiGetFiberLatencyMeasurement();
int  tiSetUserSyncResetReceive(int enable);
int  tiGetLastSyncCodes(int pflag);
int  tiGetSyncHistoryBufferStatus(int pflag);
void tiResetSyncHistory();
void tiUserSyncReset(int enable, int pflag);
void tiPrintSyncHistory();
int  tiSetSyncEventInterval(int blk_interval);
int  tiGetSyncEventInterval();
int  tiForceSyncEvent();
int  tiSyncResetRequest();
int  tiGetSyncResetRequest();
void tiTriggerReadyReset();
int  tiFillToEndBlock();
int  tiResetMGT();
int  tiSetTSInputDelay(int chan, int delay);
int  tiGetTSInputDelay(int chan);
int  tiPrintTSInputDelay();
unsigned int tiGetGTPBufferLength(int pflag);
unsigned int tiGetSWAStatus(int reg);
unsigned int tiGetSWBStatus(int reg);
int  tiGetGeoAddress();
int  tiIntConnect(unsigned int vector, VOIDFUNCPTR routine, unsigned int arg);
int  tiIntDisconnect();
int  tiAckConnect(VOIDFUNCPTR routine, unsigned int arg);
void tiIntAck();
int  tiIntEnable(int iflag);
void tiIntDisable();
unsigned int  tiGetIntCount();
unsigned int  tiGetAckCount();
int  tiGetSWBBusy(int pflag);
unsigned int tiGetBusyCounter(int busysrc);
int  tiPrintBusyCounters();
int  tiSetTokenTestMode(int mode);
int  tiSetTokenOutTest(int level);
int  tiRocEnable(int roc);
int  tiRocEnableMask(int rocmask);
int  tiGetRocEnableMask();
/* # 23 "GEN_source.h" 2 */
extern int tiDoAck;
void
GEN_int_handler()
{
  theIntHandler(GEN_handlers);                    
  tiDoAck=0;  
}
static void
gentriglink(int code, VOIDFUNCPTR isr)
{
  int stat=0;
  tiIntConnect(0xec ,isr,0);
}
static void 
gentenable(int code, int card)
{
  int iflag = 1;  
  int lockkey;
  if(GEN_isAsync==0)
    {
      GENflag = 1;
    }
  tiIntEnable(1); 
}
static void 
gentdisable(int code, int card)
{
  int iwait=0, bready=0, iread=0;
  extern unsigned int tiIntCount;
  if(GEN_isAsync==0)
    {
      GENflag = 0;
    }
  tiIntDisable();
  tiIntDisconnect();
  taskDelay(1);
  while(iwait<100)
    {
      iwait++;
      bready=tiBReady();
      if(bready)
	{
	  printf("bready = %d\n",bready);
	  for(iread=0; iread<bready; iread++)
	    {
	      tiIntCount++;
	      GEN_int_handler();
	    }
	}
      else
	{
	  tiBlockStatus(0,1);
	  break;
	}
    }
  if(bready!=0)
    {
      printf("WARNING: Events left on TI\n");
    }
}
static unsigned int
genttype(int code)
{
  unsigned int tt=0;
  if(code == 2) {
    tt = 1 ;
  } else {
    tt = 1;
  }
  return(tt);
}
static int 
genttest(int code)
{
  unsigned int ret=0;
  unsigned int tidata=0;
  tidata = tiBReady();
  if(tidata!=-1)
    {
      if(tidata)
	ret = 1;
      else 
	ret = 0;
    }
  else
    {
      ret = 0;
    }
  return ret;
}
static inline void 
gentack(int code, unsigned int intMask)
{
    {
      tiIntAck();
    }
}
/* # 8 "sfi_gen.c" 2 */
/* # 1 "sfi.h" 1 */
/* # 1 "usrstrutils.c" 1 */
char *internal_configusrstr=0;
char *file_configusrstr=0;
void getflagpos(char *s,char **pos_ret,char **val_ret);
int getflag(char *s)
{
  char *pos,*val;
  getflagpos(s,&pos,&val);
  if(!pos) return(0);
  if(!val) return(1);
  return(2);
}
char *getstr(char *s){
  char *pos,*val;
  char *end;
  char *ret;
  int slen;
  getflagpos(s,&pos,&val);
  if(!val){
    return(0);
  }
  end = strchr(val,',');	 
  if(end)
    slen = end - val;
  else				 
    slen = strlen(val);
  ret = (char *) malloc(slen+1);
  strncpy(ret,val,slen);
  ret[slen] = '\0';
  return(ret);
}
unsigned int getint(char *s)
{
  char *sval;
  int retval;
  sval = getstr(s);
  if(!sval) return(0);		 
  retval = strtol(sval,0,0);
  if(retval == 0x7FFFFFFF  && (sval[1]=='x' || sval[1]=='X')) { 
     sscanf(sval,"%x",&retval);
   }
  free(sval);
  return(retval);
}
void getflagpos_instring(char *constr, char *s,char **pos_ret,char **val_ret)
{
  int slen;
  char *pos,*val;
  slen=strlen(s);
  pos = constr;
  while(pos) {
    pos = strstr(pos,s);
    if(pos) {			 
      if((pos != constr && pos[-1] != ',') ||
	 (pos[slen] != '=' && pos[slen] != ',' && pos[slen] != '\0')) {
	pos += 1;	continue;
      } else break;		 
    }
  }
  *pos_ret = pos;
  if(pos) {
    if(pos[slen] == '=') {
      *val_ret = pos + slen + 1;
    } else 
      *val_ret = 0;
  } else
    *val_ret = 0;
  return;
}
void getflagpos(char *s,char **pos_ret,char **val_ret)
{
  getflagpos_instring(file_configusrstr,s,pos_ret,val_ret);
  if(*pos_ret) return;
  getflagpos_instring(rol->usrString,s,pos_ret,val_ret);
  if(*pos_ret) return;
  getflagpos_instring(internal_configusrstr,s,pos_ret,val_ret);
  return;
}
void init_strings()
{
  char *ffile_name;
  int fd;
  char s[256], *flag_line;
  if(!internal_configusrstr) {	 
    internal_configusrstr = (char *) malloc(strlen("" )+1);
    strcpy(internal_configusrstr,"" );
  }
  ffile_name = getstr("ffile" );
  fd = fopen(ffile_name,"r");
  if(!fd) {
    free(ffile_name);
    if(file_configusrstr) free(file_configusrstr);  
    file_configusrstr = (char *) malloc(1);
    file_configusrstr[0] = '\0';
  } else {
    flag_line = 0;
    while(fgets(s,255,fd)){
      char *arg;
      arg = strchr(s,';' );
      if(arg) *arg = '\0';  
      arg = s;			 
      while(*arg && isspace(*arg)){
	arg++;
      }
      if(*arg) {
	flag_line = arg;
	break;
      }
    }
    if(file_configusrstr) free(file_configusrstr);  
    if(flag_line) {		 
      file_configusrstr = (char *) malloc(strlen(flag_line)+1);
      strcpy(file_configusrstr,flag_line);
    } else {
      file_configusrstr = (char *) malloc(1);
      file_configusrstr[0] = '\0';
    }
    fclose(fd);
    free(ffile_name);
  }
}
/* # 1 "sfi.h" 2 */
/* # 1 "threshold.c" 1 */
/* # 1 "evmacro.h" 1 */
/* # 3 "threshold.c" 2 */
unsigned long thresholds[19 ][64 ];
unsigned long readback_thresholds[19 ][64 ];
unsigned long slots[19 ];
int nmodules;
void load_thresholds()
{
  char *fname;
  int fd;
  char s[256];
  int imodule, subadd;
  int slot;
  nmodules=0;			 
  fname = getstr("tfile");
  fd = fopen(fname,"r");
  if(!fd) {
    printf("Failed to open %s\n",fname);
  }
  printf("Reading ADC thresholds from %s\n",fname); 
  imodule=-1;
  slot = -1;
  subadd = 64 ;
  while(fgets(s,255,fd)) {
    char *arg;
    arg = strchr(s,';' );
    if(arg) *arg = '\0';  
    if(arg=strstr(s,"slot=")) {
      if(subadd < 64 ) {
	printf("Not enough thresholds for slot %d, setting all to zero\n"
	       ,slot);
	for(subadd=0;subadd< 64 ;subadd++){
	  thresholds[imodule][subadd] = 0;
	}
      }
      sscanf(arg+5,"%d",&slot);
      imodule++;
      subadd = 0;
      slots[imodule] = slot;
    } else if(slot >= 0) {
      if(subadd >= 64 ) {
      } else {
	arg = strtok(s,", \n");
	while(arg && subadd< 64 ) {
	  thresholds[imodule][subadd++] = strtol(arg,0,0);
	  arg = strtok(0,", \n");
	}
      }
    }
  }
  nmodules = imodule+1;
  printf("Done reading, nmodules = %d \n",nmodules); 
  fclose(fd);
  free(fname);
     for(imodule=0;imodule<nmodules;imodule++) {
       printf("%d ",slots[imodule]);
     }
     printf("\n nth_subadd = %d \n",64 );
     for(subadd=0;subadd< 64 ;subadd++){
       for(imodule=0;imodule<nmodules;imodule++) {
         printf("%d ",thresholds[imodule][subadd]);
      }
    printf("\n");
     }
}
void set_thresholds()
{
  int imodule;
  int ichan;
  int slot;
  unsigned long temp;
  for(imodule=0;imodule<nmodules;imodule++){
    slot = slots[imodule];
    padr = slot;
    for(ichan=0;ichan< 64 ;ichan++){
      sadr = 0xc0000000  + ichan;
	 temp = thresholds[imodule][ichan];
      fpwc(padr,sadr,temp);
      sfi_error_decode(0);
    }
  }
  for(imodule=0;imodule<nmodules;imodule++){
    slot = slots[imodule];
    padr = slot;
    for(ichan=0;ichan< 64 ;ichan++){
      sadr = 0xc0000000  + ichan;
      fprc(padr,sadr,&temp);
      sfi_error_decode(0);
      readback_thresholds[imodule][ichan] = temp;
    }
  }
  return;
 fooy:
  return;
}
void reset_adc(int slot)
{
  int ichan;
  padr = slot;
  fb_frc_1(slot ,0,0,1,1,0,1,1,1,1); 
  fb_fwd_1(0,0,0x40000000,0,1,1,1,0,1,1);
  for(ichan=0;ichan< 64 ;ichan++){
    sadr = 0xc0000000  + ichan;
    fb_frd_1(0,sadr,0,0,1,1,0,1,1,1); 
    fb_fwd_1(0,0,0,0,1,1,1,0,1,1);
  }
  fprel();
  return;
 fooy:
  return;
}
/* # 2 "sfi.h" 2 */
/* # 10 "sfi_gen.c" 2 */
unsigned long scan_mask;
extern int bigendian_out;
int blockLevel=1;
int nb = 0;
unsigned int *tiData= ((void *) 0) ;
unsigned int tibready;
int adcslots[4 ];
int modslots[4 ];
int csr0[4 ];
static int debug=1;
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
  if (sysLocalToBusAdrs(0x09,0,&sfi_cpu_mem_offset)) { 
     printf("**ERROR** in sysLocalToBusAdrs() call \n"); 
     printf("sfi_cpu_mem_offset=0 FB Block Reads may fail \n"); 
  } else { 
     printf("sfi_cpu_mem_offset = 0x%x \n",sfi_cpu_mem_offset); 
  } 
    *(rol->async_roc) = 0;  
  {   
unsigned long res, laddr, jj, kk;
bigendian_out = 0;
{ 
  int sfi_addr=0xe00000;
  res = (unsigned long) sysBusToLocalAdrs(0x39,sfi_addr ,&laddr);
  if (res != 0) {
     printf("Error Initializing SFI res=%d \n",res);
  } else {
     printf("Calling InitSFI() routine with laddr=0x%x.\n",laddr);
     InitSFI(laddr);
  }
 } 
{ 
{
  tiSetFiberLatencyOffset_preInit(0x40); 
  tiSetCrateID_preInit(0x1);  
  tiInit(trig_addr ,1 ,0);
  tiDisableBusError();
  if(1 ==0)  
    {
      tiDisableDataReadout();
      tiDisableA32();
    }
  tiSetBlockBufferLevel(1 );
  tiSetBusySource(0,1);
  tiSetTriggerSource(6 );
  tiStatus(0);
  tiDisableVXSSignals();
  tiSetEventFormat(2);
 }
 } 
{ 
{
  if (4 >0) {
    kk=0;
    for(jj= 15 ;jj<= 18 ;jj++) {
       adcslots[kk]=jj;
       modslots[kk]=jj;
       csr0[kk]=0x400;
       kk++;
     }
  }
  scan_mask = 0;
  for (jj=0; jj< 4 ; jj++)
     scan_mask |= (1<<modslots[jj]);
  printf ("Crate Scan mask = %x\n",scan_mask);  
}
 } 
    daLogMsg("INFO","User Download Executed");
  }   
    return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}       
static void __prestart()
{
{ dispatch_busy = 0; bzero((char *) evMasks, sizeof(evMasks)); bzero((char *) syncTRtns, 
sizeof(syncTRtns)); bzero((char *) ttypeRtns, sizeof(ttypeRtns)); bzero((char *) Tcode, sizeof(Tcode)); 
wrapperGenerator = 0; theEvMask = 0; currEvMask = 0; trigId = 1; poolEmpty = 0; __the_event__ = (DANODE *) 0; 
input_event__ = (DANODE *) 0; } ;     *(rol->nevents) = 0;
  {   
unsigned long pedsuppress;
    daLogMsg("INFO","Entering User Prestart");
    { GEN_handlers =0;GEN_isAsync = 0;GENflag = 0;} ;
{ void titrig ();void titrig_done (); doneRtns[trigId] = (FUNCPTR) ( titrig_done ) ; 
trigRtns[trigId] = (FUNCPTR) ( titrig ) ; Tcode[trigId] = ( 1 ) ; ttypeRtns[trigId] = genttype ; {printf("linking async GEN trigger to id %d 
\n", trigId ); GEN_handlers = ( trigId );GEN_isAsync = 1;gentriglink( 1 ,GEN_int_handler);} 
;trigId++;} ;     {evMasks[ 1 ] |= (1<<( GEN_handlers ));} ;
    fb_init_1(0);
    padr   = 15 ; 
    fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,0,0);
    padr   = 16 ; 
    fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,0,0);
    padr   = 17 ; 
    fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,0,0);
    padr   = 18 ; 
    fb_fwc_1(padr,0,0x40000000,1,1,0,1,0,0,0);
  sfi_error_decode(0);
{ 
    nb =0;
    pedsuppress = 0;  
      printf("ped suppression ? %d \n",pedsuppress); 
    if(pedsuppress) {
      load_thresholds();
      set_thresholds();
    }
    sfi_error_decode(0);
 } 
    padr   = 18 ; 
    fb_fwc_1(padr,0,0x00000904,1,1,0,1,0,1,1);
if(( pedsuppress == 1) ) {
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x4000008B,1,1,1,0,0,1,1);
}
else{
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x0000008B,1,1,1,0,0,1,1);
} 
    sadr = 7 ;
    fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
    fprel(); 
  sfi_error_decode(0);
    padr   = 17 ; 
    fb_fwc_1(padr,0,0x00001904,1,1,0,1,0,1,1);
if(( pedsuppress == 1) ) {
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x4000008B,1,1,1,0,0,1,1);
}
else{
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x0000008B,1,1,1,0,0,1,1);
} 
    sadr = 7 ;
    fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
    fprel(); 
  sfi_error_decode(0);
    padr   = 16 ; 
    fb_fwc_1(padr,0,0x00001904,1,1,0,1,0,1,1);
if(( pedsuppress == 1) ) {
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x4000008B,1,1,1,0,0,1,1);
}
else{
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x0000008B,1,1,1,0,0,1,1);
} 
    sadr = 7 ;
    fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
    fprel(); 
  sfi_error_decode(0);
    padr   = 15 ; 
    fb_fwc_1(padr,0,0x00001104,1,1,0,1,0,1,1);
if(( pedsuppress == 1) ) {
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x4000008B,1,1,1,0,0,1,1);
}
else{
    sadr = 1 ;
    fb_fwc_1(0,sadr,0x0000008B,1,1,1,0,0,1,1);
} 
    sadr = 7 ;
    fb_fwc_1(0,sadr,2,1,1,1,0,0,1,1);
    fprel(); 
  sfi_error_decode(0);
    daLogMsg("INFO","User Prestart Executed");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;     *(rol->nevents) = 0;
    rol->recNb = 0;
    return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}       
static void __end()
{
  {   
{ 
{
   gentdisable(  1  ,   0  );  ;
  tiStatus(0);
  printf("Interrupt Count: %8d \n",tiGetIntCount());
  printf("Live time percentage:%8d \n",tiLive(1));
  printf("Live time:%8d \n",tiGetLiveTime());
  printf("Busy time:%8d \n",tiGetBusyTime());
  if(tiData!= ((void *) 0) )
    {
      free(tiData);
      tiData= ((void *) 0) ;
   } 
}
 } 
    daLogMsg("INFO","User End Executed");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;     return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}  
static void __pause()
{
  {   
   gentdisable(  1  ,   0  );  ;
    daLogMsg("INFO","User Pause Executed");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;     return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}  
static void __go()
{
  {   
    daLogMsg("INFO","Entering User Go");
{ 
{
  blockLevel = tiGetCurrentBlockLevel();
  printf("rocGo: Block Level set to %d\n",blockLevel);
  tiData = (unsigned int*)malloc(50*sizeof(unsigned int));
}
 } 
{ 
{
   gentenable(  1  ,   0  );  ;
  tiStatus(0);
}
 } 
    daLogMsg("INFO","User Go Executed");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;     return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}
void titrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {   
unsigned long dCnt, ev_type, ii, datascan, was_scan, res, jj, fbres, numLocal, numBranchdata, 
syncFlag;   *sfi.sequencerEnable = 0;
  rol->dabufp = (long *) 0;
{ 
{
  if(1 ==1)
    {
      dCnt = tiReadBlock(tiData,50,0);
      if(dCnt<=0)
	{
	  logMsg("No data or error.  dCnt = %d\n",dCnt);
	}
      else
	{
	  ev_type=(tiData[2]&(0xFF000000))>>24;  
	  tibready = tiBReady();
	  numLocal= tiGetReadoutEvents();  
    	  numBranchdata=(tiData[4]&(0xF0000000))>>28;  
	  syncFlag = tiGetSyncEventFlag();
	  if (numBranchdata==0){
	  nb++;
	     }
	}
    }
}
 } 
{	{if(__the_event__ == (DANODE *) 0 && rol->dabufp == ((void *) 0) ) { {{ ( __the_event__ ) = 0; if (( 
&( rol->pool ->list) )->c){ ( &( rol->pool ->list) )->c--; ( __the_event__ ) = ( &( rol->pool 
->list) )->f; ( &( rol->pool ->list) )->f = ( &( rol->pool ->list) )->f->n; }; if (!( &( rol->pool ->list) 
)->c) { ( &( rol->pool ->list) )->l = 0; }} ;} ; if(__the_event__ == (DANODE *) 0) { logMsg ("TRIG ERROR: no pool buffer 
available\n"); return; } rol->dabufp = (long *) &__the_event__->length; if (input_event__) { 
__the_event__->nevent = input_event__->nevent; } else { __the_event__->nevent = *(rol->nevents); } } } ; 
StartOfEvent[event_depth__++] = (rol->dabufp); if(input_event__) {	*(++(rol->dabufp)) = (( 1 ) << 16) | (( 0x10 ) << 8) | (0xff & 
(input_event__->nevent));	} else {	*(++(rol->dabufp)) = (syncFlag<<24) | (( 1 ) << 16) | (( 0x10 ) << 8) | (0xff & 
*(rol->nevents));	}	((rol->dabufp))++;} ; { 
{
  if(1 ==1)
    {
{	long *StartOfBank; StartOfBank = (rol->dabufp); *(++(rol->dabufp)) = ((( 4 ) << 16) | ( 0x01 ) 
<< 8) | ( 0 );	((rol->dabufp))++; ;       if(dCnt<=0)
	{
	  logMsg("No data or error.  dCnt = %d\n",dCnt);
	}
      else
	{
	   *rol->dabufp++ =  0xddddcccc;
	   for(ii=0; ii<dCnt; ii++) {
	     *rol->dabufp++ = tiData[ii];
           }
           *rol->dabufp++ = 0xb0b04444;
           *rol->dabufp++ = 1 ;
           *rol->dabufp++ = numBranchdata;
	}
*StartOfBank = (long) (((char *) (rol->dabufp)) - ((char *) StartOfBank));	if ((*StartOfBank 
& 1) != 0) { (rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfBank += 1; }; if 
((*StartOfBank & 2) !=0) { *StartOfBank = *StartOfBank + 2; (rol->dabufp) = ((long *)((short *) 
(rol->dabufp))+1);; };	*StartOfBank = ( (*StartOfBank) >> 2) - 1;}; ;     }
 }
 } 
{ 
ii=50;
if (1 ==numBranchdata ) {
  ii=0;
  datascan = 0;
  while ((ii<50) && ((datascan&scan_mask) != scan_mask)) {
    fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
    ii++;
  }
 }
 was_scan=datascan;
 } 
{	long *StartOfBank; StartOfBank = (rol->dabufp); *(++(rol->dabufp)) = ((( 7 ) << 16) | ( 0x01 ) 
<< 8) | ( 0 );	((rol->dabufp))++; ; if(( ii <  50) ) {
  fb_fwcm_1(0x15,0,0x400,1,0,1,0,0,0);
    {*(rol->dabufp)++ = ( 0xda000011 );} ; 
    padr   = 18  ;
    fpbr(padr,520); 
    {*(rol->dabufp)++ = ( ii );} ; 
    {*(rol->dabufp)++ = ( 0xda000022 );} ; 
{ 
  datascan = 0;
  fbres = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
  if (fbres) logMsg("fbres = 0x%x\n",fbres,0,0,0,0,0);
  if ((datascan != 0) && (datascan&~scan_mask)) { 
logMsg("Error: Read data but More data available after readout datascan = 0x%08x fbres = 
0x%x\n",datascan,fbres,0,0,0,0);   }
 } 
}
else{
{ 
  datascan = 0;
  fbres = fb_frcm_1(9,0,&datascan,1,0,1,0,0,0);
  if (fbres) logMsg("fbres = 0x%x\n",fbres,0,0,0,0,0);
  if ((datascan != 0) && (datascan&~scan_mask)) { 
logMsg("Error: datascan = 0x%08x fbres = 0x%x numBranchdata = %d 
\n",datascan,fbres,numBranchdata);  }
 } 
    {*(rol->dabufp)++ = ( was_scan );} ; 
    {*(rol->dabufp)++ = ( ii );} ; 
    {*(rol->dabufp)++ = ( 0xda0000ff );} ; 
} 
*StartOfBank = (long) (((char *) (rol->dabufp)) - ((char *) StartOfBank));	if ((*StartOfBank 
& 1) != 0) { (rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfBank += 1; }; if 
((*StartOfBank & 2) !=0) { *StartOfBank = *StartOfBank + 2; (rol->dabufp) = ((long *)((short *) 
(rol->dabufp))+1);; };	*StartOfBank = ( (*StartOfBank) >> 2) - 1;}; ; {event_depth__--; *StartOfEvent[event_depth__] = (long) (((char *) (rol->dabufp)) - ((char 
*) StartOfEvent[event_depth__]));	if ((*StartOfEvent[event_depth__] & 1) != 0) { 
(rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfEvent[event_depth__] += 1; }; if 
((*StartOfEvent[event_depth__] & 2) !=0) { *StartOfEvent[event_depth__] = *StartOfEvent[event_depth__] + 2; (rol->dabufp) = 
((long *)((short *) (rol->dabufp))+1);; };	*StartOfEvent[event_depth__] = ( 
(*StartOfEvent[event_depth__]) >> 2) - 1;}; ;   }   
    return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}  
void titrig_done()
{
  {   
  }   
    return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}  
void __done()
{
poolEmpty = 0;  
  {   
   gentack(  1  ,  0  );  ;
  }   
    return;
   fooy: 
    sfi_error_decode(0) ;
    return ;
}  
static void __status()
{
  {   
  }   
}  
