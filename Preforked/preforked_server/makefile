server: build_server
	clear ; ./server -n 4 -w www/ -p 6001

client: build_client
	clear ; ./client
	 
build_client:
	gcc -g -o client ../lib/utils.c client.c

build_server:
	gcc -g -o server ../lib/utils.c server.c

all: server 