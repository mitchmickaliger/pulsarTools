# pulsarTools

plotfil requires PGPLOT and its CPGPLOT extension.

Here is an example of my makefile:

```
#Compiling flags
CFLAGS = -O3 -I/path/to/cpgplot.h

#Linking flags
LFLAGS = -L/path/to/pgplot/libraries -lcpgplot -lpgplot -lm -L/path/to/X11/libraries -lX11 -fno-backslash -lpng

all: plotfil

plotfil: plotfil.o
	gfortran -o plotfil plotfil.o $(LFLAGS)
```
