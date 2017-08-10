# SBS_DAQ
DAQ for the Fastbus portion of SBS.
In the SBS there is Fastbus for the EDET (12 crates) and CDET (9).
Here I have the following subdirectories.

Update, Aug 10, 2017
From now on, we mainly need ./sbs_unified
it is based closely on code I got from Bryan Moffit, which works
on both vxWorks and Fastbus.  I have not yet added the Fastbus,
but will shortly.

In ./fastbus/ there is software from Dave Abbott (slightly modified by me)
with test and diagnostic code, a library of useful functions and some 
documentation in fastbus.txt.  This is pretty old code (ca 1996)

In ./intel_fb is largely the same as ./fastbus, but ported to Intel/Linux.

In ./secondary is a secondary readout list for Intel/Linux. 
The listGet macro was replaced by BoblistGet to avoid a 
SEGFAULT.  This is not completely understood, but it did make
the DAQ run stable.  (noted, June 2017)

(obsolete, see *** below) 
In ./sbs_crl/ is the vxWorks readout list for the SBS Fastbus.  
The intention is to have ONE readout list used by all crates.  
The readout list automatically senses what modules there are and 
what kinds of modules. At the moment the rules are: 

Rule 1. Need >= 3 modules of each type (ADC1881 or TDC1877) 
(if you want only 1 module you should do something else, maybe I
add this situation later).  

Rule 2. The modules of each type should be put in a block with no gap in
the slot number. E.g. slots 4,5,6,7 and not 4,5,7.  However, there
can be a gap between the block of ADCs and the block of TDCs.

(obsolete, see *** below) 
In ./intel_rol is essentially the same as ./sbs_crl, but ported to
Linux/VME.  The differerences are fairly minimimal, and the idea
is to eventually merge these codes into one code which has the
appropriate preprocessor flags to pick the platform dependence.

*** Again, we won't need sbs_crl or intel_rol in the near future,
only sbs_unified, which is much cleaner code.


 
