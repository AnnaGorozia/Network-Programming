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
#define MAX_ROUTE_ENTRY 25
#define PORT 520
#define DST_IP "224.0.0.9"
#define BUF_SIZE 504
typedef struct header{
  int8_t command;
  int8_t version;
  short unused;
}header;

sem_t mutex;

/** a single entry in the routing table */
typedef struct entry {
  short family;
  short tag;
  uint32_t network_addr;
  uint32_t subnet_mask;
  uint32_t next_hop;
  uint32_t metric;
  
  UT_hash_handle hh; /* makes this structure hashable */
} entry;

unsigned int convert(char* string){
  int a,b,c,d;
  
  a = atoi(strtok(string, "."));
  b = atoi(strtok(NULL, "."));
  c = atoi(strtok(NULL, "."));
  d = atoi(strtok(NULL, "."));
  
  unsigned int ipInteger = a * pow(256, 3) + b * pow(256, 2) + c* pow(256, 1) + d;
  return ipInteger;
  
}
char* intToIp(int n){
  struct in_addr addr = {htonl(n)};
  return inet_ntoa( addr );  
}

struct entry *map = NULL;

void add_elem(struct entry *s) {
  HASH_ADD_INT(map, network_addr, s);    
}

struct entry *find_elem(unsigned int ip) {
  struct entry *s;
  HASH_FIND_INT(map, &ip, s);  
  return s;
}

void delete_elem(struct entry *elem) {
  HASH_DEL(map, elem);  
}

void recieveUpdate(){
  struct sockaddr_in addr;
  int fd, nbytes,addrlen;
  struct ip_mreq mreq;
  char buf[BUF_SIZE];
  
  u_int yes=1;        
  
  /* create what looks like an ordinary UDP socket */
  if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    exit(1);
  }
  
  /**** MODIFICATION TO ORIGINAL */
  /* allow multiple sockets to use the same PORT number */
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
    perror("Reusing ADDR failed");
    exit(1);
  }
  /*** END OF MODIFICATION TO ORIGINAL */
  
  /* set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
  addr.sin_port=htons(PORT);
  
  /* bind to receive address */
  if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }
  
  printf("\nlisten on socket UDP %i\n", PORT); 
  
  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(DST_IP);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  
  /* now just enter a read-print loop */
  while (1) {
    addrlen=sizeof(addr);
    if ((nbytes=recvfrom(fd,buf,BUF_SIZE,0,(struct sockaddr *) &addr,&addrlen)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    printf("recived RIP update from %s\n", intToIp(addr.sin_addr.s_addr));
    int entrySize = sizeof(entry)-sizeof(UT_hash_handle);
    int i;
    int numOfEntries = (nbytes - sizeof(header))/entrySize;
    for(i = 0; i < numOfEntries; i++){
      entry *e = malloc(entrySize);
      memcpy(e, &buf[4+i*entrySize], entrySize);
      uint32_t network = e->network_addr;
      struct entry *entr = find_elem(network);
      if(entr == NULL && e->metric <= 15){
	sem_wait(&mutex); 
	add_elem(entr);
	printf("%s/%s Metric %u",intToIp(entr->network_addr), intToIp(entr->subnet_mask), entr->metric);
	sem_post(&mutex); 
      }else{
	if(entr->network_addr == 2130706432) continue;
	if(e->metric < entr->metric && e->metric <= 15){
	  sem_wait(&mutex);
	  printf("%s/%s Metric %u",intToIp(entr->network_addr), intToIp(entr->subnet_mask), entr->metric);
	  printf("found better route to %s %s Metric %u through %s\n", intToIp(entr->network_addr), intToIp(entr->subnet_mask), entr->metric, intToIp(e->next_hop));
	  delete_elem(entr);
	  add_elem(e);
	  sem_post(&mutex); 
	}
      }
    }
  }
}

void sendMessage(void* message, int size){
  struct sockaddr_in addr;
  int fd, cnt;
  struct ip_mreq mreq;
  
  /* create UDP socket */
  if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    exit(1);
  }
  
  /*set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr(DST_IP);
  addr.sin_port=htons(PORT);
  
  while (1) {
    if (sendto(fd,message,size,0,(struct sockaddr *) &addr,
	       sizeof(addr)) < 0) {
      perror("sendto");
      exit(1);
    }
    printf("sending RIP request to %s\n", intToIp(addr.sin_addr.s_addr));
    sleep(30);
  }
}

int main(int argc, char *argv[])
{
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];
  char mask[NI_MAXHOST];
  sem_init(&mutex, 0, 1);
  
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }
  header response_header;
  header request_header;
  response_header.command = 2;
  response_header.version = 2;
  response_header.unused = 0;
  request_header.command = 1;
  request_header.version = 2;
  request_header.unused = 0;
  
  /* Walk through linked list, maintaining head pointer so we
     can free list later */
  int map_len = 0;
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;
    
    family = ifa->ifa_addr->sa_family;
    
    /* Display interface name and family (including symbolic
       form of the latter for the common families) */
    
    printf("%s  address family: %d%s\n",
	   ifa->ifa_name, family,
	   (family == AF_PACKET) ? " (AF_PACKET)" :
	   (family == AF_INET) ?   " (AF_INET)" :
	   (family == AF_INET6) ?  " (AF_INET6)" : "");
    
    /* For an AF_INET* interface address, display the address */
    
    if (family == AF_INET) {
      
      s = getnameinfo(ifa->ifa_addr,
		      (family == AF_INET) ? sizeof(struct sockaddr_in) :
		      sizeof(struct sockaddr_in6),
		      host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      
      n = getnameinfo(ifa->ifa_netmask,
		      (family == AF_INET) ? sizeof(struct sockaddr_in) :
		      sizeof(struct sockaddr_in6),
		      mask, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
	printf("getnameinfo() failed: %s\n", gai_strerror(s));
	exit(EXIT_FAILURE);
      }
      printf("\taddress: <%s>\n", host);
      entry *e = malloc(sizeof(entry));
      struct in_addr ip_addr;
      inet_aton(host, &ip_addr);
      uint32_t host_ip = htonl(ip_addr.s_addr);
      inet_aton(mask, &ip_addr);
      uint32_t mask_addr = htonl(ip_addr.s_addr);
      e->subnet_mask = mask_addr;
      uint32_t network = mask_addr&host_ip;
      
      if(network == 2130706432) continue;
      
      printf("\t%u\n", network);
      e->network_addr = network;
      printf("\t%s\n", intToIp(host_ip));
      printf("\t%s\n", intToIp(mask_addr));
      printf("\t%s\n", intToIp(network));
      e->next_hop = 0;
      e->metric = 0;
      e->family = 2;
      e->tag = 0;
      sem_wait(&mutex);
      add_elem(e);
      sem_post(&mutex); 
      map_len++;
    }
  }
  
  pthread_t thread;
  int iret = pthread_create (&thread, NULL, (void *) &recieveUpdate, NULL);
  if(iret)
    {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
      exit(EXIT_FAILURE);
    }
  
  //Sending message
  struct entry * cur;
  int i = 0;
  void* message = malloc(sizeof(header));
  int size = 0;
  int sizeOfHeader = sizeof(header);
  int sizeOfEntry = sizeof(entry) - sizeof(UT_hash_handle);
  for(cur = map; cur != NULL; cur = cur->hh.next) {
    if(i % MAX_ROUTE_ENTRY == 0){
      if(i != 0) sendMessage(message, size+sizeOfEntry);
      free(message);
      size = sizeOfHeader;
      message = malloc(sizeOfHeader);
      memcpy(message, &request_header, sizeOfHeader);
    }
    size = size + (i % MAX_ROUTE_ENTRY)*sizeOfEntry;
    message = realloc(message, size+sizeOfEntry);
    cur->metric = cur->metric + 1;
    memcpy((char*)message + size, cur, sizeOfEntry);
    i++;
    if(i == map_len){
      sendMessage(message,size+sizeOfEntry);
      break;
    }
  }
  pthread_join(thread, NULL);
  sem_destroy(&mutex);
  freeifaddrs(ifaddr);
  exit(EXIT_SUCCESS);
}

