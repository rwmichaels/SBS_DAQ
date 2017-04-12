#
# Description:
#   	Makefile for FASTBUS Module Test Routines
#       using SFI Master readout
#

LIBDIR = $(CODA)/VXWORKSPPC/lib
VXINC = /home/adaq/vxworks/5.5/5.5
CODA_INC = $(CODA)/source/fastbus/sfi

CC = ccppc
DEFS = -mcpu=604 -DCPU=PPC604 -DVXWORKS -D_GNU_TOOL -DVXWORKSPPC -Wall \
       -fno-for-scope -fno-builtin -fvolatile -fstrength-reduce -mlongcall
INCS = -I. -I$(CODA_INC) -I$(VXINC)
LIBS = -L$(LIBDIR)
CFLAGS = -O $(DEFS)

all:  test_1877.o test_1881.o test_1872.o fb_diag_cl.o

tdc_readout.o: tdc_readout.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ tdc_readout.c

test_1877.o: test_1877.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ test_1877.c

test_1881.o: test_1881.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ test_1881.c

test_1872.o: test_1872.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ test_1872.c

fb_diag_cl.o: fb_diag_cl.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ fb_diag_cl.c

clean:
	rm -f *.o

