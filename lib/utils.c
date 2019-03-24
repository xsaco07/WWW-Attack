#include "utils.h"

void disable_buffers(){
	setbuf(stdout,NULL);
}

void exit_on_error(const char *error){
	printf("[ERROR] %s \n", error);
	print_error_status();
	printf(">>> Program finished.\n");
	exit(1);
}

void print_error_status(){
	printf("Error status: %s\n", strerror(errno));
}

int create_socket(){

	int socket_fd;

    // Create the socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if ( socket_fd  == ERROR) { 
       	exit_on_error("Error creating the socket");
    } 
}

void bind_socket(int socket_fd, int port){
	struct sockaddr_in address; 
    address.sin_family = AF_INET; 
    address.sin_port = htons( port ); 
    address.sin_addr.s_addr = INADDR_ANY;

    int address_length = sizeof(address);      

    // Bind the socket to the port
    int result = bind(socket_fd, (struct sockaddr *) &address, address_length );
    if(result == ERROR ){ 
    	close(socket_fd);
    	exit_on_error("Error binding the socket to port");
    } 
}


void start_listening(int socket_fd, int max_clients){
	int result = listen(socket_fd, max_clients);
    if (result == ERROR){ 
    	close(socket_fd);
        exit_on_error("Error starting to listen");
    } 
}

void connect_to_server(int socket_fd, int port){
    //Specify an address for the socket
    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(port);
    client_address.sin_addr.s_addr = INADDR_ANY;

    //Making a connection
    int connection_status = connect(socket_fd, (struct sockaddr *)&client_address, sizeof(client_address));

    if(connection_status == -1){
    	close(socket_fd);
        exit_on_error("Error connecting to server");
    }
}


void parse_http_request(char *string, http_request *request){
	char method[20], uri[256], protocol[20];
	sscanf(string,"%s %s %s", method, uri, protocol); 
	printf("-method:%s\n", method);
    printf("-uri:%s\n", uri);
    printf("-protocol:%s\n", protocol);

	if( strcmp(protocol,"HTTP/1.1") != EQUALS)
		exit_on_error("Unknown HTTP protocol");
	
	if(strcmp(method,"GET"))
		request->method = GET;
	else if (strcmp(method,"POST"))
		request->method = POST;
	else if (strcmp(method,"DELETE"))
		request->method = DELETE;
	else if (strcmp(method,"PUT"))
		request->method = PUT;
	else
		exit_on_error("Unknown HTTP method");
	
	strcpy(request->uri, uri);
	strcpy(request->protocol, protocol);

}

void parse_http_response(char *string, http_response *response){
	return;
}

void build_filename(char *folder, char *uri, char *dest){
    // Add the final "/" to the folder name 
    int offset = sprintf(dest,"%s/",folder);
    // +1 to skip the starting "/" of the uri
    offset += sprintf(dest+offset,"%s",uri+1);

}

void send_response(int socket_fd, http_response response){
    // All the response will be written here
    char buffer[10*1024];
    char *protocol = "HTTP/1.1";
    // This is used to track where to write the next time 
    int offset = sprintf(buffer,"%s %d %s\n",protocol,response.status_code,phrases[response.status_code]);

    if(response.status_code == 200){
        offset += sprintf(buffer+offset,"Content-length: %d\n",response.content_length);
        offset += sprintf(buffer+offset,"\n");
        //Write exactly the size of the body 
        memcpy(buffer+offset, response.body, response.content_length);
        offset += response.content_length;
    }

    int bytes_sent = send(socket_fd, buffer, offset,0);  
    printf("%d bytes sent content: %s\n",bytes_sent,buffer);
}

int copy_file(FILE *file, char *buffer){
    int bytes_count = 0;
    int bytes_read=0;
    while ((bytes_read = fread(buffer+bytes_count, 1, 1024, file)))
        bytes_count += bytes_read;
    return bytes_count;
}

void fill_phrases(){
    phrases[404] = "Not Found";
    phrases[200] = "OK";
}

void parse_arguments(int argc, char *argv[], arguments *arguments){
    if(argc != 7){
        exit_on_error("Invalid amount of arguments");
    }

    char current_option_char; 
    while((current_option_char = getopt(argc, argv, "n:w:p:")) != -1){
        switch(current_option_char){
            case 'n':
                sscanf(optarg,"%d",&arguments->processes);
                break;
            case 'w':
                strcpy(arguments->path,optarg);
                break;
            case 'p':
                sscanf(optarg,"%d",&arguments->port);
                break;
        }
    }
}