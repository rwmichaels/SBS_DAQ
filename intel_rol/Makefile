#
# File:
#    Makefile
#
# Description:
#    Makefile for the coda primary and secondary readout lists 
#    running on an Intel-tibased controller running Linux
#
#
# $Date$
# $Rev$
#
# Uncomment DEBUG line for debugging info ( -g and -Wall )
DEBUG=1


# Plug in your primary readout lists here..
VMEROL			= linuxsfi_list.so 
# Add shared library dependencies here.  (jvme, tir, tsUtil are already included)

CODA=/root/coda

ifndef LINUXVME_LIB
	LINUXVME_LIB	= ${CODA}/extensions/linuxvme/lib
endif
ifndef LINUXVME_INC
	LINUXVME_INC	= ${CODA}/extensions/linuxvme/include 
endif

# DEFs for compiling primary readout lists
CC			= gcc
AR                      = ar
RANLIB                  = ranlib
CFLAGS			= -O2 -w ${LINUXVME_INC} -I. -I/root/sfi/libsrc/ \
			  -I/usr/include -I${CODA}/common/include\
			  -L${LINUXVME_LIB} -L. -L/root/sfi/libsrc -DJLAB
CFLAGS			+= -ljvme -lsfifb /root/sfi/test/fb_diag_cl.so
CFLAGS			+= -DLINUX -DDAYTIME=\""`date`"\"
ifdef DEBUG
CFLAGS			+= -Wall -g
endif

# DEFs for compiling CODA readout lists
CCRL			= ${CODA_BIN}/ccrl
CODA_INCS		= -I.  -I${LINUXVME_INC} -I${CODA}/common/include
CODA_LIBDIRS            = -L. -L${LINUXVME_LIB}
CODA_LIBS		= -ljvme -lsfifb
CODA_DEFS		= -DLINUX -DDAYTIME=\""`date`"\"
CODA_CFLAGS		= -O -w -fpic -shared ${CODA_INCS} -I/root/sfi/libsrc \
		   	  ${CODA_LIBDIRS} \
			  ${CODA_LIBS} -L/root/sfi/libsrc ${CODA_DEFS}
ifdef DEBUG
CODA_CFLAGS		+= -Wall -g
endif



all: $(VMEROL)

linuxsfi_list.so: linuxsfi_list.c 
	@echo
	@echo "Building $@ from $<"
	@echo
	${CC} ${CODA_CFLAGS} -o $@ ${LINUXVME_LIB}/libti.so ${LINUXVME_LIB}/libtir.so /root/sfi/test/fb_diag_cl.so $<

%.so: %.c Makefile
	@echo
	@echo "Building $@ from $<"
	@echo
	$(CC) -fpic -shared  $(CFLAGS) -DINIT_NAME=$(@:.so=__init) -o $@ $<

clean distclean:
	@rm -f  $(VMEROL) *~ 

.PHONY: all
