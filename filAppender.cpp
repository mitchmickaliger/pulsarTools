#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {

  int flag = 0;
  char *header, *samplesToAdd, *firstFileData, *outfileName;
  long long int firstFileSize, headerSize = atol(argv[3]), dataSize = atol(argv[4]);
  std::ifstream firstFile, secondFile;
  std::ofstream outfile;

  // Open first .fil file to read and moev the file pointer to the end
  firstFile.open(argv[1], std::ifstream::binary | std::ifstream::ate);
  if (!firstFile.is_open()) {
    std::cerr << "Could not open file" << argv[1] << " to read!" << std::endl;
    exit(0);
  }

  // Since we opened the first file at the end, report the size oof the file.
  const size_t firstFileLength = firstFile.tellg();

  // Determine how much memory is needed to hold the entire file
  firstFileSize = sizeof(char) * firstFileLength;

  // Allocate memory to hold the first file
  firstFileData = (char*) malloc(firstFileSize);

  // Seek back to the beginning of the file
  firstFile.seekg(0, firstFile.beg);

  // Open second .fil file to read
  secondFile.open(argv[2], std::ifstream::binary);
  if (!secondFile.is_open()) {
    std::cerr << "Could not open file " << argv[2] << " to read!" << std::endl;
    exit(0);
  }

  // Name the output file based on the input file
  outfileName = strcat(argv[1], ".appended");

  // Open output .fil file to write
  outfile.open(outfileName, std::ofstream::binary);
  if (!outfile.is_open()) {
    std::cerr << "Could not open output file " << outfileName << " to write!" << std::endl;
    exit(0);
  }

  // Allocate memory to store the header of the second .fil file
  header = (char*) malloc(sizeof(char) * headerSize);

//NEED TO READ HEADER PARAMETERS, SET `long long int samplesToOverlap = dmDelay * numChans/sampTime`
//int numChans = ?; float sampTime = ?;

//dmDelay = max delay across band: 4 seconds for LBand, ? for PBand. Actually, dmDelay for LBand is lower for later data, as I shrank the band, but that is probably not important.

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
  secondFile.read(header, headerSize);
  if (!secondFile) {
    std::cerr << "Could not read header from file " << argv[2] << "!" << std::endl;
    exit(0);
  }

  
  if (flag == 1) {
    // Read all of the data from the second file
    secondFile.read(samplesToAdd, dataSize);
    if (!secondFile) {
      std::cerr << "Could not read " << (dataSize/numChans) * sampTime << " seconds of data from file " << argv[2] << "!" << std::endl;
      exit(0);
    }
  } else if (flag == 2) {
    // Read required amount of data from the beginning of the second file
    secondFile.read(samplesToAdd, samplesToOverlap);
    if (!secondFile) {
      std::cerr << "Could not read " << (samplesToOverlap/numChans) * sampTime << " seconds of data from file " << argv[2] << "!" << std::endl;
      exit(0);
    }
  } else {
    std::cerr << "Something is wrong! Check size of " << argv[2] << "!" << std::endl;
  }

  // Read the entire first file into memory
  firstFile.read(firstFileData, firstFileSize);
  if (!firstFile) {
    std::cerr << "Could not read data from file " << argv[1] << "!" << std::endl;
    exit(0);
  }

  // Write the entire first file to the output
  outfile.write(firstFileData, firstFileSize);
  if (!outfile) {
    std::cerr << "Could not write data from file " << argv[1] << " to output file " << outfileName << "!" << std::endl;
    exit(0);
  }

  if (flag == 1) {
    // Append all data from the second file
    outfile.write(samplesToAdd, sizeof(char) * dataSize);
    if (!outfile) {
      std::cerr << "Could not write " << (dataSize/numChans) * sampTime << " seconds of data from file " << argv[2] << " to output file" << outfileName << "!" << std::endl;
      exit(0);
    }
  } else if (flag == 2) {
    // Append required data from the second file
    outfile.write(samplesToAdd, sizeof(char) * samplesToOverlap);
    if (!outfile) {
      std::cerr << "Could not write " << (samplesToOverlap/numChans) * sampTime << " seconds of data from file " << argv[2] << " to output file " << outfileName << "!" << std::endl;
      exit(0);
    }
  }

  firstFile.close();
  secondFile.close();
  outfile.close();

}
