#include "thread_pool.h"
#include "../lib/utils.h"

void handle_conexion();
void *handle_client();
_Bool arguments_OK(int argc, char* argv[]);
void initiate_threads();
void save_arguments(char* argv[]);
void answering_client(int client_socket_fd, char buffer[], int limit_reached);
void show_threads_use();
void create_rejecting_clients_thread();
void* answering_when_limit_reached();

int NEW_CLIENT_SOCKET_FD = -1;
int MAIN_SOCKET_ID = -1;
int MAX_CLIENTS = 20;

char limit_reached_filename[32] = "/NO_MORE_CLIENTS.html";

pthread_mutex_t available_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t new_client_fd_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t new_client_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t available_threads_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_available_threads_condition = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {

  disable_buffers();

  if(arguments_OK(argc, argv)){
    save_arguments(argv);
    create_thread_pool(PROGRAM.NUM_CLIENT_THREADS);

    create_rejecting_clients_thread();

    MAIN_SOCKET_ID = create_socket();
    bind_socket(MAIN_SOCKET_ID, PROGRAM.ARGUMENT_PORT);
    start_listening(MAIN_SOCKET_ID, PROGRAM.NUM_CLIENT_THREADS);
    initiate_threads();
    handle_conexion();
    close(MAIN_SOCKET_ID);
  }
  else{
    printf("\nARGUMENTS ARE MISSING OR WRONG\n");
    print_error_status();
  }

  return 0;
}

void show_threads_use(){
  printf("\n<<<<Threads available: %d | Threads in use: %d>>>>\n",
  PROGRAM.AVAILABLE_THREADS, PROGRAM.NUM_CLIENT_THREADS - PROGRAM.AVAILABLE_THREADS);
}

void create_rejecting_clients_thread(){
  pthread_t thread;
  pthread_create(&thread, NULL, answering_when_limit_reached, NULL);
}

void handle_conexion(){

  while (1) {

    show_threads_use();
    int client_socket_fd = accept(MAIN_SOCKET_ID, NULL, NULL);
    show_threads_use();

    if(client_socket_fd == ERROR){
      printf("Error accepting new client\n");
      print_error_status();
    }
    else{
      printf("\nConexion established\n");

      // START CRITIC REGION -----------------------------------------------------
      // Update the socket fd that the threads are reading from.
      pthread_mutex_lock(&new_client_fd_mutex);
      NEW_CLIENT_SOCKET_FD = client_socket_fd;
      pthread_mutex_unlock(&new_client_fd_mutex);
      // END CRITIC REGION -----------------------------------------------------
      printf("\nThe client fd is = %d\n", NEW_CLIENT_SOCKET_FD);

      pthread_cond_broadcast(&new_client_condition); // Unlock all threads locked in this condition

      // START CRITIC REGION ---------------------------------------------------
      pthread_mutex_lock(&available_threads_mutex);
      PROGRAM.AVAILABLE_THREADS--;

      if (PROGRAM.AVAILABLE_THREADS == 0) {
        printf("\nSERVER HAS REACHED THE LIMIT OF CONCURRENT CLIENTS\n");
        pthread_cond_wait(&available_threads_condition, &available_threads_mutex);
      }

      pthread_mutex_unlock(&available_threads_mutex);
      // END CRITIC REGION -----------------------------------------------------
    }

  }
  pthread_exit(NULL);
}

void* answering_when_limit_reached(){
  while (1) {
    if(!PROGRAM.AVAILABLE_THREADS){

      printf("\nIm listening for clients who can not be attended.\n");
      int client_socket_fd = accept(MAIN_SOCKET_ID, NULL, NULL);

      if(client_socket_fd == ERROR){
        printf("Error accepting new client\n");
        print_error_status();
      }
      else{
        printf("\nConexion established\n");
        printf("\nThe client fd is = %d\n", client_socket_fd);
        printf("\nClient can not be attended.\nThere is no space.\n");

        char buffer[BUFFER_SIZE];
        recv(client_socket_fd, buffer, BUFFER_SIZE, 0);
        answering_client(client_socket_fd, buffer, TRUE);
        close(client_socket_fd);
      }
    }
  }
}

void* handle_client(){

  int client_socket_fd;
  char buffer[BUFFER_SIZE];

  while (1) {

    // START CRITIC REGION -----------------------------------------------------
    printf("\nNEW CLIENT STATUS = %d\n", NEW_CLIENT_SOCKET_FD);

    pthread_mutex_lock(&new_client_fd_mutex);
    while (NEW_CLIENT_SOCKET_FD == -1) {
      pthread_cond_wait(&new_client_condition, &new_client_fd_mutex);
    }

    show_threads_use();
    client_socket_fd = NEW_CLIENT_SOCKET_FD;
    NEW_CLIENT_SOCKET_FD = -1;
    pthread_mutex_unlock(&new_client_fd_mutex);

    // END CRITIC REGION -------------------------------------------------------

    printf("\nClient with socket_fd = %d will be attended.\n", client_socket_fd);

    if(client_socket_fd){

      printf("\nReading from client %d\n", client_socket_fd);

      recv(client_socket_fd, buffer, BUFFER_SIZE, 0);
      answering_client(client_socket_fd, buffer, FALSE);
      close(client_socket_fd);

      printf("\nClient %d attended succesfully\n", client_socket_fd);
      show_threads_use();

      pthread_mutex_lock(&available_threads_mutex);
      PROGRAM.AVAILABLE_THREADS++;
      pthread_mutex_unlock(&available_threads_mutex);

      show_threads_use();

      // Unlock the wait condition because there is space for another client
      pthread_cond_signal(&available_threads_condition);
    }
  }
  pthread_exit(NULL);
}

void answering_client(int client_socket_fd, char buffer[], int limit_reached){

  http_request req;
  parse_http_request(buffer, &req);

  char filename[20];
  if(!limit_reached) build_filename(PROGRAM.PATH_ROOT,req.uri, filename);
  else build_filename(PROGRAM.PATH_ROOT,limit_reached_filename, filename);

  printf("\nFile asked = %s\n", filename);

  FILE *file = fopen(filename, "r");

  http_response response;
  char file_buffer[15*1024];

  if(file){
    int file_size = copy_file(file, file_buffer);
    response.content_length = file_size;
    response.body = file_buffer;
    response.status_code = 200;
  }
  else{
    printf("\nFile not found\n");
    response.status_code = 404;
  }
  printf("\nSending response...\n");
  send_response(client_socket_fd, response);
  printf("\nDone\n");

  if(file)
    fclose(file);
}

_Bool arguments_OK(int argc, char* argv[]){
  return ((argc == 7) && (!strcmp(argv[1], "-n") && !strcmp(argv[3], "-w") && !strcmp(argv[5], "-p")));
}

void initiate_threads(){
  for (int i = 0; i < PROGRAM.NUM_CLIENT_THREADS; i++) {
    int initial_limit_flag = 0;
    if(pthread_create(&(thread_pool[i]), NULL, handle_client, NULL)){
      printf("\nError in creation of the thread #%d\n", i);
      print_error_status();
    }
    else printf("\nThread #%d created succesfully.\n", i);
  }
}

void save_arguments(char* argv[]){
  PROGRAM.NUM_CLIENT_THREADS = atoi(argv[2]);
  PROGRAM.ARGUMENT_PORT = atoi(argv[6]);
  PROGRAM.AVAILABLE_THREADS = PROGRAM.NUM_CLIENT_THREADS;
  strcpy(PROGRAM.PATH_ROOT, argv[4]);
}
