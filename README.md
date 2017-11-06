# pulsarTools

dmReducer removes entries from an ASCII events file that have arrival times greater than 60 seconds. This happens when there is an event at the beginning of a filterbank file that is appended to the previous filterbank file. Since the event will be picked up at the beginning of the next file, there is no need to record it twice.

______________________________
filAdder adds together any filterbank files you pass as arguments. NB, this does not check that the files you are adding are continguous in time, have the same frequency, etc.
______________________________



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
filAppender is similar to filAdder, but will append a specified length of one filterbank file to another. NB, this also will not check that the files you are adding are continguous in time, have the same frequency, etc.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
filEdit modifies filterbank file headers in place, a la filedit from SIGPROC, but has more available parameters to edit and is standalone.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
plotFil and plotEvents require PGPLOT and its CPGPLOT extension.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
receiver listens for connection on a specified port and writes incoming data to a specified file.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RFIclean applies a channel mask and/or runs MAD (median absolute deviation) cleaning on a filterbank file. The MAD cleaning algorithm is a CPU implementation.
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
sift and strongSift are deigned to identify and group together candidates that are harmonically related,
or detections of the same signal at different DMs.

sift searches integer harmonics, as well as a few fractional harmonics. This version is less likely to
group real signals with noise or RFI than strongSift, but may return too many candidates.

strongSift searches integer harmonics, as well as A LOT of fractional harmonics. This version is more
likely to group real signals with noise or RFI than standard sift, but may return a more manageable
number of candidates.

Both sift and strongSift can be compiled with a simple call to g++, no extra libraries required!
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



Here is an example of my makefile:

```
#Compiling flags
CPPFLAGS = -O3 -I/path/to/cpgplot.h

#Linking flags
LFLAGS = -L/path/to/pgplot/libraries -lcpgplot -lpgplot -L/path/to/X11/libraries -lX11 -fno-backslash -lpng -lstdc++ -lm

# Compiler
CXX = g++

all: dmReducer filAdder filAppender filEdit plotFil plotEvents receiver RFIclean sift strongSift

dmReducer:
	${CXX} -o dmReducer dmReducer.cpp

filAdder:
	${CXX} -o filAdder filAdder.cpp

filAppender:
	${CXX} -o filAppender filAppender.cpp

filEdit:
	${CXX} -o filEdit filEdit.cpp

plotFil: plotFil.o
	gfortran -o plotFil plotFil.o $(LFLAGS)

plotEvents: plotEvents.o
	gfortran -o plotEvents plotEvents.o $(LFLAGS)

receiver:
	${CXX} -o receiver receiver.cpp

RFIclean:
	${CXX} -o RFIclean RFIclean.cpp

sift:
	${CXX} -o sift sift.cpp

strongSift:
	${CXX} -o strongSift strongSift.cpp

clean:
	rm -f *.o
```
