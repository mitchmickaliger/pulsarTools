#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "cpgplot.h"
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

#define LIM 256

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: plotFil (-options) -f filFile" << std::endl << std::endl;
  std::cout << "     -f: Input .fil file" << std::endl;
  std::cout << "     -b: Number of time samples to bin (default = 1)" << std::endl;
  std::cout << "     -c: Either plot channel frequencies (1) or channel indices (2) (default = 1)" << std::endl;
  std::cout << "     -C: Number of channels to add together (default = 1)" << std::endl;
  std::cout << "     -d: DM to dedsiperse before plotting (default = 0)" << std::endl;
  std::cout << "     -g: Output plot type (default = /xs)" << std::endl;
  std::cout << "     -k: Create a plot in grayscale (default = color)" << std::endl;
  std::cout << "     -S: Time at which to begin plotting (default = start of file)" << std::endl;
  std::cout << "     -T: Seconds of data to plot, from start of file or requested start point (default = til end of file)" << std::endl;
  std::cout << "     -t: Time chunk (in seconds) to plot (default = entire file)" << std::endl << std::endl;
}

/* -- plotFil ----------------------------------------------------------------------------------------------------
** Plots the spectrum from a filterbank file, showing either channel frequency vs time or channel index vs time. |
** Will average in time, but not in frequency (yet).                                                             |
** Will work with 1-, 2-, 4-, 8-, 16-, and 32-bit data.                                                          |
--------------------------------------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {

  char string[80], sourceName[80], plotType[LIM] = "/xs";
  int i, nchar = sizeof(int), samplesToAdd = 1, numChans = 0, channelsToAdd = 1, numBits = 0, numIFs = 0, arg, multiPage = 0, grayscale = 0;
  int telescopeID, dataType, machineID, bit, channelInfoType = 1, numBeams, beamNumber;
  unsigned char charOfValues, eightBitInt;
  unsigned short sixteenBitInt;
  double obsStart, sampTime, fCh1, fOff;
  long long numSamples = 0, numSamps = 0, sample, channel, numTimePoints, numChannels, numDataPointsAdded, bin, inputIndex, outputIndex;
  float dataMin, dataMax, frequency, dm = 0.0, dmDelay;
  float minPlotTime, maxPlotTime, freqMin, freqMax, t0 = -1.0, tl = -1.0, ts = -1.0, startTime, endTime;
  float tr[] = {-0.5, 1.0, 0.0, -0.5, 0.0, 1.0};
  float heat_l[] = {0.0, 0.2, 0.4, 0.6, 1.0}, heat_r[] = {0.0, 0.5, 1.0, 1.0, 1.0}, heat_g[] = {0.0, 0.0, 0.5, 1.0, 1.0}, heat_b[] = {0.0, 0.0, 0.0, 0.3, 1.0};
  std::ifstream file;

  // If the user has not provided any arguments or has forgotten to use a flag, print usage and exit
  if (argc < 3) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "b:c:C:d:g:kS:T:t:f:h")) != -1) {
    switch (arg) {

      case 'b':
        samplesToAdd = atoi(optarg);
        if (samplesToAdd < 1) {
          std::cerr << "Cannot add less than one time sample together! Defaulting to 1!" << std::endl;
          samplesToAdd = 1;
        }
        break;

      case 'c':
        channelInfoType = atoi(optarg);
        if (channelInfoType < 1 || channelInfoType > 2) {
          std::cerr << std::endl << "You must either choose to plot channel frequencies (-c 1) or channel indices (-c 2)!" << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'C':
        channelsToAdd = atoi(optarg);
        if (channelsToAdd < 1) {
          std::cerr << "Cannot add less than one channel together! Defaulting to 1!" << std::endl;
          channelsToAdd = 1;
        }
        break;

      case 'd':
        dm = atof(optarg);
        if (dm < 0) {
          std::cerr << "DM cannot be negative! Defaulting to 0!" << std::endl;
          dm = 0.0;
        }
        break;

      case 'g':
        strcpy(plotType, optarg);
        break;

      case 'k':
        grayscale = 1;
        break;

      case 'S':
        t0 = atof(optarg);
        break;

      case 'T':
        tl = atof(optarg);
        break;

      case 't':
        ts = atof(optarg);
        break;

      case 'f':
        file.open(argv[optind - 1], std::ifstream::binary | std::ifstream::ate);
        if (!file.is_open()) {
          std::cerr << "Error opening file " << argv[optind - 1] << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'h':
        usage();
        exit(0);

      default:
        return 0;
        break;

    }
  }

  // Check if the file has failed to open
  if (!file.is_open()) {
    std::cerr << "You must input a .fil file with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  // Since we opened the file at the end, report the size of the file. Once we have the header size we can calculate the length of the data.
  const size_t fileSize = file.tellg();

  // Seek back to the beginning of the file
  file.seekg(0, file.beg);

  // Read header parameters until "HEADER_END" is encountered
  while (true) {

    // Read string size
    strcpy(string, "ERROR");
    file.read((char*) &nchar, sizeof(int));
    if (!file) {
      std::cerr << "Error reading header string size!" << std::endl;
      exit(0);
    }

    // Skip wrong strings
    if (!(nchar > 1 && nchar < 80)) {
      continue;
    }

    // Read string
    file.read((char*) string, nchar);
    if (!file) {
      std::cerr << "Could not read header string!" << std::endl;
      exit(0);
    }
    string[nchar] = '\0';

    // Exit at end of header
    if (strcmp(string, "HEADER_END") == 0) {
      break;
    }

    // Read parameters
    if (strcmp(string, "HEADER_START") == 0) {
      continue;
    } else if (strcmp(string, "tsamp") == 0) {
      file.read((char*) &sampTime, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'tsamp' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      file.read((char*) &obsStart, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'tstart' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      file.read((char*) &fCh1, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'fch1' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      file.read((char*) &fOff, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'foff' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      file.read((char*) &numChans, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'nchans' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      file.read((char*) &numIFs, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'nifs' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0) {
      file.read((char*) &numBits, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'nbits' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      file.read((char*) &numSamps, sizeof(long long));
      if (!file) {
        std::cerr << "Did not read header parameter 'nsamples' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      file.read((char*) &machineID, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'machine_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "telescope_id") == 0) {
      file.read((char*) &telescopeID, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'telescope_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "data_type") == 0) {
      file.read((char*) &dataType, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'data_type' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "source_name") == 0) {
      file.read((char*) &nchar, sizeof(int));
      file.read((char*) sourceName, nchar);
      sourceName[nchar] = '\0';
      if (!file) {
        std::cerr << "Did not read header parameter 'source_name' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nbeams") == 0) {
      file.read((char*) &numBeams, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'nbeams' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "ibeam") == 0) {
      file.read((char*) &beamNumber, sizeof(int));
      if (!file) {
        std::cerr << "Did not read header parameter 'ibeam' properly!" << std::endl;
        exit(0);
      }
    } else {
      std::cerr << "Unknown header parameter " << string << std::endl;
    }

  }

  std::cerr << "Done reading header!" << std::endl;

  // Get the total size of the data in bytes based on the current position indicator and the value of the position indicator at the end of the file
  const size_t bufferSize = fileSize - file.tellg();

  // Calculate how many samples are in the file. This should be the same as the number of samples reported in the header.
  numSamples = (long long) (long double) bufferSize/((long double) numBits/8.0)/(long double) numChans;

  // Instantiate a zero-filled vector to hold the data
  std::vector<float> buffer(numSamples * numChans, 0);

  // Determine the bit width of the data and read them in appropriately
  // Data are read in as they are stored in the filterbank file, i.e. tsamp_1_chan_1, tsamp_1_chan_2, ..., tsamp_1_chan_N, tsamp_2_chan_1, ...
  switch (numBits) {

    // 1-bit data
    case 1:
      for (i = 0; i < numSamples * numChans; i += 8) {
        // Read one byte of data (eight 1-bit values) from the input file
        file.read((char*) &charOfValues, sizeof(unsigned char));
        if (!file) {
          std::cerr << "Could not read 1-bit values properly!" << std::endl;
          exit(0);
        } else {
          // One byte contains eight data values, so read each separately
          // Data are stored with the first value in the lowest bit, i.e. the right-most bit
          for (bit = 0; bit < 8; bit++) {
            // Perform a bitwise 'AND' operation between variable 'charOfValues' and decimal 1 (00000001 in binary)
            // This will return the value of the right-most bit of 'charOfValues'
            buffer[i + bit] = charOfValues & 1;
            // Shift the bits in variable 'charOfValues' one to the right
            charOfValues >>= 1;
          }
        }
      }
      break;

    // 2-bit data
    case 2:
      for (i = 0; i < numSamples * numChans; i += 4) {
        // Read one byte of data (four 2-bit values) from the input file
        file.read((char*) &charOfValues, sizeof(unsigned char));
        if (!file) {
          std::cerr << "Could not read 2-bit values properly!" << std::endl;
          exit(0);
        } else {
          // Perform bitwise 'AND' operations to pull out the correct two bits for each value
          // Shift the bits to the lowest two bits so they are read as floats properly
          buffer[i] = (float) (charOfValues & 3);                /* 3 =   00000011 in binary */
          buffer[i + 1] = (float) ((charOfValues & 12) >> 2);    /* 12 =  00001100 in binary */
          buffer[i + 2] = (float) ((charOfValues & 48) >> 4);    /* 48 =  00110000 in binary */
          buffer[i + 3] = (float) ((charOfValues & 192) >> 6);   /* 192 = 11000000 in binary */
        }
      }
      break;

    // 4-bit data
    case 4:
       for (i = 0; i < numSamples * numChans; i += 2) {
        // Read one byte of data (2 4-bit values) from the input file
        file.read((char*) &charOfValues, sizeof(unsigned char));
        if (!file) {
          std::cerr << "Could not read 4-bit values properly!" << std::endl;
          exit(0);
        } else {
          // Perform bitwise 'AND' operations to pull out the correct four bits for each value
          // Shift the bits to the lowest four bits so they are read as floats properly
          buffer[i] = (float) (charOfValues & 15);             /* 15 =  00001111 in binary */
          buffer[i + 1] = (float) ((charOfValues & 240) >> 4); /* 240 = 11110000 in binary */
        }
      }
      break;

    // 8-bit data
    case 8:
      for (i = 0; i < numSamples * numChans; i++) {
        file.read((char*) &eightBitInt, sizeof(unsigned char));
        if (!file) {
          std::cerr << "Could not read 8-bit data properly!" << std::endl;
          exit(0);
        } else {
          buffer[i] = (float) eightBitInt;
        }
      }
      break;

    // 16-bit data
    case 16:
      for (i = 0; i < numSamples * numChans; i++) {
        file.read((char*) &sixteenBitInt, sizeof(unsigned short));
        if (!file) {
          std::cerr << "Could not read 16-bit data properly!" << std::endl;
          exit(0);
        } else {
          buffer[i] = (float) sixteenBitInt;
        }
      }
      break;

    // 32-bit data
    case 32:
      file.read((char*) &buffer[0], sizeof(float) * numSamples * numChans);
      if (!file) {
        std::cerr << "Could not read 32-bit data properly!" << std::endl;
        exit(0);
      }
      break;

    default:
      std::cerr << "Cannot read " << numBits << " bit data!" << std::endl << "Data must be 1-, 2-, 4-, 8-, 16-, or 32-bit!" << std::endl;
      exit(0);

  }

  // Close file
  file.close();

  // Create a zero-filled vector of delays in time samples for each channel
  std::vector<int> dmDelaySamps(numChans, 0);
  // Compute dispersion shifts for each channel if the DM is not zero; otherwise the above zero-filled vector is already correct
  if (dm > 0.0) {
    for (channel = 0; channel < numChans; channel++) {
      frequency = fCh1 + (float) channel * fOff;
      dmDelay = 4.148808e3 * (pow(frequency, -2) - pow(fCh1, -2)) * dm;
      dmDelaySamps[channel] = (int) floor(dmDelay/sampTime + 0.5);
    }
  }

  // Calculate the number of points in time that will be plotted. This number will differ from the number of time samples only if the -b option is used to bin the data in time.
  numTimePoints = (long long) ((float) numSamples/(float) samplesToAdd);
  // calculate the number of points in frequency that will be plotted. This number will differ from the number of channels only if the -C option is used to bin the data in frequency.
  numChannels = (long long) ((float) numChans/(float) channelsToAdd);
  std::vector<float> data(numTimePoints * numChannels, 0);

  // Read the data into a vector, adding time samples and/or channels and dedispersing if required
  // Note that the data are corner-turned here, which is necessary for pgplot. This puts them into the data vector as tsamp_1_chan_1, tsamp_2_chan_1, ..., tsamp_N_chan_1, tsamp_1_chan_2, ...
  for (sample = 0; sample < numTimePoints; sample++) {
    for (channel = 0; channel < numChannels; channel++) {

      // Calculate the output index based on the current input sample and channel
      outputIndex = sample + numTimePoints * channel;

      // Count the number of data points added so we can normalize later
      // Will only differ from 1 if binning with the -b or -C flags
      numDataPointsAdded = 0;
/*
      // Dedisperse and, if requested, add 'channelsToAdd' channels together
      for (bin = 0; bin < channelsToAdd; bin++) {
        inputIndex = numTimePoints * (channelsToAdd * channel + bin) + sample;
        if (inputIndex >= numChannels * numSamples || inputIndex < 0) {
          continue;
        }
        data[outputIndex] += buffer[inputIndex];
        numDataPointsAdded++;
      }
*/
      // Add 'samplesToAdd' time samples together
      for (bin = 0; bin < samplesToAdd; bin++) {
        inputIndex = numChannels * (samplesToAdd * sample + bin + dmDelaySamps[channel]) + channel;
        if (inputIndex >= numChannels * numSamples || inputIndex < 0) {
          continue;
        }
        data[outputIndex] += buffer[inputIndex];
        numDataPointsAdded++;
      }

      // Normalize the data point based on the number of points added
      if (data[outputIndex] != 0) {
        data[outputIndex] /= (float) numDataPointsAdded;
      }

    }
  }

  // Find extremes of data for pgplot
  dataMin = *std::min_element(data.begin(), data.end());
  dataMax = *std::max_element(data.begin(), data.end());

  // Transformation matrix used by cpgimag to map data onto plot
  tr[1] = sampTime * numSamples/(float) numTimePoints;
  tr[2] = 0.0;
  tr[0] = -0.5 * tr[1];
  tr[4] = 0.0;
  if (channelInfoType == 1) { // If plotting channel frequencies, center first Y-bin on fCh1
    tr[5] = fOff * numChannels/(float) numChannels;
    tr[3] = fCh1 - 0.5 * tr[5];
  } else if (channelInfoType == 2) { // If plotting channel indices, center first Y-bin on 0.5
    tr[5] = 1.0;
    tr[3] = 0.5;
  }

  // Open selected plot device
  cpgopen(plotType);

  // Set the color table
  cpgctab(heat_l, heat_r, heat_g, heat_b, 5, 1.0, 0.5);

  // Compute plotting limits
  freqMin = fCh1;
  freqMax = fCh1 + numChannels * fOff;

  // User wants to see entire contents of file on one page
  if (ts < 0.0 && t0 < 0.0 && tl < 0.0) {
    minPlotTime = 0.0;
    maxPlotTime = sampTime * numSamples;
  // User wants to see first 'tl' seconds of file
  } else if (ts < 0.0 && t0 < 0.0) {
    minPlotTime = 0.0;
    if (tl < sampTime * numSamples) {
      maxPlotTime = tl;
    } else {
      maxPlotTime = sampTime * numSamples;
    }
  // User wants to see from 't0' seconds til end of file
  } else if (ts < 0.0 && tl < 0.0) {
    if (t0 < sampTime * numSamples) {
      minPlotTime = t0;
    } else {
      minPlotTime = 0.0;
    }
    maxPlotTime = sampTime * numSamples;
  // User wants to see from 't0' seconds til 't0 + tl' seconds of file on one page
  } else if (ts < 0.0 && t0 > 0.0 && tl > 0.0) {
    if (t0 < sampTime * numSamples) {
      minPlotTime = t0;
    } else {
      if (tl < numSamples * sampTime) {
        minPlotTime = sampTime * numSamples - tl;
      } else {
        minPlotTime = 0.0;
      }
    }
    if (t0 + tl < sampTime * numSamples) {
      maxPlotTime = t0 + tl;
    } else {
      maxPlotTime = sampTime * numSamples;
    }
  // User wants to see entire contents of file on pages of length 'ts' seconds
  } else if (ts > 0.0 && t0 < 0.0 && tl < 0.0) {
    minPlotTime = 0.0;
    maxPlotTime = sampTime * numSamples;
    multiPage = 1;
  // User wants to see first 'tl' seconds of file on pages of length 'ts' seconds
  } else if (ts > 0.0 && t0 < 0.0) {
    minPlotTime = 0.0;
    if (tl < sampTime * numSamples) {
      maxPlotTime = tl;
    } else {
      maxPlotTime = sampTime * numSamples;
    }
    multiPage = 1;
  // User wants to see from 't0' seconds til end of file on pages of length 'ts' seconds
  } else if (ts > 0.0 && tl < 0.0) {
    if (t0 < sampTime * numSamples) {
      minPlotTime = t0;
    } else {
      minPlotTime = 0.0;;
    }
    maxPlotTime = sampTime * numSamples;
    multiPage = 1;
  // User wants to see from 't0' seconds til 't0 + tl' seconds of file on pages of length 'ts' seconds
  } else if (ts > 0.0 && t0 > 0.0 && tl > 0.0) {
    if (t0 < sampTime * numSamples) {
      minPlotTime = t0;
    } else {
      if (tl < numSamples * sampTime) {
        minPlotTime =  sampTime * numSamples - tl;
      } else {
        minPlotTime = 0.0;
      }
    }
    if (t0 + tl < sampTime * numSamples) {
      maxPlotTime = t0 + tl;
    } else {
      maxPlotTime = sampTime * numSamples;
    }
    multiPage = 1;
  }

  if (multiPage) {

    // Loop until all pages have been displayed
    for (startTime = minPlotTime; startTime < maxPlotTime - ts; startTime += ts) {
  
      // Find the max time to plot based on the current 'startTime' and the requested 'ts'
      endTime = startTime + ts;
  
      // Bring up a plot of 'ts' seconds from 'startTime' to 'endTime'
      cpgpage();
      // Set size of plot window
      cpgsvp(0.1, 0.95, 0.1, 0.95);
      // Map the plot to the plot window, using either channel frequencies or channel indices
      if (channelInfoType == 1) {
        cpgswin(startTime, endTime, freqMin, freqMax);
        cpglab("Time (s)", "Frequency (MHz)", " "); // Label the x- and y-axis; leave the title blank
      } else if (channelInfoType == 2) {
        cpgswin(startTime, endTime, 0, numChannels);
        cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
      }
      // Plot the .fil file in the plot window
      if (grayscale == 0) {
        cpgimag(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a color image
      } else {
        cpggray(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a grayscale image
      }
      // Place axes on plot
      cpgbox("BCTSNI", 0., 0, "BCTSNI", 0., 0);
  
    }

    // Display any plot still remaining
    if (startTime < maxPlotTime) {

      // Compute new plotting limits
      endTime = maxPlotTime;

      // Bring up a plot of the remainder of the file
      cpgpage();
      // Set size of plot window
      cpgsvp(0.1, 0.95, 0.1, 0.95);
      // Map the plot to the plot window, using either channel frequencies or channel indices
      if (channelInfoType == 1) {
        cpgswin(startTime, endTime, freqMin, freqMax);
        cpglab("Time (s)", "Frequency (MHz)", " "); // Label the x- and y-axis; leave the title blank
      } else if (channelInfoType == 2) {
        cpgswin(startTime, endTime, 0, numChannels);
        cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
      }
      // Plot the .fil file in the plot window
      if (grayscale == 0) {
        cpgimag(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a color image
      } else {
        cpggray(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a grayscale image
      }
      // Place axes on plot
      cpgbox("BCTSNI", 0., 0, "BCTSNI", 0., 0);

    }

  } else {
    cpgpage();
    // Set size of plot window
    cpgsvp(0.1, 0.95, 0.1, 0.95);
    // Map the plot to the plot window
    if (channelInfoType == 1) {
      cpgswin(minPlotTime, maxPlotTime, freqMin, freqMax);
      cpglab("Time (s)", "Frequency (MHz)", " "); // Label the x- and y-axis; leave the title blank
    } else if (channelInfoType == 2) {
      cpgswin(minPlotTime, maxPlotTime, 0, numChannels);
      cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
    }
    // Plot the .fil file in the plot window
    if (grayscale == 0) {
      cpgimag(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a color image
    } else {
      cpggray(&data[0], numTimePoints, numChannels, 1, numTimePoints, 1, numChannels, dataMin, dataMax, tr); // Make a grayscale image
    }
    // Place axes on plot
    cpgbox("BCTSNI", 0., 0, "BCTSNI", 0., 0);
  }

  cpgend();

  return 0;

}
