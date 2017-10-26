#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: filAdder filFile1 filFile2 filFile3..." << std::endl << std::endl;
}

/* -- filAdder -------------------------------------
** Adds together as many fil files as you give it. |
------------------------------------------------- */
int main(int argc, char *argv[]) {

  int nchar = sizeof(int), numChans = 0, numBits = 0, numIFs = 0, machineID, telescopeID, dataType, numSamps = 0;
  double obsStart, sampTime, fCh1, fOff;
  char *buffer, string[80];
  std::ifstream infile;
  std::ofstream outfile;

  if (argc < 3) {
    std::cerr << std::endl << "You must at least list two files to add!" << std::endl;
    usage();
    exit(0);
  }

  // Open output .fil file to write
  outfile.open("combined.fil", std::ofstream::binary);
  if (!outfile.is_open()) {
    std::cerr << "Could not open output file combined.fil to write!" << std::endl << std::endl;
    exit(0);
  }

  // Open first .fil file to read
  infile.open(argv[1], std::ifstream::binary | std::ifstream::ate);
  if (!infile.is_open()) {
    std::cerr << "Could not open file " << argv[1] << " to read!" << std::endl << std::endl;
    exit(0);
  }

  std::cout << "Reading " << argv[1] << "..." << std::endl;

  // Since we opened the file at the end, report the size of the file
  const size_t fileSize = infile.tellg();

  // Seek back to the beginning of the file
  infile.seekg(0, infile.beg);

  // Allocate memory to store the entire .fil file
  buffer = (char*) malloc(sizeof(char) * fileSize);

  // Read the entire file into the malloc'd buffer
  infile.read(buffer, fileSize);

  // Close the file
  infile.close();

  // Write the entire first file to the output file
  outfile.write(buffer, fileSize);

  // Delete the malloc'd buffer
  free(buffer);

  // For the remaining files, get the data and append them to the output file
  for (int i = 2; i < argc; i++) {

    // Open next .fil file to read
    infile.open(argv[i], std::ifstream::binary);
    if (!infile.is_open()) {
      std::cerr << "Could not open file " << argv[i] << " to read!" << std::endl << std::endl;
      exit(0);
    }

    std::cout << "Reading " << argv[i] << "..." << std::endl;

    // Read header parameters until "HEADER_END" is encountered
    while (true) {

      // Read string size
      strcpy(string, "ERROR");
      infile.read((char*) &nchar, sizeof(int));
      if (!infile) {
        std::cerr << "Error reading header string size!" << std::endl << std::endl;
        exit(0);
      }

      // Skip wrong strings
      if (!(nchar > 1 && nchar < 80)) {
        continue;
      }

      // Read string
      infile.read((char*) string, nchar);
      if (!infile) {
        std::cerr << "Could not read header string!" << std::endl << std::endl;
        exit(0);
      }
      string[nchar] = '\0';

      // Exit at end of header
      if (strcmp(string, "HEADER_END") == 0) {
        break;
      }

      // Read parameters
      if (strcmp(string, "tsamp") == 0) {
        infile.read((char*) &sampTime, sizeof(double));
        if (!infile) {
          std::cerr << "Did not read header parameter 'tsamp' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "tstart") == 0) {
        infile.read((char*) &obsStart, sizeof(double));
        if (!infile) {
          std::cerr << "Did not read header parameter 'tstart' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "fch1") == 0) {
        infile.read((char*) &fCh1, sizeof(double));
        if (!infile) {
          std::cerr << "Did not read header parameter 'fch1' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "foff") == 0) {
        infile.read((char*) &fOff, sizeof(double));
        if (!infile) {
          std::cerr << "Did not read header parameter 'foff' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "nchans") == 0) {
        infile.read((char*) &numChans, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'nchans' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "nifs") == 0) {
        infile.read((char*) &numIFs, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'nifs' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "nbits") == 0 ) {
        infile.read((char*) &numBits, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'nbits' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "nsamples") == 0) {
        infile.read((char*) &numSamps, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'nsamples' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "machine_id") == 0) {
        infile.read((char*) &machineID, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'machine_id' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "telescope_id") == 0) {
        infile.read((char*) &telescopeID, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'telescope_id' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else if (strcmp(string, "data_type") == 0) {
        infile.read((char*) &dataType, sizeof(int));
        if (!infile) {
          std::cerr << "Did not read header parameter 'data_type' properly!" << std::endl << std::endl;
          exit(0);
        }
      } else {
        std::cerr << "Unknown header parameter " << string << std::endl;
      }

    }

    // Get the size of the header from the current position of the file pointer
    const size_t headerSize = infile.tellg();

    // Seek to the end of the file
    infile.seekg(0, infile.end);

    // Get the size of the data based on the difference of the total file size and the header size
    const size_t bufferSize = infile.tellg() - headerSize;

    // Seek back to the end of the header
    infile.seekg(headerSize, infile.beg);

    // Allocate memory to store data from file
    buffer = (char*) malloc(sizeof(char) * bufferSize);

    // Read all data from the file into malloc'd buffer
    infile.read((char*) buffer, bufferSize);

    // Close file
    infile.close();

    // Write the file's data to the output file
    outfile.write(buffer, bufferSize);

    // Delete the malloc'd buffer
    free(buffer);

  }

  // Close the output file
  outfile.close();

}
