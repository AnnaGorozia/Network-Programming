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
#include "uthash.h"
#include <pthread.h>
#include <semaphore.h> 
#include <string.h>

pthread_mutex_t lock;
int counter = 0;

struct client_info {
  int c_sock;
  unsigned long c_ip;
  int c_port;
  int s_port;
  char** servers;
  int num_of_servers;
}; 

struct entry {
  char id[22];
  unsigned int c_IP;
  int port;
  unsigned long server;
  UT_hash_handle hh; /* makes this structure hashable */
};


struct entry *map = NULL;

void add_elem(struct entry *s) {
  HASH_ADD_STR(map, id, s);    
}

struct entry *find_elem(char ip[]) {
  struct entry *s;
  HASH_FIND_STR(map, ip, s);  
  return s;
}

void delete_elem(struct entry *elem) {
  HASH_DEL(map, elem);  
}

void *connection_handler(void *data)
{
  //Get the socket descriptor
  struct client_info *c_i;
  c_i = (struct client_info *)data;
  int sock = c_i->c_sock;
  int read_size;
  char client_message[2000];
  int num_servers = c_i->num_of_servers;
  pthread_mutex_lock (&lock);
  counter++;
  pthread_mutex_unlock (&lock);
  //Receive a message from client
  
  int server_n = (counter-1)%num_servers;
  struct entry *e = malloc(sizeof(struct entry));
  unsigned int temp;
  char* res = malloc(22);
  struct in_addr ip_addr;
  ip_addr.s_addr = c_i->c_ip;
  
  strcpy(res,inet_ntoa(ip_addr));
  strcat(res, ":");
  char str_port[5];
  sprintf(str_port,"%u", c_i->c_port); 
  strcat(res, str_port);
  
  unsigned long server_addr;
  //struct entry * found = NULL;	
  struct entry * found = malloc(sizeof(struct entry));
  found = find_elem(res);
  int sending_port = c_i->s_port;
  
  if(found == NULL){
    strcpy(e->id, res);
    e->c_IP = htonl(c_i->c_ip);
    e->port = c_i->c_port;
    struct in_addr addr;
    inet_aton(c_i->servers[server_n], &addr);
    e->server = addr.s_addr;
    server_addr = addr.s_addr;
    add_elem(e);	
    found = find_elem(res);
  }else{
    server_addr = found->server;
  }
  
  free(res);
  while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
    //end of string marker
    client_message[read_size] = '\0';
    int sockfd = 0;
    int n = 0;
    struct sockaddr_in serv_addr;
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
      perror("socket");
      
    }
    struct in_addr addr;
    addr.s_addr = server_addr;
    printf("TCP %s > %s:%i\n",found->id,inet_ntoa(addr),sending_port);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(sending_port);
    serv_addr.sin_addr.s_addr = server_addr;
    int i;
    
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
      for(i = 0; i < num_servers; i++){
	struct in_addr addr;
	inet_aton(c_i->servers[i], &addr);
	server_addr = addr.s_addr;
	serv_addr.sin_addr.s_addr = htons(server_addr);
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0){
	  found->server = server_addr;
	  add_elem(found);
	  break;						
	}else perror("connect");;				
      }
    } 
    
    if(write(sockfd, client_message, 2000) < 0){
      for(i = 0; i < num_servers; i++){
	struct in_addr addr;
	inet_aton(c_i->servers[i], &addr);
	server_addr = addr.s_addr;
	serv_addr.sin_addr.s_addr = htons(server_addr);
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0){
	  if(write(sockfd, client_message, 2000) >= 0){						
	    found->server = server_addr;
	    add_elem(found);
	    break;
	  }perror("write");						
	}perror("connect");			
      }
    } 
    
    //clear the message buffer
    memset(client_message, 0, 2000);
    if((n = read(sockfd, client_message, 1999)) < 0){
      for(i = 0; i < num_servers; i++){
	struct in_addr addr;
	inet_aton(c_i->servers[i], &addr);
	server_addr = addr.s_addr;
	serv_addr.sin_addr.s_addr = htons(server_addr);
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0){
	  if(write(sockfd, client_message, 2000) >= 0){
	    if((n = read(sockfd, client_message, 1999)) < 0){				
	      found->server = server_addr;
	      add_elem(found);
	      break;
	    }perror("read");
	  }perror("write");						
	}perror("connect");			
      }
    } 
    addr.s_addr = server_addr;
    printf("TCP %s > %s:%i\n",found->id,inet_ntoa(addr),sending_port);
    client_message[n] = 0;
    //send to client response received from server
    write(sock, client_message ,n+1);
    
  }
  
  if(read_size == 0)
    {
      puts("Client disconnected");
      fflush(stdout);
    }
  else if(read_size == -1)
    {
      perror("recv failed");
    }
  
  return 0;
} 

void tcp(int argc, char *argv[]){
  puts("It's tcp!");
  int port = atoi(argv[2]);
  
  int numOfServers = argc - 3;
  
  int firstServerIndex = argc - numOfServers;
  
  int socket_desc, client_sock, c;
  struct sockaddr_in server, client;
  
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_desc == -1){
    perror("socket");
    exit(1);
  }
  
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);	
  
  if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
    perror("bind");
    exit(1);
  }
  
  listen(socket_desc, 10);
  
  c = sizeof(struct sockaddr_in);
  pthread_t thread_id;
  
  while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
    puts("Connection accepted");
    struct client_info c_i;
    c_i.c_sock = client_sock;
    c_i.c_ip = client.sin_addr.s_addr;
    c_i.c_port = (int) ntohs(client.sin_port); 
    c_i.servers = &argv[3];
    c_i.num_of_servers = numOfServers; 
    c_i.s_port = port;
    if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &c_i) < 0){
      perror("pthread_create");
      exit(1);
    }
    
  }
  
  if (client_sock < 0)
    {
      perror("accept");
      exit(1);
    }
  
}
void get_host(char* temp,char* host_name){
  char *tok = NULL;
  tok = strtok(temp, " ");
  tok = strtok(NULL, " ");
  strcpy(host_name, tok);
}
void* connection_handler_http(void* data){
  //Get the socket descriptor
  struct client_info *c_i;
  char client_message[2000];
  c_i = (struct client_info *)data;
  int sock = c_i->c_sock;
  int read_size = 0;
  int num_servers = c_i->num_of_servers;
pthread_mutex_lock (&lock);
  counter++;
  pthread_mutex_unlock (&lock);
  while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
    //end of string marker
    client_message[read_size] = '\0';
    char *token = NULL;
    char c_message[2000];
    strcpy(c_message,client_message);
    token = strtok(c_message, "\n");
    int l = 0;
    char* host_name;
    char* temp_host = malloc(256);
    while (token) {
      l = l + 1; 
      if(l == 2){
	memset(temp_host, '\0', 256);
	strcpy(temp_host, token);
      }
      token = strtok(NULL, "\n");
    }
    host_name = malloc(256);
    memset(host_name, '\n', 256);
    get_host(temp_host, host_name);
    printf("host: %s\n",host_name);
    int num_servs = 0;
    puts(client_message);
    int i;
    char** servs;
    for(i = 0; i < num_servers; i++){
      char* temp = malloc(256);
      strcpy(temp, c_i->servers[i]);
      token = strtok(temp, "=");
      int num;
      for (num=0; c_i->servers[i][num]; c_i->servers[i][num]==',' ? num++ : *c_i->servers[i]++);
      
      if(strstr(host_name, token) != NULL){
	int size = num + 1;
	servs = calloc(size, sizeof(char*));
	while(token){
	  if(num_servs == size) break;
	  servs[num_servs] = token;
	  num_servs = num_servs + 1;
	  token = strtok(NULL, ",");	  
	}		
	break;		
      }
    }
    
    int server_index = (counter-1)%num_servs;
    struct in_addr addr;
    inet_aton(servs[server_index], &addr);
    
    int sockfd = 0;
    int n = 0;
    struct sockaddr_in serv_addr;
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
      perror("socket");
      
    }
    unsigned long server_addr = addr.s_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    serv_addr.sin_addr.s_addr = server_addr;
    
    
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
      perror("connect");
    } 
    
    if(write(sockfd, client_message, 2000) < 0){
      perror("write");
    } 
    
    //clear the message buffer
    memset(client_message, 0, 2000);
    if((n = read(sockfd, client_message, 1999)) < 0){
      perror("read");
    } 
    struct in_addr addr2;
    addr2.s_addr = c_i->c_ip;
    addr.s_addr = server_addr;
    printf("TCP %s:%i > %s:%i\n",inet_ntoa(addr2),c_i->s_port,inet_ntoa(addr),c_i->s_port);
    client_message[n] = 0;
    //send to client response received from server
    write(sock, client_message ,n+1);
    free(servs);
  }
  
  if(read_size == 0)
    {
      puts("Client disconnected");
      fflush(stdout);
    }
  else if(read_size == -1)
    {
      perror("recv failed");
    }
  
  return 0;
}
void http(int argc, char *argv[]){
  puts("It's http!");
  int port = 80;
  int socket_desc, client_sock, c;
  struct sockaddr_in server, client;
  
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  
  if(socket_desc == -1){
    perror("socket");
    exit(1);
  }
  
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);	
  
  if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
    perror("bind");
    exit(1);
  }
  
  listen(socket_desc, 10);
  
  c = sizeof(struct sockaddr_in);
  pthread_t thread_id;
  
  while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
    puts("Connection accepted");
    struct client_info c_i;
    c_i.c_sock = client_sock;
    c_i.c_ip = client.sin_addr.s_addr;
    c_i.c_port = (int) ntohs(client.sin_port); 
    c_i.servers = &argv[2];
    c_i.num_of_servers = argc - 2; 
    c_i.s_port = port;
    if( pthread_create( &thread_id , NULL , connection_handler_http , (void*) &c_i) < 0){
      perror("pthread_create");
      exit(1);
    }
    
  }
  
  if (client_sock < 0)
    {
      perror("accept");
      exit(1);
    }
}

int main(int argc, char *argv[])
{
  if(argc <= 2){
    puts("Arguments are not enough!");		
    exit(1);
  }
  if(strcmp(argv[1],"tcp") == 0){
    tcp(argc, argv);
  }else if(strcmp(argv[1],"http") == 0){
    http(argc, argv);
  }
}
