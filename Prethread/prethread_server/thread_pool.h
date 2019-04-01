#ifndef THREAD_POOL
#define THREAD_POOL

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define BUFFER_SIZE 10000

typedef struct prog{
  int NUM_CLIENT_THREADS;
  int AVAILABLE_THREADS;
  int ARGUMENT_PORT;
  char PATH_ROOT[100];
}prog;

prog PROGRAM;
pthread_t thread_pool[1000];
int threads_available;

void create_thread_pool(int n);

#endif
