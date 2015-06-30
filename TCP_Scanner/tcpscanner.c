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
#include <pthread.h>
static int const buff_size = 1024;
unsigned int convert(char* string){
  int a = atoi(strtok(string, "."));
  int b = atoi(strtok(NULL, "."));
  int c = atoi(strtok(NULL, "."));
  int d = atoi(strtok(NULL, "."));
  unsigned int ipInteger = a * pow(256, 3) + b * pow(256, 2) + c * pow(256, 1) + d;
  return ipInteger;
}

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

typedef struct str_thdata
{
  unsigned int ip;
  int range;
  int port;
  int thread_num;
  int sock;
} thdata;

int isOpen(int sockfd, int ip, int port){
  struct sockaddr_in addr;
  struct in_addr ip_addr;
  inet_pton(AF_INET, intToIp(ip), &ip_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = ip_addr; 
  if (connect (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))==0){
    return 1;
  }
  return 0;
}
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int* arr;
void scanner ( void *ptr )
{
  thdata *data;            
  data = (thdata *) ptr;  
  int range = data->range;
  int ip = data->ip;
  int port = data->port;
  int tn = data->thread_num;
  int sockfd = data->sock;
  int i;
  char buff[buff_size];
  
  pthread_mutex_lock(&mutex);
  for(i = 0; i < range; i++){
    ip = ip + 1;
    int check = isOpen(sockfd, ip, port); 
    if(check == 1){
      snprintf(buff, sizeof(buff), "%s port: %i is open\n", intToIp(ip), port);
    }else{
      snprintf(buff, sizeof(buff), "%s port: %i is closed\n", intToIp(ip), port);    
    }
    write(sockfd, &buff, sizeof(buff));
  }
  pthread_mutex_unlock(&mutex);
  pthread_exit(0); 
}

int main(int argc, char* argv[])
{
  int sockfd;
  int i;
  data recData;
  struct sockaddr_in addr;
  struct in_addr ip_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1)
    perror("Couldn't create the socket");
  inet_pton(AF_INET, argv[1], &ip_addr);
  addr.sin_family = AF_INET;
  int port = atoi( argv[2] );
  addr.sin_port = htons(port);
  addr.sin_addr = ip_addr; 
  if (connect (sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
    perror("Connection problem.");
    exit(0);
  }
  
  read(sockfd, &recData, sizeof(recData));
  int portNum = recData.portNum;
  
  pthread_t *threads;
  threads = (pthread_t*)calloc(portNum, sizeof(pthread_t));
  thdata thread_data;
  thread_data.ip = recData.ip;
  thread_data.range = recData.range;
  thread_data.sock = sockfd;
  for(i = 0; i < portNum; i++){
    read(sockfd, &thread_data.port, sizeof(int)); 
    thread_data.thread_num = i+1;
    int e = pthread_create (&threads[i], NULL, (void *) &scanner, (void *) &thread_data);
    if(e == 0){
      printf("port sent to thread %i\n", thread_data.port);
    }
  }
  for(i = 0; i < portNum; i++){
    pthread_join(threads[i], NULL);
  }
  free(threads);
  close(sockfd);
}
