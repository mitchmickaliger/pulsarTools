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
  std::cout << std::endl << "receiver -p <port> -b <blockSize> -f <file>" << std::endl << std::endl;
  return;
}

/* -- receiver ----------------------------------------------------------------------|
** Listens to the specified port and writes the incoming data to the specified file. |
----------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {

  int socketFileDescriptor, newSocketFileDescriptor, bytesRead, arg, port = 56000, blockSize = 8192, optval = 1;
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
          std::cerr << "Failed to open fifo " << optarg << " to read!" << std::endl;
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

  // Allow for reuse of this socket, which needs to be done, as we regularly close and reopen the socket for each new observation
  if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) != 0) {
    std::cerr << "Error setting up socket for reuse!" << std::endl;
  }

  // Turn off socket linger, and set the linger timeout to 0. A close() or shutdown() command will wait until all messages have been sent, or until the linger timeout has been reached (hence setting the timeout to zero). The exit() command will close the socket, but the socket will always linger, unless this is specifically turned off.
  linger socketLinger;
  socketLinger.l_onoff = 0; // Explicitly turn off socket linger
  socketLinger.l_linger = 0; // Set the linger timeout to 0
  if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_LINGER, (const void *) &socketLinger, sizeof(socketLinger)) != 0) {
    std::cerr << "Error turning off socket linger!" << std::endl;
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

  std::cout << "receiver at port " << port << " opened" << std::endl;

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
