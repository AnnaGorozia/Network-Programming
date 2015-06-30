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


void server(char* ip, int port){
  int sockfd;
  int new_connection;
  socklen_t addrlen;
  struct sockaddr_in addr;
  struct in_addr ip_addr;
  struct sockaddr_in connected_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    printf("can't open passive socket at %s : %i\n", ip, port);
    perror("Couldn't create the socket.");
    exit(0);
  }else{
    printf("opening passive socket at %s : %i\n", ip, port);
  }
  inet_pton(AF_INET, ip, &ip_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
 
  if (bind (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
    perror("Couldn't bind socket.");
    exit(0);
  }
 
  if (listen(sockfd, 10) == -1){
    perror("Problems with listening the socket.");
    exit(0);
  }
 
 
  addrlen = sizeof(struct sockaddr_in);
  new_connection = accept(sockfd, (struct sockaddr*)&connected_addr, &addrlen);

  if(new_connection == -1){
    perror("Connection problem");
    exit(0);
  }else{
    printf("accapted connection from client %s\n", ip);
  }

  char filename[256];
  memset(filename, '\0', 256);
  read(new_connection, filename, 256);
  printf("file: %s\n", filename);
  
  //filename = "text.txt";
  
  FILE *f = fopen(filename, "a");
  if(f == NULL){
    printf("can't open file %s\n", filename);
    exit(0);
  }else{
    printf("opened file %s to write byte stream\n", filename);
  }
  char buff[512]; 	
  int size;
  printf("start receiving byte stream\n");
  while(1){
    size = recv(sockfd, buff, 512, 0);
    if(size <= 0 || size != 512){
      break;
    }
    int writing = fwrite(buff, sizeof(char), 512, f);
    bzero(buff, 512);
    
  }
  printf("byte stream finished\n");
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
  }else if(argc == 3){
    int port = atoi( argv[2] );
    server(argv[1], port);
   }
  
}

