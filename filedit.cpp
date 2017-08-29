#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<cmath>
#include<iostream>
#include<fstream>
#include<getopt.h>
#include<time.h>

// ADD FUNCTIONALITY TO REMOVE PARAMETERS

void usage() {
  std::cout << std::endl << "Modify a .fil file 'in place'" << std::endl;
  std::cout << "==================" << std::endl << std::endl;
  std::cout << "filedit -f filFile -options" << std::endl << std::endl;
  std::cout << "Header options that can be modified:" << std::endl;
  std::cout << "    -b beamNum     : change beam number to beamNum" << std::endl;
  std::cout << "    -B nBeams      : change number of beams to nBeams" << std::endl;
  std::cout << "    -c nChans      : change number of channels to nChans" << std::endl;
  std::cout << "    -d DEC         : change dec to DEC in form ddmmss.xxx" << std::endl;
  std::cout << "    -f fch1        : change frequency of first channel to fch1" << std::endl;
  std::cout << "    -F filFile     : .fil file to edit" << std::endl;
  std::cout << "    -i nBits       : change number of bits to nBits" << std::endl;
  std::cout << "    -m machineID   : change machine ID to machineID" << std::endl;
  std::cout << "    -n srcName     : change source name to srcName" << std::endl;
  std::cout << "    -o fOff        : change channel width to fOff" << std::endl;
  std::cout << "    -p tSamp       : change sampling time to tSamp" << std::endl;
  std::cout << "    -r RA          : change ra to RA in form hhmmss.xxx" << std::endl;
  std::cout << "    -t telID       : change telescope ID to telID" << std::endl;
  std::cout << "    -T newMJD      : change start MJD to newMJD" << std::endl << std::endl;
}

int main (int argc, char* argv[]) {

  int arg, headerStringLength, headerLength, dataType, telescopeID, machineID, numIFs = 0, numBits = 0, numChans = 0;
  int newTelescopeID = -1, newMachineID = -1, newBeamNumber = -1, newNumBeams = -1, newNumBits = 0, newNumChans = 0;
  double RA, Dec, samplingTime, fch1, fOff, obsStart;
  double newRA = 900000001, newDec = 900000001, newStartTime = 900000001, newSamplingTime = -1, newFCh1 = 0, newFOff = 0;
  long long numSamples = 0;
  char string[80], sourceName[1024];
  std::fstream file;

  sourceName[0] = '\0';

  while ((arg = getopt(argc, argv, "b:B:c:d:f:F:hi:m:n:o:p:r:t:T:")) != -1) {
    switch (arg) {

      case 'b':
        newBeamNumber = atof(optarg);
        break;

      case 'B':
        newNumBeams = atof(optarg);
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
          std::cout << "Failed to open file " << argv[optind - 1] << " for reading and writing!" << std::endl;
          exit(0);
        }
        break;

      case 'i':
        newNumBits = atoi(optarg);
        break;

      case 'm':
        newMachineID = atof(optarg);
        break;

      case 'n':
        strcpy(sourceName, optarg);
        break;

      case 'o':
        newFOff = atof(optarg);

      case 'p':
        newSamplingTime = atof(optarg);
        break;

      case 'r':
        newRA = atof(optarg);
        break;

      case 't':
        newTelescopeID = atof(optarg);
        break;

      case 'T':
        newStartTime = atof(optarg);
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
    std::cout << std::endl << "You must input a .fil file with the -F flag!" << std::endl;
    usage();
    exit(0);
  }

  // Read header parameters until "HEADER_END" is encountered
  while (true) {

    strcpy(string, "ERROR");

    // Read ant int containing the length of next header string in chars
    file.read((char*) &headerStringLength, sizeof(int));
    if (!file) {
      std::cout << "Error reading header string size!" << std::endl;
      exit(0);
    }
std::cout << "nchar = " << headerStringLength << std::endl;
if (headerStringLength > 20 || headerStringLength < 0) {
  std::cout << "headerStringLength too large or too small! " << headerStringLength << std::endl;
  exit(0);
}
    // Skip wrong strings
    if (!(headerStringLength > 1 && headerStringLength < 80)) {
std::cout << "wrong string! headerStringLength = " << headerStringLength << std::endl;
      continue;
    }

    // Read the next header string
    file.read((char*) string, headerStringLength);
    if (!file) {
      std::cout << "Could not read header string!" << std::endl;
      exit(0);
    }
    string[headerStringLength] = '\0';
std::cout << "string = " << string << std::endl;
    // Exit at end of header
    if (strcmp(string, "HEADER_END") == 0) {
      break;
    }

    // Read parameters
    if (strcmp(string, "tsamp") == 0) {
      file.read((char*) &samplingTime, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'tsamp' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "tstart") == 0) {
      file.read((char*) &obsStart, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'tstart' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "fch1") == 0) {
      file.read((char*) &fch1, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'fch1' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "foff") == 0) {
      file.read((char*) &fOff, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'foff' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nchans") == 0) {
      file.read((char*) &numChans, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'nchans' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nifs") == 0) {
      file.read((char*) &numIFs, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'nifs' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nbits") == 0) {
      file.read((char*) &numBits, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'nbits' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "nsamples") == 0) {
      file.read((char*) &numSamples, sizeof(long long));
      if (!file) {
        std::cout << "Did not read header parameter 'nsamples' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "machine_id") == 0) {
      file.read((char*) &machineID, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'machine_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "telescope_id") == 0) {
      file.read((char*) &telescopeID, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'telescope_id' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "data_type") == 0) {
      file.read((char*) &dataType, sizeof(int));
      if (!file) {
        std::cout << "Did not read header parameter 'data_type' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "source_name") == 0) {

      // In this case, we don't know how long the source name is, so we have to read the length, as we do above
      file.read((char*) &headerStringLength, sizeof(int));
      if (!file) {
        std::cout << "Error reading header string size!" << std::endl;
        exit(0);
      }

      // Skip wrong strings
      if (!(headerStringLength > 1 && headerStringLength < 80)) {
        continue;
      }

      // Read the source name
      file.read((char*) string, headerStringLength);
      if (!file) {
        std::cout << "Could not read header string!" << std::endl;
        exit(0);
      }
      string[headerStringLength] = '\0';

    } else if (strcmp(string, "src_raj") == 0) {
      file.read((char*) &RA, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'src_raj' properly!" << std::endl;
        exit(0);
      }
    } else if (strcmp(string, "src_dej") == 0) {
      file.read((char*) &Dec, sizeof(double));
      if (!file) {
        std::cout << "Did not read header parameter 'src_dej' properly!" << std::endl;
        exit(0);
      }
    }

  }

  // Set the header length to the current position in the file, since we just encountered "HEADER_END"
  headerLength = file.tellg();

  int newlen;
  char* headerArray;
  char* ptr;
  char buf[1024];
  int an_int;
  double a_double;
  float a_float;
  int i;

  printf("Fixing header\n");
  newlen = strlen(sourceName);

  // Seek to beginning of file
  file.seekg(0, file.beg);

  // Malloc enough memory to store the header
  headerArray = (char*) malloc(headerLength);

  // Read the entire header into the malloc'd headerArray
  file.read(headerArray, headerLength);

  // Set a variable 'ptr' to the beginning of headerArray
  ptr = headerArray;

  // If ptr - headerArray is less than headerLength, we haven't parsed the entire header
  // ptr increases every time a parameter is found, so once all parameters are dealt with, ptr - headerArray = headerLength
  while ((ptr - headerArray) < headerLength) {

    memcpy(buf, ptr, 11);
    buf[11] = '\0';
    if (sourceName[0] != '\0' && strcmp(buf, "source_name") == 0) {
      ptr += 11;
      an_int = *((int*) (ptr));
      ptr += sizeof(int);
      memcpy(buf, ptr, an_int);
      buf[an_int] = '\0';
      printf("old src name = '%s'\n", buf);
      if (an_int > newlen) {
        // the old name is longer than the new, pad
        memcpy(buf, sourceName, newlen);
        for (i = newlen; i < an_int; i++) {
          buf[i] = ' ';
        }
      } else {
        memcpy(buf, sourceName, an_int);
      }
      buf[an_int] = '\0';
      printf("new src name = '%s'\n", buf);
      memcpy(ptr, buf, an_int);
    }

    memcpy(buf, ptr, 7);
    buf[7] = '\0';
    if (newRA < 900000000 && strcmp(buf, "src_raj") == 0) {
    	ptr += 7;
    	a_double = *((double*) (ptr));
    	printf("old ra = '%lf'\n", a_double);
    	printf("new ra = '%lf'\n", newRA);
    	*((double*) (ptr)) = newRA;
    }
    if (newDec < 900000000 && strcmp(buf, "src_dej") == 0) {
    	ptr += 7;
    	a_double = *((double*) (ptr));
    	printf("old dec = '%lf'\n", a_double);
    	printf("new dec = '%lf'\n", newDec);
    	*((double*) (ptr)) = newDec;
    }

    buf[6] = '\0';
    if (newStartTime < 900000000 && strcmp(buf, "tstart") == 0) {
    	ptr += 6;
    	a_double = *((double*) (ptr));
    	printf("old tstart = '%lf'\n", a_double);
    	printf("new tstart = '%lf'\n", newStartTime);
    	*((double*) (ptr)) = newStartTime;
    }

    memcpy(buf, ptr, 5);
    buf[5] = '\0';
    if (newBeamNumber >= 0 && strcmp(buf, "ibeam") == 0) {
    	ptr += 5;
    	an_int = *((int*) (ptr));
    	printf("old ibeam = '%d'\n", an_int);
    	printf("new ibeam = '%d'\n", newBeamNumber);
    	*((int*) (ptr)) = newBeamNumber;
    }

    memcpy(buf, ptr, 6);
    buf[6] = '\0';
    if (newNumBeams >= 0 && strcmp(buf, "nbeams") == 0) {
    	ptr += 6;
    	an_int = *((int*) (ptr));
    	printf("old nbeams = '%d'\n", an_int);
    	printf("new nbeams = '%d'\n", newNumBeams);
    	*((int*) (ptr)) = newNumBeams;
    }

    memcpy(buf, ptr, 5);
    buf[5] = '\0';
    if (newNumBits && strcmp(buf, "nbits") == 0) {
    	ptr += 5;
    	an_int = *((int*) (ptr));
    	printf("old nbits = '%d'\n", an_int);
    	printf("new nbits = '%d'\n", newNumBits);
    	*((int*) (ptr)) = newNumBits;
    }

    memcpy(buf, ptr, 5);
    buf[5] = '\0';
    if (newSamplingTime > 0 && strcmp(buf, "tsamp") == 0) {
    	ptr += 5;
    	a_double = *((double*) (ptr));
    	printf("old tsamp = '%lf'\n", a_double);
    	printf("new tsamp = '%f'\n", newSamplingTime);
    	*((double*) (ptr)) = newSamplingTime;
    }

    memcpy(buf, ptr, 6);
    buf[6] = '\0';
    if (newNumChans && strcmp(buf, "nchans") == 0) {
    	ptr += 6;
    	an_int = *((int*) (ptr));
    	printf("old nchans= '%d'\n", an_int);
    	printf("new nchans= '%d'\n", newNumChans);
    	*((int*) (ptr)) = newNumChans;
    }

    memcpy(buf, ptr, 4);
    buf[4] = '\0';
    if (newFCh1 != 0 && strcmp(buf, "fch1") == 0) {
      ptr += 4;
      a_double = *((double*) (ptr));
      printf("old fch1= '%lf'\n", a_double);
      printf("new fch1= '%f'\n", newFCh1);
      *((double*) (ptr)) = (double) newFCh1;
    }

    memcpy(buf, ptr, 4);
    buf[4] = '\0';
    if (newFOff != 0 && strcmp(buf, "fOff") == 0) {
      ptr += 4;
      a_double = *((double*) (ptr));
      printf("old fOff= '%lf'\n", a_double);
      printf("new fOff= '%f'\n", newFOff);
      *((double*) (ptr)) = (double) newFOff;
    }

    memcpy(buf, ptr, 12);
    buf[12] = '\0';
    if (newTelescopeID >= 0 && strcmp(buf, "telescope_id") == 0) {
        ptr += 12;
        an_int = *((int*) (ptr));
        printf("old tel ID = '%d'\n", an_int);
        printf("new tel ID = '%d'\n", newTelescopeID);
        *((int*) (ptr)) = newTelescopeID;
    }

    memcpy(buf, ptr, 10);
    buf[10] = '\0';
    if (newMachineID >= 0 && strcmp(buf, "machine_id") == 0) {
        ptr += 10;
        an_int = *((int*) (ptr));
        printf("old machine ID = '%d'\n", an_int);
        printf("new machine ID = '%d'\n", newMachineID);
        *((int*) (ptr)) = newMachineID;
    }

    ptr++;

  }

  // now re-write the header
  file.seekg(0, file.beg);
  file.write(headerArray, headerLength);

  free(headerArray);

}
