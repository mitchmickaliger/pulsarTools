#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cmath>
#include<iostream>
#include<fstream>
#include<getopt.h>

// ADD FUNCTIONALITY TO REMOVE PARAMETERS

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: filEdit -F filFile -options" << std::endl << std::endl;
  std::cout << "Header options that can be modified:" << std::endl;
  std::cout << "    -a azStart     : change the starting azimuth to 'azStart' (degrees)" << std::endl;
  std::cout << "    -b beamNum     : change beam number to 'beamNum'" << std::endl;
  std::cout << "    -B nBeams      : change number of beams to 'nBeams'" << std::endl;
  std::cout << "    -c nChans      : change number of channels to 'nChans'" << std::endl;
  std::cout << "    -d DEC         : change dec to 'DEC' (ddmmss.xxx)" << std::endl;
  std::cout << "    -f fCh1        : change frequency of first channel to 'fCh1' (MHz)" << std::endl;
  std::cout << "    -F filFile     : .fil file to edit" << std::endl;
  std::cout << "    -i nBits       : change number of bits to 'nBits'" << std::endl;
  std::cout << "    -m machineID   : change machine ID to 'machineID'" << std::endl;
  std::cout << "    -n srcName     : change source name to 'srcName'" << std::endl;
  std::cout << "    -o fOff        : change channel bandwidth to 'fOff' (MHz)" << std::endl;
  std::cout << "    -p tSamp       : change sampling time to 'tSamp' (seconds)" << std::endl;
  std::cout << "    -r RA          : change ra to 'RA' (hhmmss.xxx)" << std::endl;
  std::cout << "    -t telID       : change telescope ID to 'telID'" << std::endl;
  std::cout << "    -T newMJD      : change start MJD to 'newMJD'" << std::endl;
  std::cout << "    -z zenAngStart : change the starting zenith angle to 'zenAngStart' (degrees)" << std::endl << std::endl;
}

/* -- filEdit ----------------------------------------------------------------------------------------------------------
** Edits a filterbank file header in place.                                                                            |
**                                                                                                                     |
** This provides similar functionality to SIGPROC's filedit, but with extended functionality and as a standalone tool. |
--------------------------------------------------------------------------------------------------------------------- */
int main (int argc, char* argv[]) {

  int arg, headerStringLength, headerLength, intHeaderParameterLength, dataType, telescopeID, machineID, numIFs = 0, numBits = 0, numChans = 0;
  int newTelescopeID = -1, newMachineID = -1, newBeamNumber = -1, newNumBeams = -1, newNumBits = 0, newNumChans = 0, newSourceNameLength;
  double doubleHeaderParameterLength, RA, Dec, startTime, samplingTime, fCh1, fOff, azimuthStart, zenithAngleStart;
  double newRA = 900000001, newDec = 900000001, newStartTime = 900000001, newSamplingTime = -1, newFCh1 = 0, newFOff = 0, newAzimuthStart = -1, newZenithAngleStart = -1;
  long long numSamples = 0;
  char string[80], newSourceName[100], *headerArray, *headerPointer, headerParameter[100];
  std::fstream file;

  newSourceName[0] = '\0';

  while ((arg = getopt(argc, argv, "a:b:B:c:d:f:F:hi:m:n:o:p:r:t:T:z:")) != -1) {
    switch (arg) {

      case 'a':
        newAzimuthStart = atof(optarg);
        break;

      case 'b':
        newBeamNumber = atoi(optarg);
        break;

      case 'B':
        newNumBeams = atoi(optarg);
        break;

      case 'c':
        newNumChans = atoi(optarg);
        break;

      case 'd':
        newDec = atof(optarg);
        break;

      case 'f':
        newFCh1 = atof(optarg);
        break;

      case 'F':
        file.open(argv[optind - 1], std::fstream::in | std::fstream::out | std::fstream::binary);
        if (!file.is_open()) {
          std::cerr << "Failed to open file " << argv[optind - 1] << " for reading and writing!" << std::endl;
          exit(0);
        }
        break;

      case 'i':
        newNumBits = atoi(optarg);
        break;

      case 'm':
        newMachineID = atoi(optarg);
        break;

      case 'n':
        strcpy(newSourceName, optarg);
        break;

      case 'o':
        newFOff = atof(optarg);
        break;

      case 'p':
        newSamplingTime = atof(optarg);
        break;

      case 'r':
        newRA = atof(optarg);
        break;

      case 't':
        newTelescopeID = atoi(optarg);
        break;

      case 'T':
        newStartTime = atof(optarg);
        break;

      case 'z':
        newZenithAngleStart = atof(optarg);
        break;

      case 'h':
        usage();
        exit(0);
        break;

      default:
        return 0;
        break;

    }
  }

  // Check if the file has failed to open or user has not supplied one with the -F flag
  if (!file.is_open()) {
    std::cerr << std::endl << "You must input a .fil file with the -F flag!" << std::endl;
    usage();
    exit(0);
  }

  // Get the length of the new source name (will be zero if not set)
  newSourceNameLength = strlen(newSourceName);

  // Read header parameters until "HEADER_END" is encountered
  while (true) {

    strcpy(string, "ERROR");

    // Read ant int containing the length of next header string in chars
    file.read((char*) &headerStringLength, sizeof(int));
    if (!file) {
      std::cerr << "Error reading header string size!" << std::endl;
      exit(0);
    }

    // Skip wrong strings
    if (!(headerStringLength > 1 && headerStringLength < 80)) {
      continue;
    }

    // Read the next header string
    file.read((char*) string, headerStringLength);
    if (!file) {
      std::cerr << "Could not read header string!" << std::endl;
      exit(0);
    }
    string[headerStringLength] = '\0';

    // Exit at end of header
    if (strcmp(string, "HEADER_END") == 0) {
      break;
    }

    // Read parameters
    if (strcmp(string, "tsamp") == 0) {
      file.read((char*) &samplingTime, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'tsamp' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      file.read((char*) &startTime, sizeof(double));
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
      file.read((char*) &numSamples, sizeof(long long));
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

      // In this case, we don't know how long the source name is, so we have to read the length, as we do above
      file.read((char*) &headerStringLength, sizeof(int));
      if (!file) {
        std::cerr << "Error reading header string size!" << std::endl;
        exit(0);
      }

      // Skip wrong strings
      if (!(headerStringLength > 1 && headerStringLength < 80)) {
        continue;
      }

      // Read the source name
      file.read((char*) string, headerStringLength);
      if (!file) {
        std::cerr << "Could not read header string!" << std::endl;
        exit(0);
      }
      string[headerStringLength] = '\0';

    } else if (strcmp(string, "src_raj") == 0) {
      file.read((char*) &RA, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'src_raj' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "src_dej") == 0) {
      file.read((char*) &Dec, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'src_dej' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "az_start") == 0) {
      file.read((char*) &azimuthStart, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'az_start' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "za_start") == 0) {
      file.read((char*) &zenithAngleStart, sizeof(double));
      if (!file) {
        std::cerr << "Did not read header parameter 'za_start' properly!" << std::endl;
        exit(0);
      }
    }

  }

  // Set the header length to the current position in the file, since we just encountered "HEADER_END"
  headerLength = file.tellg();

  // Seek to beginning of file
  file.seekg(0, file.beg);

  // Malloc enough memory to store the header
  headerArray = (char*) malloc(headerLength);

  // Read the entire header into the malloc'd headerArray
  file.read(headerArray, headerLength);

  // Set headerPointer to the beginning of headerArray
  headerPointer = headerArray;

  // If headerPointer - headerArray is less than headerLength, we haven't parsed the entire header
  // headerPointer increases every time a parameter is found, so once all parameters are dealt with, headerPointer - headerArray = headerLength
  while ((headerPointer - headerArray) < headerLength) {

    memcpy(headerParameter, headerPointer, 11);
    headerParameter[11] = '\0';
    if (newSourceName[0] != '\0' && strcmp(headerParameter, "source_name") == 0) {
      headerPointer += 11;
      intHeaderParameterLength = *((int*) (headerPointer));
      headerPointer += sizeof(int);
      memcpy(headerParameter, headerPointer, intHeaderParameterLength);
      headerParameter[intHeaderParameterLength] = '\0';
      std::cout << "Old source name = " << headerParameter << std::endl;
      if (intHeaderParameterLength > newSourceNameLength) {
        // The old source name is longer than the new one, pad the new source name
        memcpy(headerParameter, newSourceName, newSourceNameLength);
        for (int i = newSourceNameLength; i < intHeaderParameterLength; i++) {
          headerParameter[i] = ' ';
        }
      } else {
        memcpy(headerParameter, newSourceName, intHeaderParameterLength);
      }
      headerParameter[intHeaderParameterLength] = '\0';
      std::cout << "New source name = " << headerParameter << std::endl;
      memcpy(headerPointer, headerParameter, intHeaderParameterLength);
    }

    memcpy(headerParameter, headerPointer, 7);
    headerParameter[7] = '\0';
    if (newRA < 900000000 && strcmp(headerParameter, "src_raj") == 0) {
    	headerPointer += 7;
    	doubleHeaderParameterLength = *((double*) (headerPointer));
    	std::cout << "Old RA = " << doubleHeaderParameterLength << std::endl;
    	std::cout << "New RA = " << newRA << std::endl;
    	*((double*) (headerPointer)) = newRA;
    }
    if (newDec < 900000000 && strcmp(headerParameter, "src_dej") == 0) {
    	headerPointer += 7;
    	doubleHeaderParameterLength = *((double*) (headerPointer));
    	std::cout << "Old Dec = " << doubleHeaderParameterLength << std::endl;
    	std::cout << "New Dec = " << newDec << std::endl;
    	*((double*) (headerPointer)) = newDec;
    }

    memcpy(headerParameter, headerPointer, 6);
    headerParameter[6] = '\0';
    if (newStartTime < 900000000 && strcmp(headerParameter, "tstart") == 0) {
    	headerPointer += 6;
    	doubleHeaderParameterLength = *((double*) (headerPointer));
    	std::cout << "Old start time = " << doubleHeaderParameterLength << std::endl;
    	std::cout << "New start time = " << newStartTime << std::endl;
    	*((double*) (headerPointer)) = newStartTime;
    }

    memcpy(headerParameter, headerPointer, 5);
    headerParameter[5] = '\0';
    if (newBeamNumber >= 0 && strcmp(headerParameter, "ibeam") == 0) {
    	headerPointer += 5;
    	intHeaderParameterLength = *((int*) (headerPointer));
    	std::cout << "Old beam number = " << intHeaderParameterLength << std::endl;
    	std::cout << "New beam number = " << newBeamNumber << std::endl;
    	*((int*) (headerPointer)) = newBeamNumber;
    }

    memcpy(headerParameter, headerPointer, 6);
    headerParameter[6] = '\0';
    if (newNumBeams >= 0 && strcmp(headerParameter, "nbeams") == 0) {
    	headerPointer += 6;
    	intHeaderParameterLength = *((int*) (headerPointer));
    	std::cout << "Old number of beams = " << intHeaderParameterLength << std::endl;
    	std::cout << "New number of beams = " << newNumBeams << std::endl;
    	*((int*) (headerPointer)) = newNumBeams;
    }

    memcpy(headerParameter, headerPointer, 5);
    headerParameter[5] = '\0';
    if (newNumBits && strcmp(headerParameter, "nbits") == 0) {
    	headerPointer += 5;
    	intHeaderParameterLength = *((int*) (headerPointer));
    	std::cout << "Old number of bits = " << intHeaderParameterLength << std::endl;
    	std::cout << "New number of bits = " << newNumBits << std::endl;
    	*((int*) (headerPointer)) = newNumBits;
    }

    memcpy(headerParameter, headerPointer, 5);
    headerParameter[5] = '\0';
    if (newSamplingTime > 0 && strcmp(headerParameter, "tsamp") == 0) {
    	headerPointer += 5;
    	doubleHeaderParameterLength = *((double*) (headerPointer));
    	std::cout << "Old sampling time = " << doubleHeaderParameterLength << std::endl;
    	std::cout << "New sampling time = " << newSamplingTime << std::endl;
    	*((double*) (headerPointer)) = newSamplingTime;
    }

    memcpy(headerParameter, headerPointer, 6);
    headerParameter[6] = '\0';
    if (newNumChans && strcmp(headerParameter, "nchans") == 0) {
    	headerPointer += 6;
    	intHeaderParameterLength = *((int*) (headerPointer));
    	std::cout << "Old number of channels = " << intHeaderParameterLength << std::endl;
    	std::cout << "New number of channels = " << newNumChans << std::endl;
    	*((int*) (headerPointer)) = newNumChans;
    }

    memcpy(headerParameter, headerPointer, 4);
    headerParameter[4] = '\0';
    if (newFCh1 != 0 && strcmp(headerParameter, "fch1") == 0) {
      headerPointer += 4;
      doubleHeaderParameterLength = *((double*) (headerPointer));
      std::cout << "Old frequency of channel 1 = " << doubleHeaderParameterLength << std::endl;
      std::cout << "New frequency of channel 1 = " << newFCh1 << std::endl;
      *((double*) (headerPointer)) = newFCh1;
    }

    memcpy(headerParameter, headerPointer, 4);
    headerParameter[4] = '\0';
    if (newFOff != 0 && strcmp(headerParameter, "foff") == 0) {
      headerPointer += 4;
      doubleHeaderParameterLength = *((double*) (headerPointer));
      std::cout << "Old channel bandwidth = " << doubleHeaderParameterLength << std::endl;
      std::cout << "New channel bandwidth = " << newFOff << std::endl;
      *((double*) (headerPointer)) = newFOff;
    }

    memcpy(headerParameter, headerPointer, 12);
    headerParameter[12] = '\0';
    if (newTelescopeID >= 0 && strcmp(headerParameter, "telescope_id") == 0) {
        headerPointer += 12;
        intHeaderParameterLength = *((int*) (headerPointer));
        std::cout << "Old telescope ID = " << intHeaderParameterLength << std::endl;
        std::cout << "New telescope ID = " << newTelescopeID << std::endl;
        *((int*) (headerPointer)) = newTelescopeID;
    }

    memcpy(headerParameter, headerPointer, 10);
    headerParameter[10] = '\0';
    if (newMachineID >= 0 && strcmp(headerParameter, "machine_id") == 0) {
        headerPointer += 10;
        intHeaderParameterLength = *((int*) (headerPointer));
        std::cout << "Old machine ID = " << intHeaderParameterLength << std::endl;
        std::cout << "New machine ID = " << newMachineID << std::endl;
        *((int*) (headerPointer)) = newMachineID;
    }

    memcpy(headerParameter, headerPointer, 8);
    headerParameter[8] = '\0';
    if (newAzimuthStart >= 0 && strcmp(headerParameter, "az_start") == 0) {
        headerPointer += 8;
        doubleHeaderParameterLength = *((double*) (headerPointer));
        std::cout << "Old starting azimuth = " << doubleHeaderParameterLength << std::endl;
        std::cout << "New starting azimuth = " << newAzimuthStart << std::endl;
        *((double*) (headerPointer)) = newAzimuthStart;
    }

    memcpy(headerParameter, headerPointer, 8);
    headerParameter[8] = '\0';
    if (newZenithAngleStart >= 0 && strcmp(headerParameter, "za_start") == 0) {
        headerPointer += 8;
        doubleHeaderParameterLength = *((double*) (headerPointer));
        std::cout << "Old starting zenith angle = " << doubleHeaderParameterLength << std::endl;
        std::cout << "New starting zenith angle = " << newZenithAngleStart << std::endl;
        *((double*) (headerPointer)) = newZenithAngleStart;
    }

    headerPointer++;

  }

  // Seek to the beginning of the file
  file.seekg(0, file.beg);
  // Overwrite the old header
  file.write(headerArray, headerLength);
  // Close the file
  file.close();

  free(headerArray);

}
