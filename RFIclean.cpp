#include <cstdio>
#include <cstring>
#include <complex>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: RFIclean (-c) (-m maskFile) -f dataFile -o outputFile" << std::endl << std::endl;
  std::cout << "     -c:             Clean the data with MAD" << std::endl;
  std::cout << "     -m maskFile:    Mask the data using channel numbers from maskFile" << std::endl << std::endl;
  std::cout << "NB: you can specify -c, -m, or both. Specifying neither is pointless, as this will do nothing." << std::endl << std::endl;
}

/* -- RFIclean -----------------------------------------------------------------------------------------------------------------------
** Masks and runs MAD cleaning on filterbank data.                                                                                   |
**                                                                                                                                   |
** Currently set up for 8-bit data (numHistogramBins = 256 hardcoded below). Can be run for other data if this parameter is changed. |
**                                                                                                                                   |
** NOTE: the histogram method used to find the median is only fast for a reasonable number of bins, e.g.                             |
** for 32-bit data, the histogram would have 4294967296 bins and the code will be incredibly slow!                                   |
----------------------------------------------------------------------------------------------------------------------------------- */

// Function to calculate the Median Absolute Difference (MAD) of numDataPoints
void madFunction(std::vector<float> data, int numDataPoints, double madOutput[2]) {

  int numHistogramBins = 256; // Number of bins i the histogram, i.e. max value of the data, e.g. 256 for 8-bit data
  int currentTotal = 0, median = 0;
  long *histogram = (long *) calloc(sizeof(long), numHistogramBins);
  float MAD, variance;
  std::vector<float> residualArray;

  // Create a histogram of the data
  for (int i = 0; i < numDataPoints; i++) {
    histogram[(int) data.at(i)]++;
  }
  
  // Find the median from the histogram
  while (currentTotal < numDataPoints/2) {
    currentTotal += histogram[median];
    median++;
  }

  // Find the difference of each point from the median
  for (int i = 0; i < numDataPoints; i++) {
    residualArray.push_back(std::fabs(data.at(i) - median));
  }

  // Find the median of the differences
  std::sort(residualArray.begin(), residualArray.end());
  if (numDataPoints%2 == 0) {
    MAD = 0.5 * (residualArray.at(numDataPoints/2 - 1) + residualArray.at((numDataPoints/2)));
  } else if (numDataPoints%2 == 1) {
    MAD = residualArray.at(int(numDataPoints/2 + 0.5));
  }

  // Calculate the variance
  variance = 1.4826 * MAD;

  // Return the median and variance to an array of size 2
  madOutput[0] = median;
  madOutput[1] = variance;

  residualArray.clear();

}

int main (int argc, char *argv[]) {

  char string[80], *header, *buffer;
  int nchar = sizeof(int), numChans = 0, numBits = 0, numIFs = 0, arg, machineID, channel, index = 0, willCleanData = 0;
  int stride, nstride = 10, numSampsToAverage = 50, nSigma = 3;
  long long int numSamps = 0, sample;
  double startTime, sampTime, fch1, foff, stats[2];
  float rand1 = 0, rand2 = 0;
  std::ifstream dataFile, maskFile;
  std::ofstream outputFile;

  // If the user has not provided any arguments or has forgotten to use a flag, print usage and exit
  if (argc < 2) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "cf:m:o:h")) != -1) {
    switch (arg) {

      case 'c':
        willCleanData = 1;
        break;

      case 'f':
        dataFile.open(optarg, std::ifstream::binary);
        if (!dataFile.is_open()) {
          std::cerr << "Could not open file " << optarg << " to read!" << std::endl;
          exit(0);
        }
        break;

      case 'm':
        maskFile.open(optarg);
        if (!maskFile.is_open()) {
          std::cerr << "Could not open file " << optarg << " to read!" << std::endl;
          exit(0);
        }
        break;


      case 'o':
        outputFile.open(optarg, std::ofstream::binary);
        if (!outputFile.is_open()) {
          std::cerr << "Could not open file " << optarg << " to write!" << std::endl;
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

  // Check if the input file has failed to open
  if (!dataFile.is_open()) {
    std::cerr << std::endl << "You must input a fil file with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  // Check if the output file has failed to open
  if (!outputFile.is_open()) {
    std::cerr << std::endl << "You must input an output file with the -o flag!" << std::endl;
    usage();
    exit(0);
  }

  // Check if the user has not specified any cleaning
  if (willCleanData == 0 && !maskFile.is_open()) {
    std::cout << std::endl << "You haven't asked me to do anything! Exiting..." << std::endl << std::endl;
    dataFile.close();
    outputFile.close();
    exit(0);
  }

  // Read header parameters
  while (true) {

    // Read string size
    strcpy(string, "ERROR");
    dataFile.read((char*) &nchar, sizeof(int));
    if (!dataFile) {
      std::cerr << "Error reading header string size!" << std::endl;
      exit(0);
    }

    // Skip wrong strings
    if (!(nchar > 1 && nchar < 80)) {
      continue;
    }

    // Read string
    dataFile.read((char*) string, nchar);
    if (!dataFile) {
      std::cerr << "Could not read header string!" << std::endl;
      exit(0);
    }
    string[nchar] = '\0';

    // Exit at end of header
    if (strcmp(string, "HEADER_END") == 0) {
      break;
    }

    // Read parameters
    if (strcmp(string, "tsamp") == 0) {
      dataFile.read((char*) &sampTime, sizeof(double));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'tsamp' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      dataFile.read((char*) &startTime, sizeof(double));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'tstart' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      dataFile.read((char*) &fch1, sizeof(double));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'fch1' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      dataFile.read((char*) &foff, sizeof(double));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'foff' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      dataFile.read((char*) &numChans, sizeof(int));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'nchans' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      dataFile.read((char*) &numIFs, sizeof(int));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'nifs' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0 ) {
      dataFile.read((char*) &numBits, sizeof(int));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'nbits' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      dataFile.read((char*) &numSamps, sizeof(int));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'nsamples' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      dataFile.read((char*) &machineID, sizeof(int));
      if (!dataFile) {
        std::cerr << "Did not read header parameter 'machine_id' properly!" << std::endl;
        exit(0);
      }
    } else {
      std::cerr << "Unknown header parameter " << string << std::endl;
    }

  }

  // Determine the size of the header from the current position in the file
  const size_t headerSize = dataFile.tellg();
  // Seek to the end of the file
  dataFile.seekg(0, dataFile.end);
  // Calculate the size of the data based on the total file size minus the header size
  const size_t bufferSize = dataFile.tellg() - headerSize;
  // Calculate how many time samples we have
  numSamps = bufferSize/numChans;
  // Seek back to the beginning of the file
  dataFile.seekg(0, dataFile.beg);

  // Read header
  header = (char*) malloc(sizeof(char) * headerSize);
  dataFile.read((char*) header, sizeof(char) * headerSize);

  // Read buffer
  buffer = (char*) malloc(sizeof(char) * bufferSize);
  dataFile.read((char*) buffer, sizeof(char) * bufferSize);

  // Close input file
  dataFile.close();

  // Perform MAD cleaning if the user has requested it
  if (willCleanData == 1) {

    std::vector<float> bufferVecAvg(numChans, 0), vectorOfSampleStdDevs(numSamps, 0), vectorOfSampleMeans(numSamps, 0);

    // Calculate an array of spectral means/standard deviations for each time sample
    for (sample = 0; sample < numSamps; sample++) {
      float sum = 0, sumSquaredResiduals = 0;
      for (channel = 0; channel < numChans; channel++) {
        sum += buffer[sample * numChans + channel]; // Add all channels for the current sample, like dedispersing at DM = 0
      }
      vectorOfSampleMeans[sample] = sum/numChans; // Calculate the mean of all channels for this sample
      for (channel = 0; channel < numChans; channel++) {
        sumSquaredResiduals += pow(buffer[sample * numChans + channel] - vectorOfSampleMeans[sample], 2);
      }
      vectorOfSampleStdDevs[sample] = sqrt(sumSquaredResiduals/numChans); // Calculate the standard deviation of channels for this sample
    }

    /********************************* Calculate channel averages *****************************************/

    float sumOfTimeSamples;
    int xMAX = numSamps + nstride - numSampsToAverage; 
    float *buffer_avg = (float *) malloc(sizeof(float) * int(ceil(float(xMAX)/float(nstride)) * numChans));

    // Do a moving average over a boxcar of width numSampsToAverage which moves nstride samples every step
    for (int chunk = 0; chunk < ceil(float(xMAX)/float(nstride)); chunk++) {
      for (channel = 0; channel < numChans; channel++) {
        sumOfTimeSamples = 0;
        if ((((numSampsToAverage - 1) * numChans) + (chunk * nstride * numChans)) < (numSamps * numChans)) {
          for (int a = 0; a < numSampsToAverage; a++){
            sumOfTimeSamples += buffer[(a * numChans) + channel + (chunk * nstride * numChans)]; // Average together numSampsToAverage time samples in each channel
          }
        } else {
          numSampsToAverage = ((numSamps * numChans) - (chunk * nstride * numChans) + 1)/numChans;
          for (int a = 0; a < numSampsToAverage; a++){
            sumOfTimeSamples += buffer[(a * numChans) + channel + (chunk * nstride * numChans)]; // Average together the remaining time samples in each channel
          }
        }
        buffer_avg[channel + chunk * numChans] = sumOfTimeSamples/float(numSampsToAverage);
      }
    }

    /***************************************** F-DOMAIN MAD CLEANING **************************************/

    // Initialize random number generator
    srand(time(NULL));

    // Calculate how many time samples remain after averaging
    int numSampsAvg = int(ceil(float(xMAX)/float(nstride)));

    for (int x = 0; x < numSampsAvg; x++) {

      // Fill bufferVecAvg with all channels from an individual time sample
      for (channel = 0; channel < numChans; channel++) {
        bufferVecAvg[channel] = buffer_avg[channel + numChans * x];
      }

      // Calculate MAD statistics
      madFunction(bufferVecAvg, numChans, stats);

      for (channel = 0; channel < numChans; channel++) {

        // Determine if this channel in this averaged time sample is more than nSigma * variance away from the median
        if (bufferVecAvg[channel] > (stats[0] + (nSigma * stats[1])) || bufferVecAvg[channel] < (stats[0] - (nSigma * stats[1]))) {

          // Replace this channel with Gaussian noise
          for (stride = 0; stride < nstride; stride++) {

            // Generate Gaussian noise and put into bufferVecAvg[t]
            rand1 = rand() % 99 + 1;
            rand2 = rand() % 99 + 1;

            buffer[(x * nstride * numChans) + stride * numChans + channel] = (vectorOfSampleStdDevs[x * nstride + stride] * sqrt(-2 * log(rand1/100)) * cos(2 * M_PI * rand2/100)) + vectorOfSampleMeans[x * nstride + stride];

            // Check that generated data is greater than 0
            while (buffer[(x * nstride * numChans) + stride * numChans + channel] < 0) {

              rand1 = rand() % 99 + 1;
              rand2 = rand() % 99 + 1;

              buffer[(x * nstride * numChans) + stride * numChans + channel] = (vectorOfSampleStdDevs[x * nstride + stride] * sqrt(-2 * log(rand1/100)) * cos(2 * M_PI * rand2/100)) + vectorOfSampleMeans[x * nstride + stride];

            }

          }

        }

      }

    }

    // Clean up
    free(buffer_avg);
    vectorOfSampleStdDevs.clear();
    vectorOfSampleMeans.clear();

  }

  // Mask bad channels
  if (maskFile) {
    while (maskFile >> channel) {
      for (sample = 0; sample < numSamps; sample++) {
        index = channel + numChans * sample;
        buffer[index] = 128;
      }
    }
    maskFile.close();
  }

  // Output .fil files
  outputFile.write(header, sizeof(char) * headerSize);
  outputFile.write(buffer, sizeof(char) * bufferSize);
  outputFile.close();

  // Clean up
  free(buffer);
  free(header);

  return 0;

}
