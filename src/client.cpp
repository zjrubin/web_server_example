#include <arpa/inet.h>  // ntohs()
#include <getopt.h>
#include <sys/socket.h>  // socket(), connect(), send(), recv()
#include <unistd.h>      // close()

#include <cstdio>   // printf(), perror()
#include <cstdlib>  // atoi()
#include <cstring>  // strlen()
#include <iostream>
#include <string>
#include <utility>

#include "utility.h"  // make_client_sockaddr()

using std::cerr;
using std::cout;
using std::endl;
using std::move;
using std::string;

void parse_arguments(int argc, char *argv[], char **hostname, int *port,
                     char **message);
int send_message(const char *hostname, int port, const char *message);

int main(int argc, char *argv[]) {
  static const char *const usage_message =
      "Usage: ./client -h hostname -p port_num -m message";

  int response;
  try {
    char *hostname = nullptr;
    int port = 0;
    char *message = nullptr;
    parse_arguments(argc, argv, &hostname, &port, &message);

    printf("Sending message %s to %s:%d\n", message, hostname, port);

    response = send_message(hostname, port, message);
  } catch (const Error &e) {
    cerr << e.msg << endl;
    cerr << usage_message << endl;
    return 1;
  } catch (...) {
    cerr << "Unknown exception caught! Exiting..." << endl;
    cerr << usage_message << endl;
    return 1;
  }

  cout << "Server responds with status code " << response << endl;
  return 0;
}

void parse_arguments(int argc, char *argv[], char **hostname, int *port,
                     char **message) {
  const static struct option longopts[] = {
      {"hostname", required_argument, NULL, 'h'},
      {"port", required_argument, NULL, 'p'},
      {"message", required_argument, NULL, 'm'},
      {0, 0, 0, 0}};

  int c;

  while (true) {
    // getopt_long stores the option index here
    int longindex = 0;
    c = getopt_long(argc, argv, ":h:p:m:", longopts, &longindex);

    // End of options
    if (c == -1) {
      if (*hostname == nullptr) {
        throw Error("Missing hostname");
      }

      if (*port == 0) {
        throw Error("Missing port");
      }

      if (*message == nullptr) {
        throw Error("Missing message");
      }

      break;
    }

    switch (c) {
      case 'h':
        *hostname = optarg;
        break;

      case 'p':
        *port = atoi(optarg);
        break;

      case 'm':
        *message = optarg;
        break;

      case '?':
        // getopt_long already printed an error message
        throw Error(string{"Got unexpected option for: "} + (char)optopt);
        break;

      case ':':
        throw Error(string{"Missing option argument for: "} + (char)optopt);
        break;

      default:
        throw Error("Hit default!");
        break;
    }
  }

  if (optind < argc) {
    string error_message = "Got non-option ARGV-elements:";

    while (optind < argc) {
      error_message += " ";
      error_message += argv[optind++];
    }

    throw Error{move(error_message)};
  }
}

/*
 * Sends a string message to the server and waits for an integer response.
 *
 * Parameters:
 *		hostname: 	Remote hostname of the server.
 *		port: 		Remote port of the server.
 * 		message: 	The message to send.
 * Returns:
 *		The server's response code on success, -1 on failure.
 */
int send_message(const char *hostname, int port, const char *message) {
  if (MAX_MESSAGE_SIZE < strlen(message))
    throw Error("Error: Message exceeds maximum length\n");

  // (1) Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // (2) Create a sockaddr_in to specify remote host and port
  struct sockaddr_in addr;
  make_client_sockaddr(&addr, hostname, port);

  // (3) Connect to remote server
  if (connect(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1)
    throw Error("Error connecting stream socket");

  // (4) Send message to remote server
  if (send(sockfd, message, strlen(message) + 1, 0) == -1)
    throw Error("Error sending on stream socket");

  // (5) Wait for integer response
  int response;
  if (recv(sockfd, &response, sizeof(response), MSG_WAITALL) == -1)
    throw Error("Error receiving response from server");

  // Convert from network to host byte order
  response = ntohs(response);

  // (6) Close connection
  close(sockfd);

  return response;
}
