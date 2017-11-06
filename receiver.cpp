#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>

// External function to print help if needed
void usage(void) {
  std::cout << std::endl << "receiver -p <port> -b <blockSize> <file>" << std::endl << std::endl;
  return;
}

/* -- receiver ----------------------------------------------------------------------|
** Listens to the specified port and writes the incoming data to the specified file. |
----------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {

  int socketFileDescriptor, newSocketFileDescriptor, bytesRead, arg, port = 56000, blockSize = 8192;
  socklen_t clientAddressLength;
  char *buffer;
  struct sockaddr_in serverAddress, clientAddress;
  std::ofstream fifoFileDescriptor;

  // Read command line parameters
  while ((arg = getopt(argc, argv, "b:f:p:h")) != -1) {
    switch (arg) {

      case 'b':
        blockSize = atoi(optarg);
        break;

      case 'f':
        fifoFileDescriptor.open(optarg, std::ofstream::binary);
        if (!fifoFileDescriptor.is_open()) {
          std::cerr << "Failed to open fifo " << optarg << " to write!" << std::endl;
          exit(1);
        }
        break;

      case 'p':
        port = atoi(optarg);
        break;

      case 'h':
        usage();
        exit(0);

      default:
        return 0;
        break;

    }
  }

  // Print usage if user has not specified any options
  if (argc == 1) {
    usage();
    return 0;
  }

  // Allocate buffer
  buffer = (char *) malloc(sizeof(char) * blockSize);

  // Open socket
  socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFileDescriptor < 0) {
    std::cerr << "Error opening socket!" << std::endl;
    exit(1);
  }
  bzero((char *) &serverAddress, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(port);
  if (bind(socketFileDescriptor, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    std::cerr << "Error binding socket!" << std::endl;
    exit(1);
  }

  // Listen to socket
  if (listen(socketFileDescriptor, 5) < 0) {
    std::cerr << "Error listening to socket!" << std::endl;
    exit(1);
  }

  std::cout << "receiver at port " << port << " openened" << std::endl;

  clientAddressLength = sizeof(clientAddress);
  newSocketFileDescriptor = accept(socketFileDescriptor, (struct sockaddr *) &clientAddress, &clientAddressLength);

  bzero(buffer, blockSize);
  if (newSocketFileDescriptor < 0) {
    std::cerr << "Error accepting connection from socket!" << std::endl;
    exit(1);
  }

  while (true) {
    bytesRead = read(newSocketFileDescriptor, buffer, blockSize);
    if (bytesRead < 0) {
      std::cerr << "Error reading from socket!" << std::endl;
      exit(1);
    }
    fifoFileDescriptor.write(buffer, sizeof(char) * bytesRead);
    if (bytesRead == 0) {
      break;
    }
  }
  fifoFileDescriptor.close();
  close(newSocketFileDescriptor);
  std::cout << "receiver at port " << port << " finished" << std::endl;

  close(socketFileDescriptor);

  // Free buffer
  free(buffer);

  std::cout << "Exiting " << port << std::endl;

  return 0;

}
