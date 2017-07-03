# pulsarTools

plotfil and plot_events require PGPLOT and its CPGPLOT extension.

Here is an example of my makefile:

```
#Compiling flags
CFLAGS = -O3 -I/path/to/cpgplot.h
CPPFLAGS = -O3 -I/path/to/cpgplot.h -lgfortran

#Linking flags
LFLAGS = -L/path/to/pgplot/libraries -lcpgplot -lpgplot -L/path/to/X11/libraries -lX11 -fno-backslash -lpng -lstdc++ -lm

# Compiler
CC = g++

all: plotfil plot_events

plotfil: plotfil.o
	gfortran -o plotfil plotfil.o $(LFLAGS)

plot_events: plot_events.o
	gfortran -o plot_events plot_events.o $(LFLAGS)

clean:
	rm -f *.o
```
