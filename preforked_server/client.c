#include "../lib/utils.h"

int main(){

	int socket_fd = create_socket();
	connect_to_server(socket_fd, PORT);

	char filename[] = {"GET /a.html HTTP/1.1"};

	printf("request: %s\n", filename);
	send(socket_fd, filename, sizeof(filename), 0);
	printf("sent \n");
	
	FILE *down = fopen("down.txt","w");
	char buff[1024];
	recv(socket_fd, buff, sizeof(buff), 0);
	while(recv(socket_fd, buff, sizeof(buff), 0))
		fprintf(down,"%s",buff);
	printf("end receiving\n");
	fclose(down);
	close(socket_fd);

}
