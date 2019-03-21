#include "../lib/utils.h"

#define MAX_QUEUE_LENGTH 1
#define BUFFER_SIZE 1024


void handle_client(int client_socket_fd){
    // FOO HTTP RESPONSE
    // char request[2048];
    // strcpy(request,"HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: 58\n\n<html><head></head><body><h1>HELLO WORLD</h1</body></html>");
    // strcpy(t,"<html><head></head><body><h1>HELLO WORLD</h1</body></html>");
    // send(client_socket_fd, t, sizeof(t),0);


    // char r[1024];
    // recv(client_socket_fd,r, sizeof(r), 0);
    // printf("client entered with:%s\n",r);

    // http_request req;
    // parse_http_request(r, &req);
    // printf("method:%d\n", req.method);
    // printf("uri:%s\n", req.uri);
    // printf("protocol:%s\n", req.protocol);

    // return;


	// Read the filename
	char buffer[BUFFER_SIZE];
	read( client_socket_fd, buffer, BUFFER_SIZE );
	http_request req;
    parse_http_request(buffer, &req);


    char filename[20];
    int offset=sprintf(filename,"www/");
    sprintf(filename+offset, req.uri+1);

    printf("file to open %s\n",filename);
	// Open the file 
    FILE *file = fopen(filename, "r");

    if(file){
        char response[2048];
        int offs = sprintf(response, "HTTP/1.1 200 OK\n");
        offs += sprintf(response+offs, "\n");
    	// Write the file to the socket
        char block[1024];
        int bytes_read;
        while ((bytes_read = fread(block, 1, sizeof(block), file)))
            offs += sprintf(response+offs,"%s", block);

        send(client_socket_fd, response, offs, 0);
        printf("Response sent:%s \n",response);
        // Close the requested file 
        fclose(file);
    }
    else{
        printf("File requested not found\n");
    }
}

int main() {

    // http_request req;
    // parse_http_request("GET /hola/mundo.res HTTP/1.1 \n", &req);
    // printf("method:%d\n", req.method);
    // printf("uri:%s\n", req.uri);
    // printf("protocol:%s\n", req.protocol);
    // return 0;



    disable_buffers();

	int socket_fd = create_socket();

    bind_socket(socket_fd, PORT);

    start_listening(socket_fd, MAX_QUEUE_LENGTH);

    int served_clients = 0, fails=0;

    struct sockaddr_in client_address;
    while(served_clients <= 3){
        printf("Waiting clients at %d | served: %d received: %d\n",PORT,served_clients, fails);
    	int client_socket_fd = accept(socket_fd, 
    		(struct sockaddr *) &client_address, 
    		(socklen_t*) &client_address);

    	if(client_socket_fd == ERROR){
    		printf("Error accepting client\n");
            print_error_status();
            fails++;
    	}
    	else{
    		handle_client(client_socket_fd);
            close(client_socket_fd);
            served_clients++;
    	}
    }
	
	close(socket_fd);    
    return 0; 
 }
