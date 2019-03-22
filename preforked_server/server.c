#include "../lib/utils.h"

#define MAX_QUEUE_LENGTH 1
#define MB 1000000
#define KB 1000
#define REQUEST_BUFFER_SIZE 1*KB
#define FILE_BUFFER_SIZE 10*MB

void foo(){return;}

void handle_client(int client_socket_fd){

    // Read the filename
    char request_buffer[REQUEST_BUFFER_SIZE];
    return;
    // char request_buffer[1024];

    printf("reading request...");
    read( client_socket_fd, request_buffer, sizeof(request_buffer));
    printf("done\n");
	http_request req;
    
    printf("parsing req...");
    parse_http_request(request_buffer, &req);
    printf("done\n");

    char filename[20];

    printf("building filename...");
    build_filename("www",req.uri, filename);
    printf("done\n");
    
    printf("file to open %s\n",filename);
    printf("KKK\n");
    
    // Open the file 
    FILE *file = fopen(filename, "r");

    http_response response;

    if(file){
        char file_buffer[FILE_BUFFER_SIZE];
        
        printf("copying file...");
        int file_size = copy_file(file, file_buffer);
        printf("done\n");

        response.content_length = file_size;
        response.body = file_buffer;
        response.status_code = 200;
        
        // Close the requested file 
        fclose(file);
    }
    else{
        printf("File requested not found\n");
        response.status_code = 404;
    }
    printf("sending response...");
    send_response(client_socket_fd, response);
    printf("done\n");
}

int main() {

    disable_buffers();

    fill_phrases();

	int socket_fd = create_socket();

    bind_socket(socket_fd, PORT);

    start_listening(socket_fd, MAX_QUEUE_LENGTH);

    int served_clients = 0, fails=0;

    struct sockaddr_in client_address;
    while(served_clients <= 20){
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
            printf("enter handling\n");
            // handle_client(client_socket_fd);
            handle_client(2);
            printf("end handling\n");
            close(client_socket_fd);
            served_clients++;
    	}
    }
	
	close(socket_fd);    
    return 0; 
 }
