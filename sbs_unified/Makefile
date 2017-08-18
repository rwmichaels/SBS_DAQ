#
# File:
#    Makefile
#
# Description:
#    Makefile for the coda primary and secondary readout lists 
#    running on an Intel-based controller running Linux
#
#
#
DEBUG	?= 1
QUIET	?= 1
#
ifeq ($(QUIET),1)
        Q = @
else
        Q =
endif

# Make this "LINUX" or "PPC"
ARCH=PPC


# Plug in your primary readout lists here..
VMEROL			= vme_list.so sfi_list.so 
# Add shared library dependencies here.  (jvme, ti, are already included)
ROLLIBS			= -lremex

ifeq (${ARCH}, PPC)
	OS=VXWORKS
	CODA=/site/coda/2.6.2
# ./jvme ./ti ./vxWorks can be links to wherever these are.
# vxWorks are headers etc for vxWorks 5.5
	JVME_INC = -I./jvme -I./ti -I./vxWorks
else
        OS=LINUX
	CODA=/root/coda
# newlinux is something like /site/coda/contrib/devel
# and you have to compile libraries and put them in /lib
# and you have to copy over the *.h to /include
	LINUXVME_LIB = ${CODA}/newlinux/lib
	LINUXVME_INC = ${CODA}/newlinux/include
endif

DT:=\""$(shell date)"\"

WARN_SUPPRESS		=  
WARN_SUPPRESS		+= -Wno-unused
WARN_SUPPRESS		+= -Wno-format

# DEFs for compiling primary readout lists
ifeq (${OS}, LINUX)
CC			= gcc
AR                      = ar
RANLIB                  = ranlib
ifdef DEBUG
	CFLAGS		= -g -Wall
else
	CFLAGS		= -O3
endif
CFLAGS			+= -O -w -m32 -L. -L${LINUXVME_LIB} -L/root/sfi/libsrc 
CFLAGS			+= -DLINUX -DDAYTIME=$(DT)
LIBS			= -lrt -lpthread -ljvme -lti -lsfifb /root/sfi/test/fb_diag_cl.so
INCS			= -I. -I${LINUXVME_INC} -I${CODA}/common/include
DEPS			+= $(VMEROL:%.so=%.d)
else #VXWORKS
CC			= ccppc
ifdef DEBUG
	CFLAGS		= -Wall -g
else
	CFLAGS		= -O2
endif
CFLAGS			+= -fno-builtin -D_GNU_TOOL -DVXWORKS -DVXWORKSPPC
CFLAGS			+= -mcpu=604 -mlongcall
CFLAGS			+= -fno-for-scope 
CFLAGS			+= -fno-writable-strings 
CFLAGS			+= -fvolatile -DDAYTIME=$(DT)
CFLAGS			+= -c -DCPU=PPC604 -w
INCS			= -I. -isystem -I${CODA}/common/include -isystem  ${JVME_INC} 
DEPS			+= $(VMEROL:%.so=%.d)
endif #LINUX

CFLAGS			+= ${WARN_SUPPRESS}

ifeq (${OS}, LINUX)
all: $(VMEROL)

%.so: %.c
	@echo "Linux CC ${CC} LINUXVME_INC ${LINUXVME_INC} INCS ${INCS} CFLAGS $(CFLAGS) LIBS $(LIBS) and    $@"
	${Q}$(CC) -fpic -shared  $(CFLAGS) ${INCS} ${LIBS} \
		-DINIT_NAME=$(@:.so=__init) -o $@ $<
else
all: $(VMEROL:%.so=%.o)

%.o: %.c
	@echo "PPC CC ${CC}  INCS ${INCS}    $@"
	${Q}${CC} ${CFLAGS} ${INCS} -DINIT_NAME=$(@:.o=__init) -o $@ $<

endif

clean distclean:
	@rm -f  $(VMEROL) *~ *.d *.o *.so

ifeq (${OS}, LINUX)
%.d: %.c
	@echo " DEP    $@"
	@set -e; rm -f $@; \
	$(CC) -MM -shared $(CFLAGS) $(INCS)  $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.so $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
else
%.d: %.c
	@echo " DEP    $@"
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $(INCS)  $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
endif

-include $(DEPS)

.PHONY: all
