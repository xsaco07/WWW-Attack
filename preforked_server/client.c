#include "../lib/utils.h"

int main(){

	int socket_fd = create_socket();
	connect_to_server(socket_fd, PORT);

	char filename[] = {"GET /hola.html HTTP/1.1"};

	printf("request: %s\n", filename);
	send(socket_fd, filename, sizeof(filename), 0);
	printf("sent \n");
	
	char buff[1024];
	bzero(buff, sizeof(buff));
	recv(socket_fd, buff, sizeof(buff), 0);
	printf("%s",buff);
	close(socket_fd);

}
