#ifndef UTILITIES_H
#define UTILITIES_H

#include <errno.h>
#include <arpa/inet.h> 
#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 

#define ERROR -1
#define PORT 9007
#define GET 1
#define POST 2
#define DELETE 3
#define PUT 4
#define EQUALS 0
#define MB 1000000
#define RESPONSE_BUFFER_SIZE 11*MB


char *phrases[600];

typedef struct http_request{
	int method;
	char uri[256];
	char protocol[20];
} http_request;

typedef struct http_response{
	int status_code;
	int method;
	int content_length;
	char *body;
	int content_type;
} http_response;

int create_socket();

void exit_on_error(const char* error);

void bind_socket(int socket_fd, int port);

void disable_buffers();

void start_listening(int socket_fd, int max_clients);

void connect_to_server(int socked_fd, int port);

void parse_http_response(char *string, http_response *response);

void parse_http_request(char *string, http_request *request);

void print_error_status();

void build_filename(char *folder, char *uri, char *dest);

void send_response(int socket_fd, http_response response);

int copy_file(FILE *file, char *buffer);

void fill_phrases();

#endif