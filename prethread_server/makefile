SERVER = server_pre_thread.c

server: build_server
	clear ; ./server -n 2 -w Resources -p 8061

build_server:
	gcc -pthread -o server ../lib/utils.c thread_pool.c $(SERVER)

all: server
