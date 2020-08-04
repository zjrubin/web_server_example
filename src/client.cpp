#include "utility.h"	// make_client_sockaddr()
#include <arpa/inet.h>	// ntohs()
#include <cstdio>		// printf(), perror()
#include <cstdlib>		// atoi()
#include <cstring>		// strlen()
#include <sys/socket.h> // socket(), connect(), send(), recv()
#include <unistd.h>		// close()

int send_message(const char *hostname, int port, const char *message);

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("Usage: ./client hostname port_num message\n");
		return 1;
	}

	const char *hostname = argv[1];
	int port = atoi(argv[2]);
	const char *message = argv[3];
	printf("Sending message %s to %s:%d\n", message, hostname, port);

	int response;
	try
	{
		response = send_message(hostname, port, message);
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

	printf("Server responds with status code %d\n", response);
	return 0;
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
int send_message(const char *hostname, int port, const char *message)
{
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
