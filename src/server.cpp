#include <arpa/inet.h> // htons()
#include <cstring>
#include <cstdio>		// printf(), perror()
#include <cstdlib>		// atoi()
#include <sys/socket.h> // socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>		// close()

#include "utility.h" // make_server_sockaddr(), get_port_number()

int run_server(int port, int queue_size);
int handle_connection(int connectionfd);

int main(int argc, const char **argv)
{
	if (argc != 2)
	{
		printf("Usage: ./server port_num\n");
		return 1;
	}

	try
	{
		int port = atoi(argv[1]);
		run_server(port, 10);
	}
	catch (Error &e)
	{
		perror(e.msg);
		return 1;
	}
	catch (...)
	{
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
 *		port: 		The port on which to listen for incoming connections.
 *		queue_size: 	Size of the listen() queue
 * Returns:
 *		-1 on failure, does not return on success.
 */
int run_server(int port, int queue_size)
{
	// (1) Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		throw Error("Error opening stream socket");

	// (2) Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1)
		throw Error("Error setting socket options");

	// (3) Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;
	make_server_sockaddr(&addr, port);

	if (bind(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1)
		throw Error("Error binding stream socket");

	// (3b) Detect which port was chosen
	port = get_port_number(sockfd);
	printf("Server listening on port %d...\n", port);

	// (4) Begin listening for incoming connections.
	listen(sockfd, queue_size);

	// (5) Serve incoming connections one by one forever.
	while (true)
	{
		int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1)
			throw Error("Error accepting connection");

		handle_connection(connectionfd);
	}
}

/**
 * Receives a null-terminated string message from the client, prints it to stdout,
 * then sends the integer 42 back to the client as a success code.
 *
 * Parameters:
 * 		connectionfd: 	File descriptor for a socket connection (e.g. the one
 *				returned by accept())
 * Returns:
 *		0 on success, -1 on failure.
 */
int handle_connection(int connectionfd)
{
	printf("New connection %d\n", connectionfd);

	// (1) Receive message from client.
	char msg[MAX_MESSAGE_SIZE];
	memset(msg, 0, sizeof(msg));

	for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
	{
		// Receive exactly one byte
		int rval = recv(connectionfd, msg + i, 1, MSG_WAITALL);
		if (rval == -1)
			throw Error("Error reading stream message");

		// Stop if we received a null character
		if (msg[i] == '\0')
			break;
	}

	// (2) Print out the message
	printf("Client %d says '%s'\n", connectionfd, msg);

	// (3) Send response code to client
	uint16_t response = htons(42);
	if (send(connectionfd, &response, sizeof(response), 0) == -1)
		throw Error("Error sending response to client");

	// (4) Close connection
	close(connectionfd);

	return 0;
}
