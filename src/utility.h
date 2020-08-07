#pragma once

#define MAX_MESSAGE_SIZE 256

struct sockaddr_in;

/**
 * Make a server sockaddr given a port.
 * Parameters:
 *		addr: 	The sockaddr to modify (this is a C-style function).
 *		port: 	The port on which to listen for incoming connections.
 * Returns:
 *		0 on success, -1 on failure.
 * Example:
 *		struct sockaddr_in server;
 *		int err = make_server_sockaddr(&server, 8888);
 */
int make_server_sockaddr(struct sockaddr_in *addr, int port);

/**
 * Make a client sockaddr given a remote hostname and port.
 * Parameters:
 *		addr: 		The sockaddr to modify (this is a C-style
 *function). hostname: 	The hostname of the remote host to connect to. port:
 *The port to use to connect to the remote hostname. Returns: 0 on success, -1
 *on failure. Example: struct sockaddr_in client; int err =
 *make_client_sockaddr(&client, "141.88.27.42", 8888);
 */
int make_client_sockaddr(struct sockaddr_in *addr, const char *hostname,
                         int port);

/**
 * Return the port number assigned to a socket.
 *
 * Parameters:
 * 		sockfd:	File descriptor of a socket
 *
 * Returns:
 *		The port number of the socket, or -1 on failure.
 */
int get_port_number(int sockfd);

// a simple class for error exceptions - msg points to a C-string error message
struct Error {
  Error(const char *msg_ = "") : msg{msg_} {}
  const char *const msg;
};
