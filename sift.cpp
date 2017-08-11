#include<cstdio>
#include<cstdlib>
#include<cmath>
#include<algorithm>
#include<vector>
#include<iostream>
#include<fstream>
#include<iterator>
#include<string>
#include<getopt.h>

// A comparator to sort a vector by its first column, from highest value to lowest value
bool vectorSort(std::vector<double> i, std::vector<double> j) {
  return (i[0] > j[0]);
}

void usage() {
  std::cout << std::endl << "Usage: sift -f CCLfile (-options) " << std::endl << std::endl;
  std::cout << "     -f: Input CCL file" << std::endl << std::endl;
  std::cout << "     -d: Match factor for DM comparison (default = 0.1)" << std::endl;
  std::cout << "     -H: Maximum harmonic ratio to search (default = 8)" << std::endl;
  std::cout << "     -p: Match factor for period comparison (default = 0.001)" << std::endl << std::endl;
  std::cout << "If -H is given as 4, say, the code will search for periods that have harmonic ratios of 2, 3, and 4 (as well as 1/3, 1/2, and 2/3)." << std::endl;
  std::cout << "Note the search for harmonic ratios 1/3, 1/2, and 2/3 are (currently) always included." << std::endl << std::endl;
  std::cout << "The match factors work as follows:" << std::endl << std::endl;
  std::cout << "  When periods are compared, they may have a ratio of 3.999. In this case, one is most likely the fourth harmonic of the other, " << std::endl;
  std::cout << "  but a straight comparison will not identify them as such. The period match factor is multiplied by the harmonic ratio being " << std::endl;
  std::cout << "  compared against to give an reasonable range for determining if periods are harmonically related." << std::endl << "" << std::endl;
  std::cout << "  When comparing DMs between two candidates with harmonically related periods, it is likely that there is a range of DMs over which " << std::endl;
  std::cout << "  the candidates are related. The DM match factor is multiplied by the candidate DM to determine a valid range in which another " << std::endl;
  std::cout << "  candidate could be considered a harmonic." << std::endl << std::endl;;
}

/* -- sift --
** This code will read in a CCL (crude candidate list; a list of all candidates returned from a search),
** attempt to determine which candidates have harmonically related periods and related DMs, and return
** an SCL (sifted candidate list; a list of only the single best candidate for every real signal).
**
** This version searches integer harmonics (and a few fractional harmonics). If there are too many related
** candidates left in the CCL after tweaking the period match factor and DM match factor, try using 'strongSift'.
*/
int main(int argc, char *argv[]) {

  int arg, cand, totalCands = 0, maxHarm = 8, arraySize;
  double candPeak, candPeriod, candDM, testPeak, testPeriod, testDM;
  double periodMatchFactor = 0.001, dmMatchFactor = 0.1, periodRatio;
  double peakSNR, period, dm;
  std::string periodString, dummyString1, dummyString2;
  std::ifstream infile;
  std::ofstream outfile;

  while ((arg = getopt(argc, argv, "d:f:H:hp:")) != -1) {
    switch (arg) {

      // Reads in a text file which lists parameters for all candidates
      case 'f':
        infile.open(argv[optind - 1]);
        if (!infile.is_open()) {
          std::cout << "Error opening file " << argv[optind - 1] << " to read!" << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'd':
        dmMatchFactor = atof(optarg);
        if (dmMatchFactor < 0) {
          std::cout << "dmMatchFactor must be positive!" << std::endl << "Defaulting to 0.1!" << std::endl;
        }
        break;

      case 'H':
        maxHarm = atoi(optarg);
        if (maxHarm < 1) {
          std::cout << "Number of trial harmonic ratios must be at least 1!" << std::endl << "Defaulting to 8 harmonics!" << std::endl;
          maxHarm = 8;
        }
        break;

      case 'p':
        periodMatchFactor = atof(optarg);
        if (periodMatchFactor < 0) {
          std::cout << "periodMatchFactor must be positive!" << std::endl << "Defaulting to 0.001!" << std::endl;
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

  // Check if the file has failed to open (or the user has failed to specify the -f flag)
  if (!infile.is_open()) {
    std::cout << "You must input a CCL file with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  // Generate a UniqueNumber class to pass to std::generate. This will be used to fill a vector with consecutive integers.
  struct c_unique {
  
    int current;
    c_unique() {
      current = 0;
    }
  
    int operator()() {
      return ++current;
    }
  
  } UniqueNumber;

  // Create a vector large enough to hold all integer harmonics from 1 to maxHarm, plus 1/3, 1/2, and 2/3
  std::vector<double> harmRatio(maxHarm + 3);

  // Fill the harmRatio vector with integer harmonics from 1 to maxHarm
  std::generate(harmRatio.begin(), harmRatio.end(), UniqueNumber);

  // Add 1/3, 1/2, and 2/3 ratios to the harmRatio vector
  harmRatio.push_back(1.0/3.0);
  harmRatio.push_back(1.0/2.0);
  harmRatio.push_back(2.0/3.0);

  // Open an output file in which to write the sifted candidates. The name 'scl' stands for 'Sifted Candidate List'
  outfile.open("scl.txt");

  // Array to store unsifted periodicity candidates. The name 'ccl' stands for 'Crude Candidate List'
  std::vector<std::vector<double> > ccl;

  // Array to store sifted periodicity candidates. The name 'scl' is as above ('Sifted Candidate List')
  std::vector<std::vector<double> > scl;

  // Scan each line of the file and read the entries into their appropriate variables
  while (infile >> peakSNR >> periodString >> dummyString1 >> dummyString2 >> dm) {
    // The period has an error listed in parentheses '(),' so we need to read it in as a string and strip off the parentheses
    periodString.erase(periodString.end() - 3, periodString.end());
    // Now that the periodString is a number, convert it to a double
    period = std::stod(periodString);
    // Put the S/N, period, and DM into a list
    double tempList[] = {peakSNR, period, dm};
    // Insert the above list into a temporary vector
    std::vector<double> tempVector(tempList, tempList + sizeof(tempList)/sizeof(double));
    // Push the temporary vector into the ccl vector
    ccl.push_back(tempVector);
  }

  // All of the data are now in the ccl vector, close the input file
  infile.close();

  // Sort the list by S/N
  // vectorSort is defined above and sorts from highest value to lowest value
  sort(ccl.begin(), ccl.end(), vectorSort);

  // Loop until all candidates have been dealt with
  while (true) {

    // Test if all candidates have been dealt with; if so, then finish
    if (ccl.size() == 0) {
      break;
    }

    // Start with the highest S/N candidate and check to see if any of the other candidates are related
    candPeak = ccl[0][0];
    candPeriod = ccl[0][1];
    candDM = ccl[0][2];

    // Create a list from the candidate values
    double candList[] = {candPeak, candPeriod, candDM};

    // Put the candidate list into a vector
    std::vector<double> candVector(candList, candList + sizeof(candList)/sizeof(double));

    // Put this candidate's parameters into the sifted candidate list
    scl.push_back(candVector);

    // Go through every remaining signal in the list to check for related signals
    for (cand = 1; cand < ccl.size(); cand++) {

      testPeak = ccl[cand][0];
      testPeriod = ccl[cand][1];
      testDM = ccl[cand][2];

      // Calculate the ratio of the periods
      // Both this ratio and its inverse are tested against all harmonics
      periodRatio = candPeriod/testPeriod;

      // Go through each ratio and test whether this period is some harmonic of the candidate
      for (auto testHarmRatio = harmRatio.begin(); testHarmRatio != harmRatio.end(); ++testHarmRatio) {
        // If the ratio of periods matches a harmonic (within some 'periodmatchFactor' tolerance), keep investigating, otherwise, try the next harmonic
        if (std::abs(periodRatio - *testHarmRatio) < (periodMatchFactor * *testHarmRatio) || std::abs((1/periodRatio) - *testHarmRatio) < (periodMatchFactor * *testHarmRatio)) {
          // If the periods are harmonically related, check if the DMs are close enough
          if (std::abs(candDM - testDM) < (candDM * dmMatchFactor)) {
            // This signal is a harmonic of the candidate; save its parameters with those of the candidate
            double testList[] = {testPeak, testPeriod, testDM};
            scl[totalCands].insert(scl[totalCands].end(), testList, testList + 3);
            // Remove the harmonic from the list
            ccl[cand].clear();
            break;
          }
        }
      }

    }

    // Write the signal and all of its harmonics to one line of the output file
    std::copy(scl[totalCands].begin(), scl[totalCands].end(), std::ostream_iterator<double>(outfile, " "));
    outfile << std::endl;

    // Remove the candidate from the list
    ccl[0].clear();

    // Remake the sorted list, removing the now empty cells
    ccl.erase(remove(ccl.begin(), ccl.end(), std::vector<double>()), ccl.end());

    // Increment the candidate counter
    totalCands++;

  }

  // Now that there are no more candidates to sift, close the output file
  outfile.close();

}
