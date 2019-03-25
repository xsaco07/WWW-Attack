#include "../lib/utils.h"

#define MAX_QUEUE_LENGTH 1
#define MB 1000000
#define KB 1000
#define REQUEST_BUFFER_SIZE 1*KB
#define FILE_BUFFER_SIZE 10*MB
#define CHILD 0
#define NO_FREE_WORKERS -1 

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd){
    ssize_t     size;
    struct msghdr   msg;
    struct iovec    iov;
    union {
        struct cmsghdr  cmsghdr;
        char        control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr  *cmsg;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        printf ("passing fd %d\n", fd);
        *((int *) CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        printf ("not passing fd\n");
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        perror ("sendmsg");
    return size;
}



ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd){
    ssize_t     size;

    if (fd) {
        struct msghdr   msg;
        struct iovec    iov;
        union {
            struct cmsghdr  cmsghdr;
            char        control[CMSG_SPACE(sizeof (int))];
        } cmsgu;
        struct cmsghdr  *cmsg;

        iov.iov_base = buf;
        iov.iov_len = bufsize;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg (sock, &msg, 0);
        if (size < 0) {
            perror ("recvmsg");
            exit(1);
        }
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmsg->cmsg_level != SOL_SOCKET) {
                fprintf (stderr, "invalid cmsg_level %d\n",
                     cmsg->cmsg_level);
                exit(1);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS) {
                fprintf (stderr, "invalid cmsg_type %d\n",
                     cmsg->cmsg_type);
                exit(1);
            }

            *fd = *((int *) CMSG_DATA(cmsg));
            printf ("received fd %d\n", *fd);
        } else
            *fd = -1;
    } else {
        size = read (sock, buf, bufsize);
        if (size < 0) {
            perror("read");
            exit(1);
        }
    }
    return size;
}



void handle_client(int client_socket, char *path){

    // Read the filename
    char request_buffer[1024];

    read( client_socket, request_buffer, sizeof(request_buffer));
    http_request req;
    
    parse_http_request(request_buffer, &req);

    char filename[20];

    build_filename(path,req.uri, filename);

    FILE *file = fopen(filename, "r");

    http_response response;
    char file_buffer[15*1024];
    if(file){
        int file_size = copy_file(file, file_buffer);
        response.content_length = file_size;
        response.body = file_buffer;
        response.status_code = 200;
        fclose(file);
    }
    else{
        printf("File not found\n");
        response.status_code = 404;
    }
    printf("sending response...\n"); 
    send_response(client_socket, response);
    printf("sending response done\n");
}



void child_role(int parent_socket_fd){
    // Get the socket fd from parent
    char buf[16];
    int client_socket_fd = -1;
    int pid = getpid();
    printf("Child with pid %d waiting for work\n",pid);
    int size = 0;
    while(! size){
        sleep(0.5);
        size = sock_fd_read(parent_socket_fd, buf, sizeof(buf), &client_socket_fd);
    }
    printf("Child with pid %d received work\n",pid);
    if (size <= 0)
        printf("Child read nothing\n");
    printf ("read %d\n", (int)size);
    if (client_socket_fd != -1) {
        // Attend the request 
        printf("Child with pid %d entering handle_client\n",pid);
        handle_client(client_socket_fd,"www");
        printf("Child with pid %d done handle_client\n",pid);
        // close(client_socket_fd);
        // close(parent_socket_fd);
    }
    printf("Child with pid: %d finished\n",pid);
    exit(0);
}



void init_workers(int n, int *sockets_to_childs){
    for (int i = 0; i < n; ++i){
        int socket_fds[2];
        if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fds) < 0) {
            exit_on_error("Error creating socket pair");
        }
        int result = fork();
        if(result == -1){
            printf("Error forking\n");
        }
        else if(result == CHILD){
            close(socket_fds[0]);
            child_role(socket_fds[1]);
        }
        else{ //Parent Role
            close(socket_fds[1]);
            // Save the socket to child in the parameter
            *(sockets_to_childs+i) = socket_fds[0];
        }
    }
}



// Returns the index of a free worker of the array
int get_worker(int *sockets_to_childs, int *free_index, int max_workers){
    int result = NO_FREE_WORKERS;
    if(*(free_index) < max_workers){
        *(free_index)++;
        result = *(free_index);
    }
    return result;
}



int respond_server_full(int client_socket_fd){
    // build an mock html and send repsonde+}
}



void print_array(int *array, int size){
    printf("[");
    int i;
    for (i = 0; i < size-1; ++i){
        printf("%d, ",*(array+i));
    }
    printf("%d]\n",*(array+i));
}



int main(int argc, char *argv[]) {

    arguments args;
    parse_arguments(argc,argv,&args);

    disable_buffers();

    fill_phrases();

	int socket_fd = create_socket();

    bind_socket(socket_fd, args.port);

    start_listening(socket_fd, MAX_QUEUE_LENGTH);

    int served_clients=0, fails=0;

    // This will hold the socket pairs to communicate 
    // parent process to its childs
    int n = args.processes;
    int sockets_to_childs[n];
    init_workers(n, sockets_to_childs);
    int free_worker_index = 0;
    struct sockaddr_in client_address;
    while(served_clients < n){
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
            // Check if there's free workers
            int worker = get_worker(sockets_to_childs, &free_worker_index, n);
            if(worker == NO_FREE_WORKERS){
                // If not then respond with "full" message
                printf("Server is full\n");
                respond_server_full(client_socket_fd);
            }
            else{
                // If there is then send the client socket to 
                // the worker so it can handle the request
                sock_fd_write(worker, "1", 1, client_socket_fd);
            }
            served_clients++;
            // Back to accept
    	}
    }
	
	close(socket_fd);    
    return 0; 
 }
