#include <arpa/inet.h>   // htons()
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>      // close()

#include <condition_variable>
#include <cstdio>   // printf(), perror()
#include <cstdlib>  // atoi()
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>

#include "utility.h"  // make_server_sockaddr(), get_port_number()

using std::condition_variable;
using std::mutex;
using std::queue;
using std::scoped_lock;
using std::thread;
using std::unique_lock;

int run_server(int port, int queue_size);
void handle_connection(int thread_id);

const size_t c_num_threads = 4;
thread g_thread_pool[c_num_threads];
condition_variable g_thread_pool_cv;
mutex g_thread_pool_mutex;
queue<int> g_connection_queue;

int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Usage: ./server port_num\n");
    return 1;
  }

  try {
    int port = atoi(argv[1]);
    run_server(port, 10);
  } catch (const Error& e) {
    perror(e.msg);
    return 1;
  } catch (...) {
    perror("Unknown exception caught! Exiting...");
    return 1;
  }

  return 0;
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 *
 * Parameters:
 *		port: 		The port on which to listen for incoming
 *connections. queue_size: 	Size of the listen() queue Returns: -1 on
 *failure, does not return on success.
 */
int run_server(int port, int queue_size) {
  // Create the thread pool
  for (size_t i; i < c_num_threads; ++i) {
    g_thread_pool[i] = thread{handle_connection, static_cast<int>(i)};
    g_thread_pool[i].detach();
  }

  // (1) Create socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    throw Error("Error opening stream socket");
  }

  // (2) Set the "reuse port" socket option
  int yesval = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) ==
      -1) {
    throw Error("Error setting socket options");
  }

  // (3) Create a sockaddr_in struct for the proper port and bind() to it.
  struct sockaddr_in addr;
  make_server_sockaddr(&addr, port);

  if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
    throw Error("Error binding stream socket");
  }

  // (3b) Detect which port was chosen
  port = get_port_number(sockfd);
  printf("Server listening on port %d...\n", port);

  // (4) Begin listening for incoming connections.
  listen(sockfd, queue_size);

  // (5) Serve incoming connections one by one forever.
  while (true) {
    int connectionfd = accept(sockfd, 0, 0);
    if (connectionfd == -1) {
      throw Error("Error accepting connection");
    }

    scoped_lock lock{g_thread_pool_mutex};
    g_connection_queue.push(connectionfd);
    g_thread_pool_cv.notify_one();
  }
}

void handle_connection(int thread_id) {
  while (true) {
    int connectionfd;
    {
      unique_lock lock{g_thread_pool_mutex};
      while (g_connection_queue.empty()) {
        g_thread_pool_cv.wait(lock);
      }

      connectionfd = g_connection_queue.front();
      g_connection_queue.pop();
    }

    printf("Thread %d: New connection %d\n", thread_id, connectionfd);

    // (1) Receive message from client.
    char msg[MAX_MESSAGE_SIZE];
    memset(msg, 0, sizeof(msg));

    for (int i = 0; i < MAX_MESSAGE_SIZE; i++) {
      // Receive exactly one byte
      int rval = recv(connectionfd, msg + i, 1, MSG_WAITALL);
      if (rval == -1) {
        throw Error("Error reading stream message");
      }

      // Stop if we received a null character
      if (msg[i] == '\0') {
        break;
      }
    }

    // (2) Print out the message
    printf("Client %d says '%s'\n", connectionfd, msg);

    // (3) Send response code to client
    uint16_t response = htons(42);
    if (send(connectionfd, &response, sizeof(response), 0) == -1) {
      throw Error("Error sending response to client");
    }

    // (4) Close connection
    close(connectionfd);
  }
}
