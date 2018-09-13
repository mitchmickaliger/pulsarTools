#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <getopt.h>

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: filAppender -f firstFile -s secondFile -d maxDMtoSearch" << std::endl << std::endl;
  std::cout << "     -f: First .fil file on which to append data" << std::endl;
  std::cout << "     -s: Second .fil file from which data to append will come" << std::endl;
  std::cout << "     -d: Maximum DM up to which you are planning to search" << std::endl;
  std::cout << "         This is important for calculating how much data to append from the second file to the first" << std::endl << std::endl;
}

/* -- filAppender ---------------------------------------------------------
** Appends some of a second .fil file to the end of an initial .fil file, |
** based on the maximum DM out to which the data will be searched         |
-------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {

  int arg, headerLength, nchar = sizeof(int), numChans = 0, numIFs = 0, numBits = 0, machineID, telescopeID, dataType, flag = 0;
  char *header, *samplesToAdd, *firstFileData, *firstFileName, *secondFileName, *outfileName, string[80];
  long long int firstFileSize, numSamps = 0, samplesToOverlap;
  double sampTime, obsStart, freqChan1, freqOffset, maxDMtoSearch = -1, dmDelay;
  std::ifstream firstFile, secondFile;
  std::ofstream outfile;

  if (argc < 7) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "d:f:s:h")) != -1) {
    switch (arg) {

      case 'd':
        maxDMtoSearch = atof(optarg);
        if (maxDMtoSearch < 0) {
          std::cerr << "Maximum DM to search must be positive!" << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'f':
        // Open first .fil file to read and move the file pointer to the end
        firstFile.open(optarg, std::ifstream::binary | std::ifstream::ate);
        if (!firstFile.is_open()) {
          std::cerr << "Could not open file " << optarg << " to read!" << std::endl;
          exit(0);
        }
        firstFileName = optarg;
        break;

      case 's':
        // Open second .fil file to read
        secondFile.open(optarg, std::ofstream::binary | std::ifstream::ate);
        if (!secondFile.is_open()) {
          std::cerr << "Could not open file " << optarg << " to write!" << std::endl;
          exit(0);
        }
        secondFileName = optarg;
        break;

      case 'h':
        usage();
        exit(0);

      default:
        return 0;
        break;

    }
  }

  // Check if the first file has failed to open or user has not supplied one with the -f flag
  if (!firstFile.is_open()) {
    std::cerr << std::endl << "You must input a .fil file to append to with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  // Check if the second file has failed to open or user has not supplied one with the -s flag
  if (!secondFile.is_open()) {
    std::cerr << std::endl << "You must input a .fil file to append with the -s flag!" << std::endl;
    usage();
    exit(0);
  }

  if (maxDMtoSearch < 0) {
    std::cerr << "You must set a maximum DM to which you plan to search (and it must be positive)!" << std::endl;
    usage();
    exit(0);
  }

  // Name the output file based on the first input file
  outfileName = strcat(firstFileName, ".appended");

  // Since we opened the first file at the end, report the size of the file
  const size_t firstFileLength = firstFile.tellg();

  // Determine how much memory is needed to hold the entire file
  firstFileSize = sizeof(char) * firstFileLength;

  // Allocate memory to hold the first file
  firstFileData = (char*) malloc(firstFileSize);

  // Seek back to the beginning of the file
  firstFile.seekg(0, firstFile.beg);

  // Open output .fil file to write
  outfile.open(outfileName, std::ofstream::binary);
  if (!outfile.is_open()) {
    std::cerr << "Could not open output file " << outfileName << " to write!" << std::endl;
    exit(0);
  }

  // Since we opened the second file at the end, report the size of the file
  const size_t secondFileLength = secondFile.tellg();

  // Seek to beginning of file
  secondFile.seekg(0, secondFile.beg);

  // Read header parameters from second file until "HEADER_END" is encountered
  while (true) {

    // Read string size
    strcpy(string, "ERROR");
    secondFile.read((char*) &nchar, sizeof(int));
    if (!secondFile) {
      std::cerr << "Error reading header string size!" << std::endl;
      exit(0);
    }

    // Skip wrong strings
    if (!(nchar > 1 && nchar < 80)) {
      continue;
    }

    // Read string
    secondFile.read((char*) string, nchar);
    if (!secondFile) {
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
      secondFile.read((char*) &sampTime, sizeof(double));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'tsamp' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      secondFile.read((char*) &obsStart, sizeof(double));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'tstart' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      secondFile.read((char*) &freqChan1, sizeof(double));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'fch1' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      secondFile.read((char*) &freqOffset, sizeof(double));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'foff' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      secondFile.read((char*) &numChans, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'nchans' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      secondFile.read((char*) &numIFs, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'nifs' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0) {
      secondFile.read((char*) &numBits, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'nbits' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      secondFile.read((char*) &numSamps, sizeof(long long));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'nsamples' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      secondFile.read((char*) &machineID, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'machine_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "telescope_id") == 0) {
      secondFile.read((char*) &telescopeID, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'telescope_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "data_type") == 0) {
      secondFile.read((char*) &dataType, sizeof(int));
      if (!secondFile) {
        std::cerr << "Did not read header parameter 'data_type' properly!" << std::endl;
        exit(0);
      }
    } else {
      std::cerr << "Unknown header parameter " << string << std::endl;
    }

  }

  // Set the header length to the current position in the file, since we just encountered "HEADER_END"
  headerLength = secondFile.tellg();

  const size_t dataSize = secondFileLength - headerLength;

  // Seek to beginning of file
  secondFile.seekg(0, secondFile.beg);

  // Allocate memory to store the header of the second .fil file
  header = (char*) malloc(headerLength);

  // Max dispersion delay across the observing band, for a given DM
  dmDelay = maxDMtoSearch * 4150.0 * (1.0/pow((freqChan1 + (freqOffset * numChans)), 2) - 1.0/pow(freqChan1, 2));

  samplesToOverlap = dmDelay * numChans/sampTime;

  // Allocate memory for data from the second file
  // If the second file is shorter than the overlap required, allocate memory for the entire second file
  // Set a flag so we know how many samples to read later
  if (dataSize < samplesToOverlap) {
    samplesToAdd = (char*) malloc(sizeof(char) * dataSize);
    flag = 1;
  } else {
    samplesToAdd = (char*) malloc(sizeof(char) * samplesToOverlap);
    flag = 2;
  }

  // Read the header from the second file into memory
  secondFile.read(header, headerLength);
  if (!secondFile) {
    std::cerr << "Could not read header from file " << secondFileName << "!" << std::endl;
    exit(0);
  }

  
  if (flag == 1) {
    // Read all of the data from the second file
    secondFile.read(samplesToAdd, dataSize);
    if (!secondFile) {
      std::cerr << "Could not read " << (dataSize/numChans) * sampTime << " seconds of data from file " << secondFileName << "!" << std::endl;
      exit(0);
    }
  } else if (flag == 2) {
    // Read required amount of data from the beginning of the second file
    secondFile.read(samplesToAdd, samplesToOverlap);
    if (!secondFile) {
      std::cerr << "Could not read " << (samplesToOverlap/numChans) * sampTime << " seconds of data from file " << secondFileName << "!" << std::endl;
      exit(0);
    }
  } else {
    std::cerr << "Something is wrong! Check size of " << secondFileName << "!" << std::endl;
  }

  // Read the entire first file into memory
  firstFile.read(firstFileData, firstFileSize);
  if (!firstFile) {
    std::cerr << "Could not read data from file " << firstFileName << "!" << std::endl;
    exit(0);
  }

  // Write the entire first file to the output
  outfile.write(firstFileData, firstFileSize);
  if (!outfile) {
    std::cerr << "Could not write data from file " << firstFileName << " to output file " << outfileName << "!" << std::endl;
    exit(0);
  }

  if (flag == 1) {
    // Append all data from the second file
    outfile.write(samplesToAdd, sizeof(char) * dataSize);
    if (!outfile) {
      std::cerr << "Could not write " << (dataSize/numChans) * sampTime << " seconds of data from file " << secondFileName << " to output file" << outfileName << "!" << std::endl;
      exit(0);
    }
  } else if (flag == 2) {
    // Append required data from the second file
    outfile.write(samplesToAdd, sizeof(char) * samplesToOverlap);
    if (!outfile) {
      std::cerr << "Could not write " << (samplesToOverlap/numChans) * sampTime << " seconds of data from file " << secondFileName << " to output file " << outfileName << "!" << std::endl;
      exit(0);
    }
  }

  firstFile.close();
  secondFile.close();
  outfile.close();

}
