#include "../lib/utils.h"

#define MAX_QUEUE_LENGTH 1
#define MB 1000000
#define KB 1000
#define REQUEST_BUFFER_SIZE 1*KB
#define FILE_BUFFER_SIZE 10*MB

void foo(){return;}


int main(int argc, char *argv[]) {

    arguments args;
    parse_arguments(argc,argv,&args);

    disable_buffers();

    fill_phrases();

	int socket_fd = create_socket();

    bind_socket(socket_fd, args.port);

    start_listening(socket_fd, MAX_QUEUE_LENGTH);

    int served_clients = 0, fails=0;

    struct sockaddr_in client_address;
    while(served_clients < 10){
        printf("Waiting clients at %d | served: %d received: %d\n",args.port,served_clients, fails);
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
            handle_client(client_socket_fd, args.path);
            printf("end handling\n");
            close(client_socket_fd);
            served_clients++;
    	}
    }
	
	close(socket_fd);    
    return 0; 
 }
