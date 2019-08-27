#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <getopt.h>
#include <iterator>

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: dmReducer -f inFileName -o outFileName" << std::endl << std::endl;
}

/* -- dmReducer ------------------------------------------------------------------------------------------------------------------
** Removes events from an ASCII event file which have arrival times beyond the length of the filterbank file (due to appending). |
** All such events will appear in the event file for the next filterbank file.                                                   |
------------------------------------------------------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {

  int arg;
  double MJD, time, dm, snr, width;
  std::ifstream infile;
  std::ofstream outfile;

  if (argc != 5) {
    usage();
    exit(0);
  }

  // Read command line parameters
  while ((arg = getopt(argc, argv, "f:o:h")) != -1) {
    switch (arg) {

      case 'f':
        infile.open(argv[optind - 1]);
        if (!infile.is_open()) {
          std::cerr << std::endl << "Error opening file " << argv[optind - 1] << " for reading!" << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'o':
        outfile.open(argv[optind - 1]);
        if (!outfile.is_open()) {
          std::cerr << std::endl << "Error opening file " << argv[optind - 1] << " for writing!" << std::endl;
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

  if (!infile.is_open()) {
    std::cerr << "You must input a .dm file with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  if (!outfile.is_open()) {
    std::cerr << "You must input an output filename with the -o flag!" << std::endl;
    exit(0);
  }

  // Scan the first line of the input file (the start MJD of the observation) into a variable
  infile >> MJD;

  // Put the MJD in the output file
  outfile << MJD << std::endl;

  // Read in each line from the input file
  while (infile >> time >> dm >> snr >> width) {
    // If the arrival time is 60 seconds or less, write it to the output file
    if (time <= 60.0) {
      double tempList[] = {time, dm, snr, width};
      std::copy(tempList, tempList + 4, std::ostream_iterator<double>(outfile, " "));
      outfile << std::endl;

    }
  }

  // Close the input and output files
  infile.close();
  outfile.close();

}
