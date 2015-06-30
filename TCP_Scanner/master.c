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
static int const buff_size = 1024;

char* intToIp(int n){
  struct in_addr addr = {htonl(n)};
  return inet_ntoa( addr );  
}

typedef struct data
{
  unsigned int ip;
  int range;
  int portNum;
} data;

void scan (int sock, unsigned int network,int range,char* argv[], int portNum){
  int i; 	
  data data;
  data.range = range;
  data.ip = network;
  data.portNum = portNum;
  //send data struct
  write(sock, &data, sizeof(data));
  int p;
  for(i = 0; i < portNum; i++){
    p = atoi(argv[i]);
    write(sock, &p, sizeof(int));
  }
  char buff[buff_size];
  while(1){
    i = read(sock, &buff, sizeof(buff));
    if(i == 0) break;
    printf("%s", buff);
  }
}

unsigned int convert(char* string){
  int a = atoi(strtok(string, "."));
  int b = atoi(strtok(NULL, "."));
  int c = atoi(strtok(NULL, "."));
  int d = atoi(strtok(NULL, "."));
  unsigned int ipInteger = a * pow(256, 3) + b * pow(256, 2) + c* pow(256, 1) + d;
  return ipInteger;
}

void pr(char* argv[], int num){
  int j;
  for(j = 0; j < num; j++){
    printf("%i\n",atoi(argv[j]));
  }
}


int main(int argc, char* argv[]){
  int sockfd;
  int new_connection;
  socklen_t addrlen;
  struct in_addr ip_addr;
  struct sockaddr_in addr;
  struct sockaddr_in connected_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1){
    perror("Couldn't create the socket.");
    exit(1);
  }
  //inet_pton(AF_INET, "0.0.0.0", &ip_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  //addr.sin_addr = ip_addr;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if (bind (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
    perror("Couldn't bind socket.");
    exit(1);
  }
  
  if (listen(sockfd, 10) == -1){
    perror("Problems with listening the socket.");
    exit(1);
  }
 
  addrlen = sizeof(connected_addr);
  unsigned int h = convert(argv[1]);	
  unsigned int s = convert(argv[2]);
  unsigned int broadcast = h|~s;
  unsigned int network = h&s;
  
  unsigned int hosts = broadcast - network - 2;
  network = network + 1;
  int portNum = argc - 3;
  int range = 1;
  while(1){
    new_connection = accept(sockfd, (struct sockaddr*)&connected_addr, &addrlen);
    if(new_connection <= 0){
      perror("Connection problem");
      exit(1);
    }
    
    if(range >= hosts){
      scan(new_connection, network, hosts, &argv[3], portNum);
      network = network + hosts;
      break;
    }else{
      scan(new_connection, network, range, &argv[3], portNum);
      network = network + range;
      hosts = hosts - range;
      range = range * 2;
    }
    close(new_connection); 
  }
  close(sockfd);
}

