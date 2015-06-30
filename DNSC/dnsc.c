#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h> 
#include<string.h> 
typedef struct dns_header{
  unsigned short ID; 
  
  unsigned char RD :1;
  unsigned char TC :1; 
  unsigned char AA :1; 
  unsigned char OPCODE :4; 
  unsigned char QR :1; 
  unsigned char RCODE :4; 
  unsigned char Z :3;
  unsigned char RA :1; 
  
  unsigned short QD_COUNT; 
  unsigned short AN_COUNT; 
  unsigned short NS_COUNT; 
  unsigned short AR_COUNT; 
}dns_header;

typedef struct dns_question{
  unsigned short Q_TYPE;
  unsigned short Q_CLASS;
}dns_question;

typedef struct question{
  unsigned char *Q_NAME;
  dns_question *q;
}question;

typedef struct dns_answer{
  unsigned short R_TYPE;
  unsigned short R_CLASS;
  unsigned int TTL;
  unsigned short RD_LENGTH;
}dns_answer;

typedef struct answer{
  unsigned char* R_NAME;
  dns_answer *a;
  unsigned char* R_DATA;
}answer;

int main(int argc, char *argv[])
{
  unsigned short q_t = 0;
  char* domain;
  char* server;
  
  if(argc == 4 && argc != 5){
    
    if(strcmp(argv[1], "-a") == 0){
      q_t = 0x0001;
      int len = strlen(argv[2]);
      domain = (char*)malloc(len+1);
      strcpy(domain, argv[2]);
      len = strlen(argv[3]);
      server = (char*)malloc(len+1);
      strcpy(server, argv[3]);
    }
  }else if(argc == 5){
    
    if(strcmp(argv[2], "A") == 0){
      q_t = 0x0001;
    }else if(strcmp(argv[2], "NS") == 0){
      q_t = 0x0002;
    }
    int len = strlen(argv[3]);
    domain = (char*)malloc(len+1);
    strcpy(domain, argv[3]);
    len = strlen(argv[4]);
    server = (char*)malloc(len+1);
    strcpy(server, argv[4]);
  }else{
    printf("Incorrect amount of arguments!\n");
    exit(0);
  }
  
  int num = 0 , i;
  
  strcat(domain,".");
  int len = strlen(domain);
  char* q_name = (char*)malloc(255);
  for(i = 0 ; i < len ; i++) 
    {		
      num++;
      if(domain[i] == '.'){
	char str[15];
	sprintf(str, "%d", num-1);
	strcat(q_name, str);
	
	char* substr = (char*)malloc(num);
	strncpy(substr, domain+(i-num+1), num-1);
	strcat(q_name, substr);
	
	num = 0;
      }       
    }
  
  unsigned char buff[65536];
  struct sockaddr_in addr;
  int s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); 
  
  addr.sin_family = AF_INET;
  addr.sin_port = htons(53);
  addr.sin_addr.s_addr = inet_addr(server); 
  
  dns_header *header = malloc(sizeof(struct dns_header));
  header->ID = (unsigned short) htons(getpid());
  header->RD = 1; 
  header->TC = 0; 		
  header->AA = 0; 
  header->OPCODE = 0; 
  header->QR = 0; 
  header->RCODE = 0;
  header->Z = 0;
  header->RA = 0; 
  header->QD_COUNT = htons(1); 
  header->AN_COUNT = 0;
  header->NS_COUNT = 0;
  header->AR_COUNT = 0;
  
  memcpy(&buff, header, sizeof(struct dns_header));
  memcpy(&buff[sizeof(struct dns_header)], q_name, strlen(q_name)+1);
  
  dns_question *q = malloc(sizeof(struct dns_question));
  q->Q_TYPE = htons( q_t ); 
  q->Q_CLASS = htons(1); 
  memcpy(&buff[sizeof(struct dns_header)+strlen(q_name) + 1], q, sizeof(struct dns_question));
  
  int total_size = sizeof(struct dns_header) + sizeof(struct dns_question) + strlen(q_name) + 1;
  
  if( sendto(s,(char*)buff,total_size,0,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    {
      perror("Send Error");
    }  
}

