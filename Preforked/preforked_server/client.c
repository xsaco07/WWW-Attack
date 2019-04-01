#include "../lib/utils.h"

int main(){

	int socket_fd = create_socket();
	int port;

	printf("port:"); scanf("%d",&port);
	connect_to_server(socket_fd, port);

	char filename[] = {"GET /a.html HTTP/1.1"};

	printf("request: %s\n", filename);
	send(socket_fd, filename, sizeof(filename), 0);
	printf("sent \n");
	
	FILE *down = fopen("down.txt","w");
	char buff[1024];
	printf("size %d\n",sizeof(buff));
	while(recv(socket_fd, buff, sizeof(buff), 0) > 0){
		fprintf(down,"%s",buff);
		printf("%s",buff);
	}
	printf("\nend receiving\n");
	fclose(down);
	close(socket_fd);

}
