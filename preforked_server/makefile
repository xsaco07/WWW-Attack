server: build_server
	clear ; ./server

client: build_client
	clear ; ./client
	 
build_client:
	gcc -g -o client ../lib/utils.c client.c

build_server:
	gcc -g -o server ../lib/utils.c server.c

all: server 