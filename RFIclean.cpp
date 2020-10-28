#include <cstdio>
#include <cstring>
#include <complex>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <time.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <random>

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: RFIclean (-c) (-m maskFile) -f dataFile -o outputFile" << std::endl << std::endl;
  std::cout << "     -c:             Clean the data with MAD" << std::endl;
  std::cout << "     -m maskFile:    Replace data in channels with a constant value (using channel numbers from maskFile)" << std::endl;
  std::cout << "     -n:             Replace data in channels with random noise (using channel numbers from maskFile)" << std::endl << std::endl;
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
// This function calculates the median using a histogram. This is roughly twice as fast as using std::nth_element
void madFunction(const std::vector<float> &data, int numDataPoints, double madOutput[2]) {

  int numHistogramBins = 256; // Number of bins in the histogram, i.e. max value of the data, e.g. 256 for 8-bit data
  int currentTotal = 0, median = 0;
  long *histogram = (long *) calloc(sizeof(long), numHistogramBins);
  float MAD, variance;
  std::vector<float> residualArray;

  // Create a histogram of the data
  for (int i = 0; i < numDataPoints; i++) {
    histogram[(int) data[i]]++;
  }
  
  // Find the median from the histogram
  while (currentTotal < numDataPoints/2) {
    currentTotal += histogram[median];
    median++;
  }

  // Find the difference of each point from the median
  for (int i = 0; i < numDataPoints; i++) {
    residualArray.push_back(std::fabs(data[i] - median));
  }

  // Find the median of the differences
  std::sort(residualArray.begin(), residualArray.end());
  if (numDataPoints%2 == 0) {
    MAD = 0.5 * (residualArray[numDataPoints/2 - 1] + residualArray[numDataPoints/2]);
  } else if (numDataPoints%2 == 1) {
    MAD = residualArray[int(numDataPoints/2 + 0.5)];
  }

  // Calculate the variance
  variance = 1.4826 * MAD;

  // Return the median and variance to an array of size 2
  madOutput[0] = median;
  madOutput[1] = variance;

  free(histogram);
  residualArray.clear();

}

void normalStats(const std::vector<float> &data, int numDataPoints, double statsOutput[2]) {

  int numHistogramBins = 256; // Number of bins in the histogram, i.e. max value of the data, e.g. 256 for 8-bit data
  int currentTotal = 0, dataMedian = 0;
  float dataMean, dataStandardDeviation;
  long *histogram = (long *) calloc(sizeof(long), numHistogramBins);

  // Create a histogram of the data
  for (int i = 0; i < numDataPoints; i++) {
    histogram[(int) data[i]]++;
  }

  // Find the median from the histogram
  while (currentTotal < numDataPoints/2) {
    currentTotal += histogram[dataMedian];
    dataMedian++;
  }

  // Calculate the mean of the data
  dataMean = std::accumulate(data.begin(), data.end(), (float) 0.0)/ (float) numDataPoints;

  // Calculate the standard deviation of this averaged time sample
  dataStandardDeviation = std::sqrt((std::inner_product(data.begin(), data.end(), data.begin(), (float) 0.0)/(float) numDataPoints) - (dataMean * dataMean));

  // Return the median and standard deviation to an array of size 2
  statsOutput[0] = dataMedian;
  statsOutput[1] = dataStandardDeviation;

//  free(histogram);

}

int main (int argc, char *argv[]) {

  int nchar = sizeof(int), numChans = 0, numBits = 0, numIFs = 0, arg, machineID, telescopeID, dataType, numBeams, beamNumber, channel, stride, willCleanData = 0, replaceWithNoise = 0;
  int const nstride = 20;
  int const numSamplesToAverage = 50;
  int const nSigma = 3;
  char string[80], sourceName[80], *header;
  unsigned char eightBitInt;
  long long int numSamples = 0, sample;
  double startTime, sampTime, fch1, foff, stats[2];
  float rand1 = 0, rand2 = 0;
  std::random_device generateRand;
  std::ifstream dataFile, maskFile;
  std::ofstream outputFile;

  // If the user has not provided any arguments or has forgotten to use a flag, print usage and exit
  if (argc < 2) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "cf:m:no:h")) != -1) {
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

      case 'n':
        replaceWithNoise = 1;
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
    dataFile.close();
    outputFile.close();
    exit(0);
  }

  // Check if the output file has failed to open
  if (!outputFile.is_open()) {
    std::cerr << std::endl << "You must input an output file with the -o flag!" << std::endl;
    usage();
    dataFile.close();
    outputFile.close();
    exit(0);
  }

  // Check if the user has not specified any cleaning
  if (willCleanData == 0 && !maskFile.is_open() && replaceWithNoise == 0) {
    std::cout << std::endl << "You haven't asked me to do anything! Exiting..." << std::endl << std::endl;
    dataFile.close();
    outputFile.close();
    exit(0);
  }

  std::cout << "Reading header... " << std::endl;

  // Read header parameters
  while (true) {

    // Read string size
    strcpy(string, "ERROR");
    dataFile.read((char*) &nchar, sizeof(int));
    if (!dataFile) {
      std::cerr << "  Error reading header string size!" << std::endl;
      exit(0);
    }

    // Skip wrong strings
    if (!(nchar > 1 && nchar < 80)) {
      continue;
    }

    // Read string
    dataFile.read((char*) string, nchar);
    if (!dataFile) {
      std::cerr << "  Could not read header string!" << std::endl;
      dataFile.close();
      outputFile.close();
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
      dataFile.read((char*) &sampTime, sizeof(double));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'tsamp' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      dataFile.read((char*) &startTime, sizeof(double));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'tstart' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      dataFile.read((char*) &fch1, sizeof(double));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'fch1' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      dataFile.read((char*) &foff, sizeof(double));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'foff' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      dataFile.read((char*) &numChans, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'nchans' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      dataFile.read((char*) &numIFs, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'nifs' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0 ) {
      dataFile.read((char*) &numBits, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'nbits' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      dataFile.read((char*) &numSamples, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'nsamples' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      dataFile.read((char*) &machineID, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'machine_id' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "telescope_id") == 0) {
      dataFile.read((char*) &telescopeID, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'telescope_id' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "data_type") == 0) {
      dataFile.read((char*) &dataType, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'data_type' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "source_name") == 0) {
      dataFile.read((char*) &nchar, sizeof(int));
      dataFile.read((char*) sourceName, nchar);
      sourceName[nchar] = '\0';
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'source_name' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "nbeams") == 0) {
      dataFile.read((char*) &numBeams, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'nbeams' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else if (strcmp(string, "ibeam") == 0) {
      dataFile.read((char*) &beamNumber, sizeof(int));
      if (!dataFile) {
        std::cerr << "  Did not read header parameter 'ibeam' properly!" << std::endl;
        dataFile.close();
        outputFile.close();
        exit(0);
      }
    } else {
      std::cerr << "  Unknown header parameter " << string << std::endl;
    }

  }

  std::cerr << "done!" << std::endl;

  // Determine the size of the header from the current position in the file
  const size_t headerSize = dataFile.tellg();
  // Seek to the end of the file
  dataFile.seekg(0, dataFile.end);
  // Calculate the size of the data based on the total file size minus the header size
  const size_t bufferSize = (size_t) dataFile.tellg() - headerSize;
  // Calculate how many time samples we have
  numSamples = (long long) (long double) bufferSize/((long double) numBits/8.0)/(long double) numChans;
  // Seek back to the beginning of the file
  dataFile.seekg(0, dataFile.beg);

  // Read header
  header = (char*) malloc(sizeof(char) * headerSize);
  dataFile.read((char*) header, sizeof(char) * headerSize);

  // Instantiate a zero-filled vector to hold data for each time sample
  std::vector<float> timeSamples(numChans, 0);

  // Instantiate a vector to hold the vectors which will contain the data for each time sample
  std::vector<std::vector<float> > data(numSamples, timeSamples);

  // Determine the bit width of the data and read them in appropriately
  // Data are read in as they are stored in the filterbank file, i.e. tsamp_1_chan_1, tsamp_1_chan_2, ..., tsamp_1_chan_N, tsamp_2_chan_1, ...
  switch (numBits) {

    // 8-bit data
    case 8:
      std::cout << "Reading 8-bit data... " << std::flush;
      for (sample = 0; sample < numSamples; sample++) {
        for (channel = 0; channel < numChans; channel++) {
          dataFile.read((char*) &eightBitInt, sizeof(unsigned char));
          if (!dataFile) {
            std::cerr << std::endl << "Could not read 8-bit data properly!" << std::endl;
            dataFile.close();
            outputFile.close();
            exit(0);
          } else {
            data[sample][channel] = (float) eightBitInt;
          }
        }
      }
      std::cout << "done!" << std::endl;
      break;

    // 32-bit data
    case 32:
      std::cout << "Reading 32-bit data... " << std::flush;
      for (sample = 0; sample < numSamples; sample++) {
        // Read each channel for a particular time sample into the corresponding vector for that time sample
        dataFile.read((char*) &data[sample], sizeof(float) * numChans);
        if (!dataFile) {
          std::cerr << "Could not read 32-bit data properly!" << std::endl;
          dataFile.close();
          outputFile.close();
          exit(0);
        }
      }
      std::cout << "done!" << std::endl;
      break;

    default:
      std::cerr << "Cannot read " << numBits << " bit data!" << std::endl << "Data must be 8- or 32-bit!" << std::endl;
      dataFile.close();
      outputFile.close();
      exit(0);

  }

  // Close input file
  dataFile.close();

  // Perform MAD cleaning if the user has requested it (and the file is 8-bit data, as MAD cleaning only works for 8-bit data currently)
  if (willCleanData == 1 && numBits == 8) {

    std::cout << "Cleaning data with MAD... " << std::flush;

    std::vector<float> bufferVecAvg(numChans, 0), vectorOfSampleStdDevs(numSamples, 0), vectorOfSampleMeans(numSamples, 0);

    // Calculate an array of spectral means/standard deviations for each time sample
    for (sample = 0; sample < numSamples; sample++) {
      // Calculate the mean of all channels for this sample
      vectorOfSampleMeans[sample] = std::accumulate(data[sample].begin(), data[sample].end(), (float) 0.0)/(float) numChans;
      // Calculate the standard deviation of channels for this sample
      vectorOfSampleStdDevs[sample] = std::sqrt((std::inner_product(data[sample].begin(), data[sample].end(), data[sample].begin(), (float) 0.0)/(float) numChans) - (vectorOfSampleMeans[sample] * vectorOfSampleMeans[sample]));
    }

    // ------------------- Calculate channel averages -------------------
    int numberOfSamplesAveraged, numberOfTimeSamplesRemaining, numberOfAveragedTimeSamples = ceil((float) (numSamples + nstride - numSamplesToAverage)/(float) nstride);
    std::vector<std::vector<float> > averagedData(numberOfAveragedTimeSamples, timeSamples);

    // Do a moving average over a boxcar of width numSamplesToAverage which moves 'nstride' samples every step
    for (int chunk = 0; chunk < numberOfAveragedTimeSamples; chunk++) {

      // For the last chunk, check how many time samples remain in the data, as it may be fewer than numSamplesToAverage
      if (chunk == (numberOfAveragedTimeSamples - 1)) {

        // We have enough time samples left to sum the full numSamplesToAverage
        if ((chunk * nstride + numSamplesToAverage - 1) < data.size()) {
          // Add together all numSamplesToAverage time samples in this chunk
          for (sample = 0; sample < numSamplesToAverage; sample++) {
            // Add each time sample in this chunk to the (initially-empty) averaged time sample for this chunk
            std::transform(averagedData[chunk].begin(), averagedData[chunk].end(), data[chunk * nstride + sample].begin(), averagedData[chunk].begin(), std::plus<float>());
          }
          // Record how many samples we've actually averaged
          numberOfSamplesAveraged = numSamplesToAverage;
        } else {
          // Calculate the number of time samples remaining in this chunk
          numberOfTimeSamplesRemaining = data.size() - chunk * nstride - 1;
          // Add together the remaining time samples in this chunk
          for (sample = 0; sample < numberOfTimeSamplesRemaining; sample++) {
            // Add each time sample in this chunk to the (initially-empty) averaged time sample for this chunk
            std::transform(averagedData[chunk].begin(), averagedData[chunk].end(), data[chunk * nstride + sample].begin(), averagedData[chunk].begin(), std::plus<float>());
          }
          // Record how many samples we've actually averaged
          numberOfSamplesAveraged = numberOfTimeSamplesRemaining;
        }

      } else { // If this isn't the last chunk of time samples, there will necessarily be numSamplesToAverage time samples left
        // Add together all numSamplesToAverage time samples in this chunk
        for (sample = 0; sample < numSamplesToAverage; sample++) {
          // Add each time sample in this chunk to the (initially-empty) averaged time sample for this chunk
          std::transform(averagedData[chunk].begin(), averagedData[chunk].end(), data[chunk * nstride + sample].begin(), averagedData[chunk].begin(), std::plus<float>());
        }
        // Record how many samples we've actually averaged
        numberOfSamplesAveraged = numSamplesToAverage;
      }

      // Calculate the mean of this chunk by dividing the sum from above by the number of time samples actually added together
      std::transform(averagedData[chunk].begin(), averagedData[chunk].end(), averagedData[chunk].begin(), std::bind(std::divides<float>(), std::placeholders::_1, (float) numberOfSamplesAveraged));

    }

    // ------------------- Frequency-domain MAD cleaning -------------------
    // Initialize random number generator
    srand(time(NULL));

//    for (int x = 0; x < numberOfAveragedTimeSamples; x++) {
//
//      // Fill bufferVecAvg with all channels from an individual time sample
//      for (channel = 0; channel < numChans; channel++) {
//        bufferVecAvg[channel] = averagedData[channel + numChans * x];
//      }
//
//      // Calculate MAD statistics
//      madFunction(bufferVecAvg, numChans, stats);
//
//      for (channel = 0; channel < numChans; channel++) {
//
//        // Determine if this channel in this averaged time sample is more than nSigma * variance away from the median
//        if (bufferVecAvg[channel] > (stats[0] + (nSigma * stats[1])) || bufferVecAvg[channel] < (stats[0] - (nSigma * stats[1]))) {
//
//          // Replace this channel with Gaussian noise
//          for (stride = 0; stride < nstride; stride++) {
//
//            // Generate Gaussian noise and put into buffer[t]
//            rand1 = rand() % 99 + 1;
//            rand2 = rand() % 99 + 1;
//
//            buffer[(x * nstride * numChans) + stride * numChans + channel] = (vectorOfSampleStdDevs[x * nstride + stride] * sqrt(-2 * log(rand1/100)) * cos(2 * M_PI * rand2/100)) + vectorOfSampleMeans[x * nstride + stride];
//
//            // Check that generated data is greater than 0
//            while (buffer[(x * nstride * numChans) + stride * numChans + channel] < 0) {
//
//              rand1 = rand() % 99 + 1;
//              rand2 = rand() % 99 + 1;
//
//              buffer[(x * nstride * numChans) + stride * numChans + channel] = (vectorOfSampleStdDevs[x * nstride + stride] * sqrt(-2 * log(rand1/100)) * cos(2 * M_PI * rand2/100)) + vectorOfSampleMeans[x * nstride + stride];
//
//            }
//
//          }
//
//        }
//
//      }
//
//    }

    // Clean up
    bufferVecAvg.clear();
    averagedData.clear();
    vectorOfSampleStdDevs.clear();
    vectorOfSampleMeans.clear();

    std::cout << "done!" << std::endl;

  } else if (willCleanData == 1 && numBits != 8) {
    std::cout << std::endl << "MAD cleaning currently only works for 8-bit data! Skipping MAD..." << std::endl << std::endl;
  }

  // Replace channels to be masked with Gaussian noise based on the median and standard deviation of the local data
  if (maskFile && replaceWithNoise) {

    std::cout << "Masking channels with Gaussian noise... " << std::flush;

    // Calculate channel averages
    int numberOfSamplesAveraged, numberOfTimeSamplesRemaining, numberOfAveragedTimeSamples = ceil((float) (numSamples + nstride - numSamplesToAverage)/(float) nstride);
    std::vector<std::vector<float> > averagedData(numberOfAveragedTimeSamples, timeSamples);

    // Do a moving average over a boxcar of width numSamplesToAverage which moves 'nstride' time samples every step
    for (long int averagedSample = 0; averagedSample < numberOfAveragedTimeSamples; averagedSample++) {

      // For the last averaged time sample, check how many time samples remain in the data, as it may be fewer than numSamplesToAverage
      if (averagedSample == (numberOfAveragedTimeSamples - 1)) {

        // We have enough time samples left to sum the full numSamplesToAverage
        if ((averagedSample * nstride + numSamplesToAverage - 1) < data.size()) {
          // Add together all numSamplesToAverage time samples in this averaged time sample
          for (sample = 0; sample < numSamplesToAverage; sample++) {
            // Add each time sample in this averaged time sample to the (initially-empty) averaged time sample
            std::transform(averagedData[averagedSample].begin(), averagedData[averagedSample].end(), data[averagedSample * nstride + sample].begin(), averagedData[averagedSample].begin(), std::plus<float>());
          }
          // Record how many time samples we've actually averaged
          numberOfSamplesAveraged = numSamplesToAverage;
        } else {
          // Calculate the number of time samples remaining in this averaged time sample
          numberOfTimeSamplesRemaining = data.size() - averagedSample * nstride - 1;
          // Add together the remaining time samples in this averaged time sample
          for (sample = 0; sample < numberOfTimeSamplesRemaining; sample++) {
            // Add each time sample in this averaged time sample to the (initially-empty) averaged time sample
            std::transform(averagedData[averagedSample].begin(), averagedData[averagedSample].end(), data[averagedSample * nstride + sample].begin(), averagedData[averagedSample].begin(), std::plus<float>());
          }
          // Record how many time samples we've actually averaged
          numberOfSamplesAveraged = numberOfTimeSamplesRemaining;
        }

      } else { // If this isn't the last averaged time sample, there will necessarily be numSamplesToAverage time samples left
        // Add together all numSamplesToAverage time samples in this averaged time sample
        for (sample = 0; sample < numSamplesToAverage; sample++) {
          // Add each time sample in this averaged time sample to the (initially-empty) averaged time sample
          std::transform(averagedData[averagedSample].begin(), averagedData[averagedSample].end(), data[averagedSample * nstride + sample].begin(), averagedData[averagedSample].begin(), std::plus<float>());
        }
        // Record how many time samples we've actually averaged
        numberOfSamplesAveraged = numSamplesToAverage;
      }

      // Calculate the mean of this averaged time sample by dividing the sum from above by the number of time samples actually added together
      std::transform(averagedData[averagedSample].begin(), averagedData[averagedSample].end(), averagedData[averagedSample].begin(), std::bind(std::divides<float>(), std::placeholders::_1, (float) numberOfSamplesAveraged));

      // Calculate the median and standard deviation of the channels in this averaged time sample
      normalStats(averagedData[averagedSample], numChans, stats);

      // Generate a random seed by generating a seed sequence starting with 8 random values
      std::seed_seq randomSeed{generateRand(), generateRand(), generateRand(), generateRand(), generateRand(), generateRand(), generateRand(), generateRand()};
      // Create a Mersenne twister based on the random seed generated above
      std::mt19937 randomNumGenerator(randomSeed);
      // Create a Gaussian distribution with the same median and standard deviation as the local averaged data
      std::normal_distribution<float> distribution(stats[0], stats[1]);

      // Clear the state (i.e. the error flags) of the mask file input stream (e.g. if the failbit is set because we tried to read past the end of the file on a previous read)
      maskFile.clear();
      // Set the input stream position to the beginning of the file
      maskFile.seekg(0);

      // Read each line of the mask file into the 'channel' variable
      while (maskFile >> channel) {
        // Replace this channel with Gaussian noise for all native samples in this averaged time sample
        for (sample = 0; sample < nstride; sample++) {
          data[averagedSample * nstride + sample][channel] = distribution(randomNumGenerator);
        }
      }

    }

    // Clean up
    averagedData.clear();
    maskFile.close();

    std::cout << "done!" << std::endl;

  } else if (maskFile) { // Replace channels to be masked with a constant value
    std::cout << "Masking channels with constant value... " << std::flush;
    // Read each line of the mask file into the 'channel' variable
    while (maskFile >> channel) {
      // Mask the channel in each time sample
      for (sample = 0; sample < numSamples; sample++) {
        // Set this data point to a constant  value
        data[sample][channel] = 128;
      }
    }
    maskFile.close();
    std::cout << "done!" << std::endl;
  }

  // Output .fil files
  outputFile.write(header, sizeof(char) * headerSize);
  switch (numBits) {

    case 8:
    {
      for (sample = 0; sample < numSamples; sample++) {
        std::vector<char> charBuffer(data[sample].begin(), data[sample].end());
        outputFile.write((char*) &charBuffer[0], sizeof(char) * numChans);
      }
      break;
    }

    case 32:
      for (sample = 0; sample < numSamples; sample++) {
        outputFile.write((char*) &data[sample][0], sizeof(float) * numChans);
      }
      break;

  }
  outputFile.close();

  // Clean up
  free(header);
  data.clear();

  return 0;

}
