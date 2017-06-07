#
# File:
#    Makefile
#
# Description:
#    Makefile for the coda primary and secondary readout lists 
#    running on an Intel-based controller running Linux
#
#
# $Date$
# $Rev$
#
# Uncomment DEBUG line for debugging info ( -g and -Wall )
#export DEBUG=1
#

CODA=/root/coda

# Plug in your primary readout lists here..
VMEROL			= vme_list.so vmets_list.so
# Add shared library dependencies here.  (vme, tir, jlabgef are already included)
ROLLIBS			= 

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
CFLAGS			= -O2 -g -w -I${LINUXVME_INC} -I. -I/usr/include -I${CODA}/common/include\
			  -L${LINUXVME_LIB} -L. -DJLAB
CFLAGS			+= -ljvme -ltir -ltsUtil $(ROLLIBS)
CFLAGS			+= -DLINUX -DDAYTIME=\""`date`"\"
ifdef DEBUG
CFLAGS			+= -Wall -g
endif

# DEFs for compiling CODA readout lists
CCRL			= ${CODA_BIN}/ccrl
CODA_INCS		= -I.  -I${LINUXVME_INC} -I${CODA}/common/include
CODA_LIBDIRS            = -L. -L${LINUXVME_LIB}
CODA_LIBS		= -ljvme -ltir
CODA_DEFS		= -DLINUX -DDAYTIME=\""`date`"\"
CODA_CFLAGS		= -O -g -w -fpic -shared ${CODA_INCS} ${CODA_LIBDIRS} \
			  ${CODA_LIBS} ${CODA_DEFS}
ifdef DEBUG
CODA_CFLAGS		+= -Wall -g
endif
CRLFILES		= $(wildcard *.crl)
CFILES			= $(CRLFILES:.crl=.c)
SOBJS			= $(CRLFILES:.crl=.so)


all: event_list.so

event_list.so: event_list.c
	@echo
	@echo "Building $@ from $<"
	@echo
	${CC} ${CODA_CFLAGS} -o $@ $<

clean distclean:
	@rm -f  *.so *~

.PHONY: all
