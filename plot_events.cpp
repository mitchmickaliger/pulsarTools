#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "cpgplot.h"
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

void usage() {
  std::cout << std::endl << "Usage: plot_events (-options) -f dat_file" << std::endl;
  std::cout << std::endl << "     -f: Input .dat file" << std::endl;
  std::cout << "     -d: Minimum DM to plot" << std::endl;
  std::cout << "     -D: Maximum DM to plot" << std::endl;
  std::cout << "     -s: Minimum S/N to plot" << std::endl;
  std::cout << "     -S: Maximum S/N to plot" << std::endl;
  std::cout << "     -t: Minimum time to plot" << std::endl;
  std::cout << "     -T: Maximum time to plot" << std::endl;
  std::cout << "     -w: Minimum width to plot" << std::endl;
  std::cout << "     -W: Maximum width to plot" << std::endl;
  std::cout << "     -g: Output plot type (default = /xs)" << std::endl << std::endl;
}

int main(int argc, char *argv[]) {

  int arg;
  float timeMin = 0.0/0.0, timeMax = 0.0/0.0, dmMin = 0.0/0.0, dmMax = 0.0/0.0, snrMin = 0.0/0.0, snrMax = 0.0/0.0, widthMin = 0.0/0.0, widthMax = 0.0/0.0, logWidth;
  float timeMinFile = 0.0/0.0, timeMaxFile = 0.0/0.0, dmMinFile = 0.0/0.0, dmMaxFile = 0.0/0.0, snrMinFile = 0.0/0.0, snrMaxFile = 0.0/0.0, widthMinFile = 0.0/0.0, widthMaxFile = 0.0/0.0;
  float line[4];
  char plotType[128] = "/xs";
  std::ifstream file;

  if (argc < 3) {
    usage();
    exit(0);
  }

  while ((arg = getopt(argc, argv, "d:D:f:g:hs:S:t:T:w:W:")) != -1) {
    switch (arg) {

      case 'f':
        file.open(argv[optind - 1], std::ifstream::binary | std::ifstream::ate);
        if (!file.is_open()) {
          std::cout << "Error opening file " << argv[optind - 1] << std::endl;
          usage();
          exit(0);
        }
        break;

      case 'g':
        strcpy(plotType, optarg);
        break;

      case 'd':
        dmMin = atof(optarg);
        break;

      case 'D':
        dmMax = atof(optarg);
        break;

      case 's':
        snrMin = atof(optarg);
        break;

      case 'S':
        snrMax = atof(optarg);
        break;

      case 't':
        timeMin = atof(optarg);
        break;

      case 'T':
        timeMax = atof(optarg);
        break;

      case 'w':
        widthMin = atof(optarg);
        break;

      case 'W':
        widthMax = atof(optarg);
        break;

      case 'h':
        usage();
        exit(0);

      default:
        return 0;
        break;

    }
  }

  if (!file.is_open()) {
    std::cout << "You must input a .dat file with the -f flag!" << std::endl;
    usage();
    exit(0);
  }

  // The file was opened at the end, so determine how many detections there are
  const size_t numberOfDetections = file.tellg()/sizeof(float)/4.0;
  // Seek back to the beginning of the file
  file.seekg(0, std::ifstream::beg);

  // If there are no detections in the file, close it and exit; this shouldn't happen, beacuse if there are no detections, a file should not be written
  if (numberOfDetections < 1) {
    file.close();
    std::cout << "No detections in file (how did this happen?!), exiting..." << std::endl;
    return 0;
  }

  std::vector<float> DMs(0, numberOfDetections), times(0, numberOfDetections), SNRs(0, numberOfDetections), widths(0, numberOfDetections);

  // Read data from file into vectors
  while (file.read(reinterpret_cast<char*>(&line), sizeof(float) * 4)) {
    DMs.push_back(line[0]);
    times.push_back(line[1]);
    SNRs.push_back(line[2]);
    widths.push_back(line[3]);
  }

  // Close the file now that we've read its contents
  file.close();

  // Find extrema of time, DM, and SNR from file
  snrMinFile = *std::min_element(SNRs.begin(), SNRs.end());
  snrMaxFile = *std::max_element(SNRs.begin(), SNRs.end());
  timeMinFile = *std::min_element(times.begin(), times.end());
  timeMaxFile = *std::max_element(times.begin(), times.end());
  dmMinFile = *std::min_element(DMs.begin(), DMs.end());
  dmMaxFile = *std::max_element(DMs.begin(), DMs.end());
  widthMinFile = *std::min_element(widths.begin(), widths.end());
  widthMaxFile = *std::max_element(widths.begin(), widths.end());

  // Check for NaNs; a variable set to NaN will never equal itself
  // If a variable is NaN, it was not given as user input, and should be set given the data in the file
  // If a variable is not NaN, it was set and should not be changed
  if (timeMin != timeMin) {
    timeMin = timeMinFile;
  }
  if (timeMax != timeMax) {
    timeMax = timeMaxFile;
  }
  if (dmMin != dmMin) {
    dmMin = dmMinFile;
  }
  if (dmMax != dmMax) {
    dmMax = dmMaxFile;
  }
  if (snrMin != snrMin) {
    snrMin = snrMinFile;
  }
  if (snrMax != snrMax) {
    snrMax = snrMaxFile;
  }
  if (widthMin != widthMin) {
    widthMin = widthMinFile;
  }
  if (widthMax != widthMax) {
    widthMax = widthMaxFile;
  }

  snrMin = floor(snrMin);
  snrMax = ceil(snrMax);

  std::cout << numberOfDetections << " events found:" << std::endl;
  std::cout << "Time: " << timeMin << " " << timeMax << ", DM: " << dmMin << " " << dmMax <<  ", SNR: " << snrMin << " " << snrMax << ", Width: " << widthMin << " " << widthMax << std::endl;

  // Don't bother plotting if there are no events over 10 sigma
  if (snrMax <= 10) {
    return 0;
  }

  // Open plot
  cpgopen(plotType);

  // DM vs time
  cpgsvp(0.1, 0.8, 0.1, 0.8);
  cpgswin(timeMin, timeMax, dmMin, dmMax);
  cpgbox("BCTSN", 0., 0, "BCTSN", 0., 0);
  cpglab("Time (s)", "DM (pc cm\\u-3\\d)", " ");

  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i > 10 && *i <= 20) {
      cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 22);
    } else if (*i > 20 && *i <= 50) {
      cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 24);
    } else if (*i > 50 && *i <= 100) {
      cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 25);
    } else if (*i > 100) {
      cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 26);
    }
  }

  cpgsci(1);

  // SNR vs DM
  cpgsvp(0.81, 0.9, 0.1, 0.8);
  if (snrMax > 10) {
    cpgswin(10, 1.05 * snrMax, dmMin, dmMax);
    cpgbox("BCTSN", 0., 1, "BCTS", 0., 0);
  } else {
    cpgswin(10, 11, dmMin, dmMax);
    cpgbox("BCTSN", 0.5, 1, "BCTS", 0., 0);
  }
  cpglab("SNR", "", " ");

  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i > 10) {
      cpgpt1(*i, DMs[std::distance(SNRs.begin(), i)], 1);
    }
  }

  // Time vs Width
  cpgsvp(0.1, 0.8, 0.81, 0.95);
  cpgswin(timeMin, timeMax, log10(widthMin)/log10(2.0), log10(widthMax)/log10(2.0));
  cpgbox("BCTS", 0., 0, "BCTSN", 0., 0);
  cpglab("", "Width (log2 samples)", " ");

  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i > 10) {
      logWidth = log10(widths[std::distance(SNRs.begin(), i)])/log10(2.0);
      cpgpt1(times[std::distance(SNRs.begin(), i)], logWidth, 1);
    }
  }

  cpgend();

  return 0;

}
