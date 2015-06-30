#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


void client(char* ip, int port, char* filename){
  int sockfd;
  struct sockaddr_in addr;
  struct in_addr ip_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1)
    perror("Couldn't create the socket");
  inet_pton(AF_INET, ip, &ip_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = ip_addr;
 	
	printf("sending file %s to %s\n", filename, ip); 

  if (connect (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
		printf("can't connect to %s on tcp %i\n", ip, port);	
    perror("Connection problem.");
    exit(0);
  }else{
		printf("connected to %s on tcp %i\n", ip, port);	
  }

  write(sockfd, filename, 256);
		
	FILE *f = fopen(filename, "r");

	if (f == NULL){
		printf("can't open file %s\n", filename);	
		exit(0);
	}else{
		printf("open file %s\n", filename);
	}
	
	printf("begin sending file\n");
	
 
	char buff[512]; 	
	int size = fread(buff, sizeof(char), 512, f);
	while(size > 0){
		//if no characters were sent
		if(send(sockfd, buff, size, 0) < 0){
			printf("Transferring file failed\n");
			break;
		}
		bzero(buff, 512);
		size = fread(buff, sizeof(char), 512, f);
	}
	printf("tranfering file %s to %s done successfully\n", filename, ip);
	printf("closing file %s\n", filename);
	fclose(f);
	printf("closing socket\n");
  close(sockfd);
}

int main(int argc, char* argv[])
{
  
   if(argc <= 1){
     printf("Arguments are not enough.");
     exit(0);
  }else if(argc == 4){
    int port = atoi( argv[2] );
     client(argv[1], port, argv[3]);
  }
  
}

