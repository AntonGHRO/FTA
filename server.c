#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define REQUEST_SIZE 65536
#define RESPONSE_SIZE 65536 << 1
#define CONTENT_SIZE 65536

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

    if (file == NULL) {
        error("fopen() error");
    }

    fseek(file, 0, SEEK_END);
    unsigned size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fread(content, sizeof(char), size, file) != size) {
        error("fread() error");
    }
    fclose(file);
    content[size] = '\0'; // Ensure null termination

    snprintf(response, RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %u\r\n\r\n%s",
             size, content);

    response[strlen(response)] = 0;
}

void response_post_login() {
    char u[1024] = {0}, p[1024] = {0};
    char en[1024 << 1] = {0};
    char *rs, *re;
    unsigned i = 0;
    char text[1024];
    uint32_t hash = 2166136261U;

    for (rs = request; *rs != '\"'; rs++);
    rs += 8;

    for (re = rs; *re != ','; re++);
    strncpy(u, rs, re - rs - 1);
    re += 9;

    for (rs = re; *rs != '}'; rs++);
    strncpy(p, re, rs - re - 1);

    strcat(en, u);
    strcat(en, p);

    while (en[i] != 0) {
        hash ^= en[i++];
        hash *= 16777619U;
    }

    fprintf(stderr, "User registered: %s %s %s %u\n", u, p, en, hash);

    FILE *file = fopen("uppair", "a");
    if (file == NULL) {
        error("fopen() error");
    }
    fprintf(file, "%s %s %u\n", u, p, hash);
    fclose(file);

    snprintf(text, 1024, "%u", hash);

    snprintf(response, RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %lu\r\n\r\n%s",
             strlen(text), text);

    response[strlen(response)] = 0;
}

void response_home() {
    FILE *file = fopen("home.html", "rb");

    if (file == NULL) {
        error("fopen() error");
    }

    fseek(file, 0, SEEK_END);
    unsigned size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fread(content, sizeof(char), size, file) != size) {
        error("fread() error");
    }
    fclose(file);
    content[size] = '\0'; // Ensure null termination

    snprintf(response, RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %u\r\n\r\n%s",
             size, content);

    response[strlen(response)] = 0;
}

void response_post_home() {
    fprintf(stderr, "\n\n%s\n\n", request);
}

void handle_request(char *request) {
    fprintf(stderr, "Request: %s\n", request);

    if (response == NULL)
        response = calloc(1, RESPONSE_SIZE);

    if (memcmp(request, "GET /login", strlen("GET /login")) == 0) {
        response_get_login();
    } else if (memcmp(request, "POST /login", strlen("POST /login")) == 0) {
        response_post_login();
    } else if (memcmp(request, "GET /home", strlen("GET /home")) == 0) {
        response_home();
    } else if (memcmp(request, "POST /home", strlen("POST /home")) == 0) {
        response_post_home();
    } else {
        snprintf(response, RESPONSE_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: 13\r\n\r\n404 Not Found");
    }

    if (send(client_socket, response, strlen(response), 0) == -1) {
        error("send() error");
    }
}

int main(int argc, char **argv) {
    if (argc == 1)
        port = 8080;
    else if (argc == 2)
        port = atoi(argv[1]);
    else
        exit(EXIT_FAILURE);

    // Socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
        error("socket() error");

    if (port < 2000 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    // Bind
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons((uint16_t)port);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        error("bind() error");

    // Loop to listen to requests
    while (1) {
        // Listen
        if (listen(server_socket, 1) < 0)
            error("listen() error");

        // Client socket
        client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);

        if (client_socket < 0)
            error("accept() error");

        // Read
        if (read(client_socket, request, REQUEST_SIZE) < 0)
            error("read() error");

        handle_request(request);
        close(client_socket); // Close the client socket after handling the request
    }

    return 0;
}
