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


void server(char* nickname){
  int sockfd;
  int new_connection;
  socklen_t addrlen;
  struct sockaddr_in addr;
  struct sockaddr_in connected_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    perror("Couldn't create the socket.");
    exit(0);
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(22222);
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
  }
 
  char buff[100];
  char* client_name = malloc(100);
  read(new_connection, client_name, sizeof(client_name));
  write(new_connection, nickname, sizeof(nickname));
  while(1){
    bzero(buff, sizeof(buff));
    read(new_connection, buff, sizeof(buff));
    printf("\n%s says: %s\n", client_name,buff);
    bzero(buff, sizeof(buff));
    printf("%s says: ", nickname);
    char temp = '0';
    int i = 0;
    while(temp != '\n'){
      temp = getchar();
      buff[i]=temp;
      i++;
    }
    write(new_connection, buff, sizeof(buff));
  }
  close(sockfd);
}

void client(char* nickname, char* ip){
  int sockfd;
  struct sockaddr_in addr;
  struct in_addr ip_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1)
    perror("Couldn't create the socket");
  inet_pton(AF_INET, ip, &ip_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(22222);
  addr.sin_addr = ip_addr;
 
  if (connect (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
    perror("Connection problem.");
    exit(0);
  }
  
  char buff[100];
  char* server_name = malloc(100);
  write(sockfd, nickname, sizeof(nickname));
  read(sockfd, server_name, sizeof(server_name));
  
  while(1){
    bzero(buff, sizeof(buff));
    printf("%s says: ", nickname);
    char temp = '0';
    int i = 0;
    while(temp != '\n'){
      temp = getchar();
      buff[i]=temp;
      i++;
    }
    write(sockfd, buff, sizeof(buff));
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("\n%s says: %s\n",server_name, buff);
  }
  close(sockfd);
}

int main(int argc, char* argv[])
{
  char* nickname;
  char* ip;
   if(argc <= 1){
     printf("Arguments are not enough.");
     exit(0);
  }else if(argc == 2){
     nickname =  malloc(sizeof(argv[1]));
     strcpy(nickname, argv[1]);
     server(nickname);
   }else{
     nickname = malloc(sizeof(argv[1]));
     strcpy(nickname, argv[1]);
     ip = malloc(sizeof(argv[2]));
     strcpy(ip, argv[2]);
     client(nickname, ip);
  }
  
}

