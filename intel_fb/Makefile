#
# File:
#    Makefile
#
# Description:
#    Makefile for the JLAB extensions to the GEFANUC Controller API
#
#
# $Date$
# $Rev$
#

CODA=/root/coda

ifndef LINUXVME_LIB
	LINUXVME_LIB	= ${CODA}/extensions/linuxvme/lib
endif
ifndef LINUXVME_INC
	LINUXVME_INC	= ${CODA}/extensions/linuxvme/include
endif

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
AR                      = ar
RANLIB                  = ranlib
CFLAGS			= -Wall -O2 -I${LINUXVME_INC} -I. -I/root/sfi/libsrc/ \
			  -I/usr/include -I/root/coda/extensions/linuxvme/jvme \
			  -L${LINUXVME_LIB} -L/root/sfi/libsrc -L.

PROGS			= test_1877 test_1881 sfiTrigTest

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) *~ *.so

%: %.c
	$(CC) $(CFLAGS) -o $@ $(@:%=%.c) $(LIBS_$@) -lrt -ljvme -lsfifb

.PHONY: all clean distclean
