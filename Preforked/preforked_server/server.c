
#include "../lib/utils.h"

#define MAX_QUEUE_LENGTH 1
#define MB 1000000
#define KB 1000
#define REQUEST_BUFFER_SIZE 1*KB
#define FILE_BUFFER_SIZE 10*MB
#define CHILD 0
#define NO_FREE_WORKERS -1

// COMMAND TO REQUEST clear ; cat port.txt | xargs -I % curl http://localhost:%/a.html

/*
    Read the request from the socket and responds it based on the received relative path
    Params:
        client_socket: where the function will read the request and write the response
        path: any resource requested will be searched within this path
 */
void handle_client(int client_socket, char *path){

    // Read the filename
    char request_buffer[1024];

    read( client_socket, request_buffer, sizeof(request_buffer));
    http_request req;

    parse_http_request(request_buffer, &req);

    char filename[20];

    int pid_file = validate_cgi_request(req.uri, filename, path);
    
    // build_filename(path,req.uri, filename);

    FILE *file = fopen(filename, "r");

    http_response response;
    if(file){
        long int file_size = get_file_size(filename);
        response.content_length = file_size;
        response.status_code = 200;

        send_response_header(client_socket, response);

        // Write the file to the socket by parts using a buffer
        write_file_to_socket(file, client_socket, file_size);
        if(pid_file){
            delete_temp_file(pid_file);
        }
    }
    else{
        printf("File not found: %s\n",filename);
        response.status_code = 404;
        printf("sending response...");
        send_response_header(client_socket, response);
        printf("sending response done\n");
    }

}



/*
    Passes the socket descriptor from parent process to a worker so it can serve the request
 */
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

//        printf ("passing fd %d\n", fd);
        *((int *) CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
//        printf ("not passing fd\n");
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        perror ("sendmsg");
    return size;
}



/*
    The worker reads the file descriptor of the socket by this function
 */
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
//            printf ("received fd %d\n", *fd);
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



/*
   Reads the key to access shared memory received from the parent process from the socket
   and return the value
 */
int receive_key(int parent_socket_fd){
    char buffer[20];
    recv(parent_socket_fd, buffer, sizeof(buffer), 0);
    // printf("key: %s\n",buffer);
    int key;
    sscanf(buffer,"%d",&key);
    return key;

}



/*
    Defines the behavior of the workers made with the fork() call
 */
void child_role(int parent_socket_fd){
    // Get the key to access shared memory from parent process
    int key = receive_key(parent_socket_fd);

    // Get the shared memory key from parent
    char *mem = get_shared_memory_segment(1,key);

    // Get the socket fd from parent
    char buf[16];
    int client_socket_fd = -1;
    int pid = getpid();

    while(1) {
        *mem = '1'; // Report available
        printf("Child with pid %d waiting for work\n",pid);
        int size = 0;
        while (!size) {
            sleep(0.5);
            size = sock_fd_read(parent_socket_fd, buf, sizeof(buf), &client_socket_fd);
        }
        *mem = '0'; // Report busy
        printf("Child with pid %d received work: %d\n", pid, client_socket_fd);
        if (size <= 0) {
            printf("Child read nothing\n");
        }
        // printf("read %d\n", (int) size);
        if (client_socket_fd != -1) {
            // Attend the request
            printf("Child with pid %d entering handle_client\n", pid);
            handle_client(client_socket_fd, "www");
            printf("Child with pid %d done handle_client\n", pid);
            close(client_socket_fd);
        }
    }
}




/*
    Creates n processes (workers) that runs child_role() function, creates the pointers to the shared
    memory that will hold the states of the workers (saved to char* array) and the keys to access the
    shared memory are saved to the int array
    Params:
        n: the number of workers to create
        sockets_to_childs: an array of ints where the socket descriptors to the workers will be saved
        child_states: an array of char* that hold the state of the workers
        keys: the array where the keys to access the shared memory will be saved
 */
void init_workers(int n, int *sockets_to_childs, char *childs_states[], int *keys){
    for (int i = 0; i < n; ++i){
        int key = 9990+i;
        childs_states[i] = get_shared_memory_segment(1, key);
        *(childs_states[i]) = '0';
        *(keys+i) = key;
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




/*
    Returns the index of a free worker of the array
    If there's no one available then returns NO_FREE_WORKER
 */
int get_worker(char *workers_states[], int max_workers){
    int index = NO_FREE_WORKERS;
    int found = 0; // Boolean flag
    for (int i = 0 ; i < max_workers && ! found ; ++i) {
        if (*(workers_states[i]) == '1') {
            index = i;
            found = 1;

        }
    }
    return index;
}



/*
    Sends an "Server full" html document to the client received
 */
int respond_server_full(int client_socket_fd){
    // build an mock html and send repsonde
    // This means the server is full and must send an "SERVER FULL MSG"
	http_response response;
    response.status_code = 200;
    char *full_msg = "<html> \n\
    				    <head> \n\
    				    </head> \n\
    				    <body> \n\
        				    <h1> \n\
        				    	SERVER IS AT FULL CAPACITY, PLEASE COME BACK LATER \n\
        				    </h1> \n\
    				    </body> \n\
    				  </html>";
    response.content_length = strlen(full_msg);
    send_response_header(client_socket_fd, response);
    send(client_socket_fd, full_msg, strlen(full_msg),0);
}




/*
    Prints an array of ints
 */
void print_array(int *array, int size){
    printf("[");
    int i;
    for (i = 0; i < size-1; ++i){
        printf("%d, ",*(array+i));
    }
    printf("%d]\n",*(array+i));
}




/*
    Prints an array of char* (the char pointed by the pointer)
 */
void print_char_array(char **array, int size){
    printf("[");
    int i;
    for (i = 0; i < size-1; ++i){
        printf("%c, ",*(array[i]));
    }
    printf("%c]\n",*(array[i]));
}





/*
    The main procedure of the Prefork HTTP Server
 */
int main(int argc, char *argv[]) {

    // Parse arguments received from terminal
    arguments args;
    parse_arguments(argc,argv,&args);
    disable_buffers();

    // Make the map of status_code-to-phrase of http codes
    fill_phrases();

    //Create the socket for receiving HTTP requests
    int socket_fd = create_socket();

    bind_socket(socket_fd, args.port);

    start_listening(socket_fd, MAX_QUEUE_LENGTH);


    // Because some data structures have the same size as the number of processes to use
    int n = args.processes;

    // This will hold the sockets to communicate this parent process to its children (workers)
    int sockets_to_workers[n];

    // This array has the pointers to the byte that says the state of the worker {'1'=available '0'=busy}
    char *workers_states[n];

    // This holds the keys to access the memory (state byte) shared with the worker
    int keys[n];

    // Make the forks, get the shared bytes, and get the keys to share memory with the workers
    init_workers(n, sockets_to_workers, workers_states, keys);

    // See keys
    // print_array(keys,n);

    // See states
    // print_char_array(workers_states, n);

    // Send the key to each worker so it can report its state
    char b[10];
    for (int i = 0; i <n ; ++i) {
        sprintf(b,"%d",keys[i]);
        send(sockets_to_workers[i], b, sizeof(b),0);
    }


    while(1){

        printf("Waiting clients at %d \n",args.port);
        // printf("Before accept "); print_char_array(workers_states, n); printf("\n");
        int client_socket_fd = accept(socket_fd, NULL, NULL);

        if(client_socket_fd == ERROR){
            exit_on_error("Error accepting client\n");
        }
        else{
            // Check if there's free workers
            int worker_index = get_worker(workers_states, n);
            if(worker_index == NO_FREE_WORKERS){

                // If not then respond with "full" message
                printf("Server is full\n");
                respond_server_full(client_socket_fd);
            }
            else{
                 // If there is then send the client socket to the worker so it can handle the request
                 sock_fd_write(sockets_to_workers[worker_index], "1", 1, client_socket_fd);
            }
            // printf("After sending work: "); print_char_array(workers_states, n); printf("\n");
            //Back to accept
        }
    }

    close(socket_fd);
    return 0;
}
