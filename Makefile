# cpu clock speed
MHZ=166.66

# for normal gcc
#
CC=gcc
#CC=gcc-2.7.2p
CFLAGS=-DDEBUG -O6 -fomit-frame-pointer -ffast-math -m486 -malign-jumps=2 -malign-loops=2 -malign-functions=2 -Wall -DMHZ=$(MHZ)

# for pentium gcc
# 
#CFLAGS=-O6 -mpentium -fopt-reg-use -fschedule-insns2 -ffast-math -Wall -DDEBUG -DMHZ=$(MHZ)
#CFLAGS=-O6 -mpentium -Wall -DDEBUG -DMHZ=$(MHZ)
CCDEBUG=-g1

# X11R6 include & libs 
#
XINCDIR=-I/usr/X11R6/include
XLIBDIR=-L/usr/X11R6/lib

# in case includes were lying around
#
STDINCDIR=
STDLIBDIR=

# Which X librairies to link with
XLIBS=-lEZ -lXext -lSM -lX11

# Of course we want libm 
STDLIBS=-lm

###############################################################################
# You should not have to edit below this line
#

INCDIRS=$(STDINCDIR) $(XINCDIR)
LIBDIRS=$(STDLIBDIR) $(XLIBDIR)

LIBS=$(STDLIBS) $(XLIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCDIRS) 

all: P5fftbench xspectrum

P5fftbench: libdsp.o P5fftbench.o
	$(CC) $(CFLAGS) -o P5fftbench libdsp.o P5fftbench.o  $(STDLIBDIRS) $(STDLIBS)

xspectrum: libdsp.o xspectrum.o
	$(CC) $(CFLAGS) -o xspectrum libdsp.o xspectrum.o  $(LIBDIRS) $(LIBS)

asm:
	$(CC) $(CCDEBUG) $(CFLAGS) -Wa,-al,-ah,-ad -c -o libdsp.o libdsp.c > libdsp.asm
	rm libdsp.o

clean:
	rm -f *.o *.s *~ P5fftbench xspectrum libdsp.asm
