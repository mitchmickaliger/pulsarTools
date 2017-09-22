# pulsarTools

sift and strongSift are deigned to identify and group together candidates that are harmonically related,
or detections of the same signal at different DMs.

sift searches integer harmonics, as well as a few fractional harmonics. This version is less likely to
group real signals with noise or RFI than strongSift, but may return too many candidates.

strongSift searches integer harmonics, as well as A LOT of fractional harmonics. This version is more
likely to group real signals with noise or RFI than standard sift, but may return a more manageable
number of candidates.

Both sift and strongSift can be compiled with a simple call to g++, no extra libraries required!



plotFil and plotEvents require PGPLOT and its CPGPLOT extension.



Here is an example of my makefile:

```
#Compiling flags
CPPFLAGS = -O3 -I/path/to/cpgplot.h

#Linking flags
LFLAGS = -L/path/to/pgplot/libraries -lcpgplot -lpgplot -L/path/to/X11/libraries -lX11 -fno-backslash -lpng -lstdc++ -lm

# Compiler
CXX = g++

all: fileEdit plotFil plotEvents sift strongSift

filEdit:
	${CXX} -o filEdit filEdit.cpp

plotFil: plotFil.o
	gfortran -o plotFil plotFil.o $(LFLAGS)

plotEvents: plotEvents.o
	gfortran -o plotEvents plotEvents.o $(LFLAGS)

sift:
	${CXX} -o sift sift.cpp

strongSift:
	${CXX} -o strongSift strongSift.cpp

clean:
	rm -f *.o
```
