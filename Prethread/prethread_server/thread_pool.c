#include "thread_pool.h"

void create_thread_pool(int n){
  for (int i = 0; i < n; i++) {
    pthread_t current_thread;
    thread_pool[i] = current_thread;
  }
}
