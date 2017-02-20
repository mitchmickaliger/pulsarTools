#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include"cpgplot.h"
#include<getopt.h>

#define LIM 256

/*
 Plots the spectrum from a filterbank file, showing either channel frequency vs time or channel index vs time.
 Will average in time, but not in frequency (yet).
 Will currently work with 8-bit or 32-bit data. Functionality to plot 1-bit, 2-bit, 4-bit, and 16-bit added, but needs to be tested.
*/

// External function to print help if needed
int usage() {
  fprintf(stderr, "\nUsage: plotfil (-options) fil_file\n\n");
  fprintf(stderr, "     -b: Number of time samples to bin (default = 1)\n");
  fprintf(stderr, "     -c: Either plot channel frequencies (1) or channel indices (2) (default = 1)\n");
  fprintf(stderr, "     -d: DM to dedsiperse before plotting (default = 0)\n");
  fprintf(stderr, "     -g: Output plot type (default = /xs)\n");
  fprintf(stderr, "     -S: Time at which to begin plotting (default = start of file)\n");
  fprintf(stderr, "     -T: Seconds of data to plot, from start of file or requested start point (default = til end of file)\n");
  fprintf(stderr, "     -t: Time chunk (in seconds) to plot (default = entire file)\n\n");
}

int main(int argc, char *argv[]) {

  FILE *file;
  char string[80], *header, plottype[LIM] = "/xs";
  unsigned char *buffer;
  int i, nchar = sizeof(int), numBins = 1, numChans = 0, numBits = 0, numIFs = 0, arg, multiPage = 0, sampsRead, *dmDelaySamps;
  int telescope_id, data_type, machine_id, bit, channelInfoType = 1;
  unsigned short sixteenBitInt;
  unsigned char charOfValues;
  double obsStart, sampTime, fch1, foff;
  long long numSamples = 0, header_size, buffer_size, sample, channel, sampleIndex, extremaIndex, arrayIndex, numXPoints, numDataPoints, bin;
  float *data, dataMin, dataMax, frequency, dm = 0.0, dmDelay, bitsRead[8];
  float minPlotTime, maxPlotTime, freqMin, freqMax, t0 = -1.0, tl = -1.0, ts = -1.0, startTime, endTime;
  float tr[] = {-0.5, 1.0, 0.0, -0.5, 0.0, 1.0};
  float heat_l[] = {0.0, 0.2, 0.4, 0.6, 1.0}, heat_r[] = {0.0, 0.5, 1.0, 1.0, 1.0}, heat_g[] = {0.0, 0.0, 0.5, 1.0, 1.0}, heat_b[] = {0.0, 0.0, 0.0, 0.3, 1.0};

  // If the user has not provided any arguments, print usage and exit
  if (argc < 2) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "b:c:d:g:S:T:t:h")) != -1) {
    switch (arg) {

      case 'b':
        numBins = atoi(optarg);
        break;

      case 'c':
        channelInfoType = atoi(optarg);
        if (channelInfoType < 1 || channelInfoType > 2) {
          fprintf(stderr, "\nYou must either choose to plot channel frequencies (-c 1) or channel indices (-c 2)!\n");
          usage();
          exit(0);
        }
        break;

      case 'd':
        dm = atof(optarg);
        break;

      case 'g':
        strcpy(plottype, optarg);
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

      case 'h':
        usage();
        exit(0);

      default:
        return 0;
        break;

    }
  }

  // Open file to read
  if ((file = fopen(argv[optind], "r")) == NULL) {
    fprintf(stderr, "Could not open file `%s to read\n", argv[argc - 1]);
    usage();
    exit(0);
  }

  // Read header parameters
  for (;;) {

    // Read string size
    strcpy(string, "ERROR");
    if (fread(&nchar, sizeof(int), 1, file) != 1) {
      fprintf(stderr, "Error reading header string size!\n");
      exit(0);
    }

    // Skip wrong strings
    if (!(nchar > 1 && nchar < 80)) {
      continue;
    }

    // Read string
    if (fread(string, nchar, 1, file) != 1) {
      fprintf(stderr, "Could not read header string!\n");
      exit(0);
    }
    string[nchar] = '\0';

    // Exit at end of header
    if (strcmp(string, "HEADER_END") == 0) {
      break;
    }

    // Read parameters
    if (strcmp(string, "tsamp") == 0) {
      if (fread(&sampTime, sizeof(double), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'tsamp' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      if (fread(&obsStart, sizeof(double), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'tstart' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      if (fread(&fch1, sizeof(double), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'fch1' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      if (fread(&foff, sizeof(double), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'foff' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      if (fread(&numChans, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'nchans' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      if (fread(&numIFs, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'nifs' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0) {
      if (fread(&numBits, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'nbits' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      if (fread(&numSamples, sizeof(long), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'nsamples' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      if (fread(&machine_id, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'machine_id' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "telescope_id") == 0) {
      if (fread(&telescope_id, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'telescope_id' properly!\n");
        exit(0);
      }
    } else if (strcmp(string, "data_type") == 0) {
      if (fread(&data_type, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Did not read header parameter 'data_type' properly!\n");
        exit(0);
      }
    }

  }

  // Get the header size from the current value of the position indicator in the file
  header_size = (int) ftell(file);
  // Move the position indicator to the end of the file
  fseek(file, 0, SEEK_END);
  // Get the total size of the data based on the header size and the value of the position indicator at the end of the file
  buffer_size = ftell(file) - header_size;
  // Calculate how many samples are in the file
  numSamples = (long long) (long double) buffer_size/((long double) numBits/8.0)/(long double) numChans;
  // Move the position indicator back to the beginning
  rewind(file);

  // Allocate room for and read the header
  header = (char *) malloc(sizeof(char) * header_size);
  if (fread(header, sizeof(char), header_size, file) != header_size) {
    fprintf(stderr, "Did not read header properly!\n");
    exit(0);
  }

  // Allocate room for the data buffer
  buffer = (unsigned char *) malloc(sizeof(unsigned char) * buffer_size);

    // Determine the bit width of the data and read them in appropriately
    switch (numBits) {
  
//      // 1-bit data
//      case 1:
//        // Read one byte of data (eight 1-bit values) from the input file
//        if (fread(&charOfValues, 1, 1, file) != 1) {
//          fprintf(stderr, "Could not read 1-bit values properly!\n");
//          exit(0);
//        }
//        // One byte contains eight data values, so read each separately
//        // Data are stored with the first value in the lowest bit, i.e. the right-most bit
//        for (bit = 0; bit < 8; bit++) {
//          // Perform a bitwise 'AND' operation between variable 'charOfValues' and decimal 1 (00000001 in binary)
//          // This will return the value of the right-most bit of 'charOfValues'
//          bitsRead[bit] = charOfValues & 1;
//          // Shift the bits in variable 'charOfValues' one to the right
//          charOfValues >>= 1;
//        }
//        // Record how many samples were read from this byte
//        sampsRead = 8;
//        break;
//
//      // 2-bit data
//      case 2:
//        // Read one byte of data (four 2-bit values) from the input file
//        if (fread(&charOfValues, 1, 1, file) != 1) {
//          fprintf(stderr, "Could not read 2-bit values properly!\n");
//          exit(0);
//        }
//        // Perform bitwise 'AND' operations to pull out the correct two bits for each value
//        // Shift the bits to the lowest two bits so they are read as floats properly
//        bitsRead[0] = (float) (charOfValues & 3);            /* 3 =   00000011 in binary */
//        bitsRead[1] = (float) ((charOfValues & 12) >> 2);    /* 12 =  00001100 in binary */
//        bitsRead[2] = (float) ((charOfValues & 48) >> 4);    /* 48 =  00110000 in binary */
//        bitsRead[3] = (float) ((charOfValues & 192) >> 6);   /* 192 = 11000000 in binary */
//        // Record how many samples were read from this byte
//        sampsRead = 4;
//        break;
//
//      // 4-bit data
//      case 4:
//         // Read one byte of data (2 4-bit values) from the input file
//        if (fread(&charOfValues, 1, 1, file) != 1) {
//          fprintf(stderr, "Could not read 4-bit values properly!\n");
//          exit(0);
//        }
//        // Perform bitwise 'AND' operations to pull out the correct four bits for each value
//        // Shift the bits to the lowest four bits so they are read as floats properly
//        bitsRead[0] = (float) (charOfValues & 15);         /* 15 =  00001111 in binary */
//        bitsRead[1] = (float) ((charOfValues & 240) >> 4); /* 240 = 11110000 in binary */
//        // Record how many samples were read from this byte
//        sampsRead = 2;
//        break;
  
      // 8-bit data
      case 8:
        if (fread(buffer, sizeof(unsigned char), buffer_size, file) != buffer_size) {
          fprintf(stderr, "Could not read 8-bit data properly!\n");
          exit(0);
        }
        break;
  
//      // 16-bit data
//      case 16:
//        // Read two bytes of data from the input file
//        if (fread(&sixteenBitInt, 2, 1, file) != 1) {
//          fprintf(stderr, "Could not read 16-bit data properly!\n");
//          exit(0);
//        }
//        // Store the value read as a float
//        bitsRead[0] = (float) sixteenBitInt;
//        // Record how many samples were read from these bytes
//        sampsRead = 1;
//        break;
  
      // 32-bit data
      case 32:
        for (i = 0; i < numSamples * numChans; i++) {
          // Read an int from the input file 
          if (fread(&bitsRead[0], 4, 1, file) == 1) {
            // Store the int in the buffer 
            buffer[i] = bitsRead[0];
          } else {
            fprintf(stderr, "Could not read 32-bit data properly!\n");
            exit(0);
          }
        }
        break;
  
      default:
        fprintf(stderr, "Cannot read %d bit data!\nData must be 1-, 2-, 4-, 8-, 16-, or 32-bit!\n", numBits);
        exit(0);

    }

  // Close file
  fclose(file);

  // Compute dispersion shifts
  dmDelaySamps = (int *) malloc(sizeof(int) * numChans);
  for (channel = 0; channel < numChans; channel++) {
    frequency = fch1 + (float) channel * foff;
    dmDelay = 4.148808e3 * (pow(frequency, -2) - pow(fch1, -2)) * dm;
    dmDelaySamps[channel] = (int) floor(dmDelay/sampTime + 0.5);
  }

  // Fill floating point buffer
  numXPoints = (long long) ((float) numSamples/(float) numBins);
  data = (float *) malloc(sizeof(float) * numXPoints * numChans);

  for (sample = 0; sample < numXPoints; sample++) {
    for (channel = 0; channel < numChans; channel++) {
      sampleIndex = sample + numXPoints * channel;
      data[sampleIndex] = 0.0;
      numDataPoints = 0;
      for (bin = 0; bin < numBins; bin++) {
        arrayIndex = (numChans * (numBins * sample + bin + dmDelaySamps[channel]) + channel);
        if (arrayIndex >= numChans * numSamples || arrayIndex < 0) {
          continue;
        }
        data[sampleIndex] += (float) buffer[arrayIndex];
        numDataPoints++;
      }
      if (data[sampleIndex] != 0) {
        data[sampleIndex] /= (float) numDataPoints;
      }
    }
  }

  // Find extrema
  for (extremaIndex = 0; extremaIndex < numXPoints * numChans; extremaIndex++) {
    if (extremaIndex == 0) {
      dataMin = data[extremaIndex];
      dataMax = data[extremaIndex];
    } else {
      if (data[extremaIndex] < dataMin) dataMin = data[extremaIndex];
      if (data[extremaIndex] > dataMax) dataMax = data[extremaIndex];
    }
  }

  // Transformation matrix used by cpgimag to map data onto plot
  tr[1] = sampTime * numSamples/(float) numXPoints;
  tr[2] = 0.0;
  tr[0] = -0.5 * tr[1];
  tr[4] = 0.0;
  if (channelInfoType == 1) { // If plotting channel frequencies, center first Y-bin on fch1
    tr[5] = foff * numChans/(float) numChans;
    tr[3] = fch1 - 0.5 * tr[5];
  } else if (channelInfoType == 2) { // If plotting channel indices, center first Y-bin on 0.5
    tr[5] = 1.0;
    tr[3] = 0.5;
  }

  // Open selected plot device
  cpgopen(plottype);
  // Set the color table
  cpgctab(heat_l, heat_r, heat_g, heat_b, 5, 1.0, 0.5);

  // Compute plotting limits
  freqMin = fch1;
  freqMax = fch1 + numChans * foff;

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
        cpgswin(startTime, endTime, 0, numChans);
        cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
      }
      // Plot the .fil file in the plot window
      cpgimag(data, numXPoints, numChans, 1, numXPoints, 1, numChans, dataMin, dataMax, tr);
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
        cpgswin(startTime, endTime, 0, numChans);
        cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
      }
      // Plot the .fil file in the plot window
      cpgimag(data, numXPoints, numChans, 1, numXPoints, 1, numChans, dataMin, dataMax, tr);
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
      cpgswin(minPlotTime, maxPlotTime, 0, numChans);
      cpglab("Time (s)", "Channel Number", " "); // Label the x- and y-axis; leave the title blank
    }
    // Plot the .fil file in the plot window
    cpgimag(data, numXPoints, numChans, 1, numXPoints, 1, numChans, dataMin, dataMax, tr);
    // Place axes on plot
    cpgbox("BCTSNI", 0., 0, "BCTSNI", 0., 0);
  }

  cpgend();

  // Free allocated memory
  free(header);
  free(buffer);
  free(data);
  free(dmDelaySamps);

  return 0;

}
