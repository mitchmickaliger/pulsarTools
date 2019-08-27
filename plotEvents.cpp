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

// External function to print help if needed
void usage() {
  std::cout << std::endl << "Usage: plotEvents (-options) -f dat_file" << std::endl;
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

/* -- plotEvents -----------------------------------------------------------------------------------------------------------------
** Plots events detected from a single-pulse/FRB search. This code will automatically zoom in around candidates, so to ensure    |
** comparibility between plots, please make use of the min/max time and DM flags.                                                |
**                                                                                                                               |
** This code expects binary output from astro-accelerate, specifically, but can be used on any binary file with the same format. |
------------------------------------------------------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {

  int arg, dataPlotted = 0;
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

  // Create vectors to hold separate parameters for each event
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

  // Find extrema of time, DM, SNR, and width from file
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

  if (timeMin != timeMin) { // The user has not specified a minimum time
    timeMin = timeMinFile; // Set it to the minimum time in the file
  } else { // The user has specified a minimum time
    if (timeMin > timeMaxFile) {
      std::cout << std::endl << std::endl << "No events detected after a time of " << timeMaxFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events after the user's minimum time
    }
  }
  if (timeMax != timeMax) { // The user has not specified a maximum time
    timeMax = timeMaxFile; // Set it to the maximum time in the file
  } else { // The user has specified a maximum time
    if (timeMax < timeMinFile) {
      std::cout << std::endl << std::endl << "No events detected before a time of " << timeMinFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events before the user's maximum time
    }
  }

  if (dmMin != dmMin) { // The user has not specified a minimum DM
    dmMin = dmMinFile; // Set it to the minimum DM in the file
  } else { // The user has specified a minimum DM
    if (dmMin > dmMaxFile) {
      std::cout << std::endl << std::endl << "No events detected above a DM of " << dmMaxFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events above the user's minimum DM
    }
  }
  if (dmMax != dmMax) { // The user has not specified a maximum DM
    dmMax = dmMaxFile; // Set it to the maximum DM in the file
  } else { // The user has specified a maximum DM
    if (dmMax < dmMinFile) {
      std::cout << std::endl << std::endl << "No events detected below a DM of " << dmMinFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events below the user's maximum DM
    }
  }

  if (snrMin != snrMin) { // The user has not specified a minimum S/N
    snrMin = snrMinFile; // Set it to the minimum S/N in the file
  } else { // The user has specified a minimum S/N
    if (snrMin > snrMaxFile) {
      std::cout << std::endl << std::endl << "No events detected above a S/N of " << snrMaxFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events above the user's minimum S/N
    }
  }
  if (snrMax != snrMax) { // The user has not specified a maximum S/N
    snrMax = snrMaxFile; // Set it to the maximum S/N in the file
  } else { // The user has specified a maximum S/N
    if (snrMax < snrMinFile) {
      std::cout << std::endl << std::endl << "No events detected below a S/N of " << snrMinFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events below the user's maximum S/N
    }
  }

  if (snrMax < snrMin) { // Check if the user has input a maximum S/N that is below their input minimum S/N
    std::cout << std::endl << std::endl << "The requested maximum S/N of " << snrMax << " is below the requested minimum S/N of " << snrMin << std::endl << std::endl;
    return 0; // Nothing would be plotted in this case, so just exit now
  }


  if (widthMin != widthMin) { // The user has not specified a minimum width
    widthMin = widthMinFile; // Set it to the minimum width in the file
  } else { // The user has specified a minimum width
    if (widthMin > widthMaxFile) {
      std::cout << std::endl << std::endl << "No events detected above a width of " << widthMaxFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events wider than the user's minimum width
    }
  }
  if (widthMax != widthMax) { // The user has not specified a maximum width
    widthMax = widthMaxFile;  //Set it to the maximum width in the file
  } else { // The user has specified a maximum width
    if (widthMax < widthMinFile) {
      std::cout << std::endl << std::endl << "No events detected below a width of " << widthMinFile << std::endl << std::endl;
      return 0; // Don't bother plotting if there are no events narrower than the user's maximum width
    }
  }

  // Round the minimum S/N down and the maximum S/N up to the nearest integer for plotting purposes
  snrMin = floor(snrMin);
  snrMax = ceil(snrMax);

  std::cout << numberOfDetections << " events found:" << std::endl;
  std::cout << "Time: " << timeMin << " " << timeMax << ", DM: " << dmMin << " " << dmMax <<  ", SNR: " << snrMin << " " << snrMax << ", Width: " << widthMin << " " << widthMax << std::endl;

  // Check if any event fals into the requested time and DM range above the minimum (requested) S/N and below the maximum (requested) S/N
  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i >= snrMin && *i <= snrMax) { // Check if the S/N of this event is within the requested range
      if (times[std::distance(SNRs.begin(), i)] >= timeMin && times[std::distance(SNRs.begin(), i)] <= timeMax && DMs[std::distance(SNRs.begin(), i)] >= dmMin && DMs[std::distance(SNRs.begin(), i)] <= dmMax) { // Check if this event is within the requested time and DM range
        dataPlotted = 1; // Signifiy that we will plot at least one point
      }
    }
  }

  // If no points will be plotted in the DM vs time plot, don't bother plotting anything, just exit here
  if (dataPlotted == 0) {
    std::cout << std::endl << std::endl << "No events detected within the time, DM, S/N, and width parameters provided!" << std::endl << std::endl;
    return 0;
  }

  // Open the requested plot (default /xs)
  cpgopen(plotType);

  // Set the color index for the plots
  cpgsci(1);

  // Create a DM vs time plot
  cpgsvp(0.1, 0.8, 0.1, 0.8);
  cpgswin(timeMin, timeMax, dmMin, dmMax);
  cpgbox("BCTSN", 0., 0, "BCTSN", 0., 0);
  cpglab("Time (s)", "DM (pc cm\\u-3\\d)", " ");

  // Plot each event in the requested time and DM range above the minimum (requested) S/N and below the maximum (requested) S/N, using larger points for higher S/N events
  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i >= snrMin && *i <= snrMax) { // Check if the S/N of this event is within the requested range
      if (times[std::distance(SNRs.begin(), i)] >= timeMin && times[std::distance(SNRs.begin(), i)] <= timeMax && DMs[std::distance(SNRs.begin(), i)] >= dmMin && DMs[std::distance(SNRs.begin(), i)] <= dmMax) { // Check if this event is within the requested time and DM range
        if (*i < 10) {
          cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 20);
        } else if (*i > 10 && *i <= 20) {
          cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 22);
        } else if (*i > 20 && *i <= 50) {
          cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 24);
        } else if (*i > 50 && *i <= 100) {
          cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 26);
        } else if (*i > 100) {
          cpgpt1(times[std::distance(SNRs.begin(), i)], DMs[std::distance(SNRs.begin(), i)], 28);
        }
      }
    }
  }

  // Create a S/N vs DM plot
  cpgsvp(0.81, 0.9, 0.1, 0.8);
  cpgswin(snrMin - 1, 1.05 * snrMax, dmMin, dmMax);
  cpgbox("BCTSN", 0., 0, "BCTS", 0., 0);
  cpglab("SNR", "", " ");

  // Plot each event in the requested time and DM range above the minimum (requested) S/N and below the maximum (requested) S/N
  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i >= snrMin && *i <= snrMax) { // Check if the S/N of this event is within the requested range
      if (times[std::distance(SNRs.begin(), i)] >= timeMin && times[std::distance(SNRs.begin(), i)] <= timeMax && DMs[std::distance(SNRs.begin(), i)] >= dmMin && DMs[std::distance(SNRs.begin(), i)] <= dmMax) { // Check if this event is within the requested time and DM range
        cpgpt1(*i, DMs[std::distance(SNRs.begin(), i)], 1);
      }
    }
  }

  // Create a time vs width plot
  cpgsvp(0.1, 0.8, 0.81, 0.95);
  cpgswin(timeMin, timeMax, log10(widthMin)/log10(2.0), log10(widthMax)/log10(2.0));
  cpgbox("BCTS", 0., 0, "BCTSN", 0., 0);
  cpglab("", "Width (log2 samples)", " ");

  // Plot each event above the minimum (requested) S/N and below the maximum (requested) S/N
  for (std::vector<float>::iterator i = SNRs.begin(); i != SNRs.end(); ++i) {
    if (*i >= snrMin && *i <= snrMax) { // Check if the S/N of this event is within the requested range
      if (times[std::distance(SNRs.begin(), i)] >= timeMin && times[std::distance(SNRs.begin(), i)] <= timeMax && DMs[std::distance(SNRs.begin(), i)] >= dmMin && DMs[std::distance(SNRs.begin(), i)] <= dmMax) { // Check if this event is within the requested time and DM range
        logWidth = log10(widths[std::distance(SNRs.begin(), i)])/log10(2.0);
        cpgpt1(times[std::distance(SNRs.begin(), i)], logWidth, 1);
      }
    }
  }

  // Close plot file and release graphics device
  cpgend();

  return 0;

}
