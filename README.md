# SBS_DAQ
DAQ for the Fastbus portion of SBS.
In the SBS there is Fastbus for the EDET (12 crates) and CDET (9).
Here I have two subdirectories.
In ./fastbus/ there is software from Dave Abbott (slightly modified by me)
with test and diagnostic code, a library of useful functions and some 
documentation in fastbus.txt. 
In ./sbs_crl/ is the readout list for the SBS Fastbus.  The intention is
to have ONE readout list used by all crates.  The readout list automatically
senses what modules there are and what kinds of modules. At the moment 
the rules are: 1. Need >= 3 modules of each type (ADC1881 or TDC1877) 
(if you want only 1 module you should do something else, maybe I
add this situation later).  And, 2. The 
modules should be put in a block with no gap in
the slot number. E.g. slots 4,5,6,7 and not 4,5,7.


 
