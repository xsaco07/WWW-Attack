#include "thread_pool.h"
#include "../lib/utils.h"

void* handle_conexion();
void handle_client(int client_socket_fd);
_Bool arguments_OK(int argc, char* argv[]);
void initiate_threads();
void save_arguments(char* argv[]);

int main(int argc, char *argv[]) {

  if(arguments_OK(argc, argv)){
    save_arguments(argv);
    create_thread_pool(NUM_CLIENT_THREADS);
    initiate_threads();
  }
  else{
    printf("\nARGUMENTS ARE MISSING OR WRONG\n");
    print_error_status();
  }

  return 0;
}

void* handle_conexion(){

  int socket_fd = create_socket();
  bind_socket(socket_fd, ARGUMENT_PORT);
  start_listening(socket_fd, NUM_CLIENT_THREADS);

  struct sockaddr_in client_address;
  while (AVAILABLE_THREADS) {
    printf("\nThreads available: %d | Threads in use: %d\n",
    AVAILABLE_THREADS, NUM_CLIENT_THREADS - AVAILABLE_THREADS);

    int client_socket_fd = accept(socket_fd, (struct sockaddr *) &client_address,
    (socklen_t*) &client_address);

    if(client_socket_fd == ERROR){
      printf("Error accepting client\n");
      print_error_status();
    }
    else{
      AVAILABLE_THREADS--;
      handle_client(client_socket_fd);
      close(client_socket_fd);
      AVAILABLE_THREADS++;
    }

  }
  return NULL;
}

void handle_client(int client_socket_fd){

  char buffer[BUFFER_SIZE];
	read( client_socket_fd, buffer, BUFFER_SIZE );
	http_request request;
  parse_http_request(buffer, &request);

  char filename[100];
  int offset = sprintf(filename, PATH_ROOT);
  sprintf(filename+offset, request.uri+1);

  printf("\nFile asked: %s\n",filename);

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
      printf("Response sent: %s \n",response);
      // Close the requested file
      fclose(file);
  }
  else{
      printf("File requested not found\n");
  }
}

_Bool arguments_OK(int argc, char* argv[]){
  return ((argc == 7) && (!strcmp(argv[1], "-n") && !strcmp(argv[3], "-w") && !strcmp(argv[5], "-p")));
}

void initiate_threads(){
  for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
    if(pthread_create(&(thread_pool[i]), NULL, handle_conexion, NULL)){
      printf("Error in creation of the thread #%d\n", i);
      print_error_status();
    }
    else printf("Thread #%d created succesfully.\n", i);
  }
}

void save_arguments(char* argv[]){
  NUM_CLIENT_THREADS = atoi(argv[2]);
  ARGUMENT_PORT = atoi(argv[6]);
  //PATH_ROOT = argv[4];
  strcpy(PATH_ROOT, argv[4]);
  AVAILABLE_THREADS = NUM_CLIENT_THREADS;
}
