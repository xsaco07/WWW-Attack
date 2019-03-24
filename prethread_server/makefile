SERVER = server_pre_thread.c

server: build_server
	./server -n 5 -w main -p 9001

build_server:
	gcc -pthread -o server ../lib/utils.c thread_pool.c $(SERVER)

all: server
