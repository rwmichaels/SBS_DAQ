# SBS_DAQ
DAQ for the Fastbus portion of SBS.
In the SBS there is Fastbus for the EDET (12 crates) and CDET (9).
Here I have two subdirectories.
In ./fastbus/ there is software from Dave Abbott (slightly modified by me)
with test and diagnostic code, a library of useful functions and some 
documentation in fastbus.txt
In ./sbs_crl/ is the readout list for the SBS Fastbus.  The intention is
to have ONE readout list used by all crates.  The readout list automatically
senses what modules there are and what kinds of modules.  
