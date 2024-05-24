#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define REQUEST_SIZE 65536
#define REQUEST_TYPES_COUNT 16
#define RESPONSE_SIZE 65536
#define CONTENT_SIZE RESPONSE_SIZE >> 1

int server_socket;
int client_socket;
int port;

struct sockaddr_in server_address;
struct sockaddr_in client_address;
socklen_t client_address_len;

char request[REQUEST_SIZE];
char *response;
char content[CONTENT_SIZE];

void error(const char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

void response_get_login() {
	FILE *file = fopen("login.html", "rb");

	if(file == NULL)
		error("fopen() error");

	fseek(file, 0, SEEK_END); 
	unsigned size = ftell(file);
	fseek(file, 0, SEEK_SET); 
	if(fread(content, sizeof(char), size, file) != size)
		error("fread() error");
	fclose(file);

	snprintf(response, RESPONSE_SIZE,
		"HTTP/1.1 200 OK\n"
		"Content-Type: text/html\n"
		"Content-Length: %lu\n\n%s", strlen(content), content);
}

void handle_request(char *request) {
	fprintf(stderr, "Request: %s\n", request);

	if(response == NULL)
		response = calloc(1, RESPONSE_SIZE << 1);

	if(memcmp(request, "GET /login", strlen("GET /login")) == 0)
		response_get_login();

	if(send(client_socket, response, strlen(response), 0) == -1)
		error("send() error");

	// fprintf(stderr, "Response: %s\n", response);
}

int main(int argc, char **argv) {
	if(argc == 1)		port = 8080;
	else if(argc == 2)	port = atoi(argv[1]);
	else				exit(EXIT_FAILURE);

	// Socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(server_socket < 0)
		error("socket() error");

	if(port < 2000 || port > 65535) {
		fprintf(stderr, "Invalid port number\n");
		exit(EXIT_FAILURE);
	}

	// Bind
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons((uint16_t)port);

	if(bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
		error("bind() error");

	// Loop to listen to requests
	while(1) {
		// Listen
		if(listen(server_socket, 1) < 0)
			error("listen() error");

		// Client socket
		client_address_len = sizeof(client_address);
		client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);

		if(client_socket < 0)
			error("accept() error");
		
		// Read
		if(read(client_socket, request, REQUEST_SIZE) < 0)
			error("read() error");
		
		handle_request(request);
	}

	return 0;
}
