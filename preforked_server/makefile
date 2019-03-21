server: build_server
	clear ; ./server

client: build_client
	clear ; ./client
	 
build_client:
	gcc -o client ../lib/utils.c client.c

build_server:
	gcc -o server ../lib/utils.c server.c

all: server 