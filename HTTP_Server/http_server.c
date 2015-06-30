#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h> 
#include <sys/epoll.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define PORT "80"
#define MAX_EVENTS 10

static int create(){
	struct addrinfo addrs;
  struct addrinfo *res, *temp;
  int sock_fd, s;

  memset (&addrs, 0, sizeof (struct addrinfo));
  addrs.ai_family = AF_UNSPEC;     
  addrs.ai_socktype = SOCK_STREAM; 
  addrs.ai_flags = AI_PASSIVE;    

  s = getaddrinfo (NULL, PORT, &addrs, &res);
  if (s != 0)
      return -1;

  for (temp = res; temp != NULL; temp = temp->ai_next)
    {
      sock_fd = socket (temp->ai_family, temp->ai_socktype, temp->ai_protocol);
      if (sock_fd == -1)
        continue;

      s = bind (sock_fd, temp->ai_addr, temp->ai_addrlen);
      if (s == 0)
        break;

      close (sock_fd);
    }

  if (temp == NULL)
      return -1;

  freeaddrinfo (res);
  return sock_fd;
}

static int setnonblocking (int sock_fd)
{
  int flags, s;
  
  flags = fcntl (sock_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  
  flags |= O_NONBLOCK;
  s = fcntl (sock_fd, F_SETFL, flags);
  if (s == -1)
    return -1;
  
  return 0;
}
void get_path(char* temp,char* path_name){
  char *tok = NULL;
  tok = strtok(temp, " ");
  tok = strtok(NULL, " ");
  
  strcpy(path_name, tok);
}

void get_host(char* temp,char* host_name){
  char *tok = NULL;
  tok = strtok(temp, " ");
  tok = strtok(NULL, " ");
  
  strcpy(host_name, tok);
}

void get_user_agent(char* temp,char* user_agent){
  
  strcpy(user_agent, (char*)temp+12);
}

int main(int argc, char *argv[])
{
  int args = argc - 1;
  
  if(args % 2 != 0)
    exit(1);
  
  int sock_fd, s, epoll_fd;
  struct epoll_event event;
  struct epoll_event *events;
  
  sock_fd = create();
  if(sock_fd == -1)
    exit(1);
  
  s = setnonblocking(sock_fd);	
  if(s == -1)
    exit(1);
  
  s = listen(sock_fd, SOMAXCONN);
  if(s == -1){
    perror("Listen error");
    exit(1);
  }
  
  epoll_fd = epoll_create1(0);
  if(epoll_fd == -1){
    perror("Epoll create error");
    exit(1);
  }
  
  event.data.fd = sock_fd;
  event.events = EPOLLIN|EPOLLET;
  
  s = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);
  if(s == -1){
    perror("Epoll control error");
    exit(1);
  }
  
  events = calloc (MAX_EVENTS, sizeof(event));
  
  for(;;){
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1){
      perror("Epoll wait error");
      exit(1);
    }
    int i;
    for(i = 0; i < nfds; i++){
      if ((events[i].events & EPOLLERR) ||
	  (events[i].events & EPOLLHUP) ||
	  (!(events[i].events & EPOLLIN))){
	close (events[i].data.fd);
	continue;
      }else if (sock_fd == events[i].data.fd){
	for(;;){
	  struct sockaddr addr;
	  int fd;
	  char host[NI_MAXHOST], port[NI_MAXSERV];
	  
	  int addr_size = sizeof(addr);
	  fd = accept (sock_fd, &addr, &addr_size);
	  if (fd == -1){
	    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
	      break;
	    
	    else
	      {
		perror ("Accept error");
		break;
	      }
	  }
	  
	  s = getnameinfo (&addr, addr_size,
			   host, sizeof(host),
			   port, sizeof(port),
			   NI_NUMERICHOST | NI_NUMERICSERV);
	  if (s == 0){
	    printf("descriptor %d host=%s, port=%s\n", fd, host, port);
	    
	  }
	  
	  s = setnonblocking(fd);
	  if (s == -1)
	    abort ();
	  
	  event.data.fd = fd;
	  event.events = EPOLLIN | EPOLLET;
	  s = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, fd, &event);
	  if (s == -1){
	    perror ("Epoll control error");
	    abort ();
	  }
	}
	continue;
      }else{
	int done = 0;
	int fd;
	//char filename[PATH_MAX];
	for(;;){
	  ssize_t count;
	  char buf[512];
	  
	  count = read (events[i].data.fd, buf, sizeof(buf));
	  if (count == -1){
	    if (errno != EAGAIN){
	      perror ("read");
	      done = 1;
	    }
	    break;
	  }
	  else if (count == 0){
	    done = 1;
	    break;
	  }
	  
	  //printf("%s\n",buf);
	  char *token = NULL;
	  token = strtok(buf, "\n");
	  int l = 0;
	  char* path_name;
	  char* host_name;
	  char* user_agent;
	  char* temp_path = malloc(256);
	  char* temp_host = malloc(256);
	  char* temp_user_agent = malloc(256);
	  while (token) {
	    l = l + 1;
	    if(l == 1){
	      memset(temp_path, '\0', 256);
	      strcpy(temp_path, token);
	    }
	    if(l == 2){
	      memset(temp_host, '\0', 256);
	      strcpy(temp_host, token);
	    }
	    if(strstr(token,"User-Agent") != NULL){
	      memset(temp_user_agent, '\0', 256);
	      strcpy(temp_user_agent, token);
	      break;
	    }
	    //puts(token);
	    token = strtok(NULL, "\n");
	  }
	  path_name = malloc(256);
	  memset(path_name, '\n', 256);
	  get_path(temp_path, path_name);
	  printf("path: %s\n",path_name);
	  host_name = malloc(256);
	  memset(host_name, '\n', 256);
	  get_host(temp_host, host_name);
	  printf("host: %s\n",host_name);
	  user_agent = malloc(256);
	  memset(user_agent, '\n', 256);
	  get_user_agent(temp_user_agent, user_agent);
	  printf("user agent: %s\n",user_agent);
	  
	  int j;
	  
	  //s = write (1, buf, count);
	  int error = -1;
	  char* message;
	  char* p;
	  char* h;
	  for (j = 1; j <= args; j+=2){
	    h = malloc(strlen(argv[j])+1);
	    p = malloc(strlen(argv[j+1])+1);
	    strcpy(h, argv[j]);
	    strcpy(p, argv[j+1]);
	    if(strstr(host_name,h) != 0){
	      error = 0;
	      message = strdup("HTTP/1.1 200 OK");
	      break;
	    }
	  }
	  
	  
	  if(error == 0){
	    printf("message header is: %s\n",message);
	    write(events[i].data.fd, message, strlen(message)+1);
	    message = strdup("Content-Type: text/html");
	    write(events[i].data.fd, message, strlen(message)+1);
	    char *filename;
	    filename = strcat(p,path_name);
	    printf("file name is: %s\n",filename);
	    fd = open(filename, O_RDONLY);
	    if (fd == -1) {  
	      message = strdup("HTTP/1.1 404 REQUESTED DOMAIN NOT FOUND");
	      write(events[i].data.fd, message, strlen(message)+1);
	      
	    }else{
	      struct stat stat_buf;  
	      fstat(fd, &stat_buf);
	      int offset = 0;
	      int rc = sendfile (events[i].data.fd, fd, &offset, stat_buf.st_size);
	      if (rc == -1) {
		fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
		exit(1);
	      }
	    }
	  }else{
	    message = strdup("HTTP/1.1 404 REQUESTED DOMAIN NOT FOUND");
	    write(events[i].data.fd, message, strlen(message)+1);
	  }
	  if (s == -1){
	    perror ("Write error");
	    abort ();
	  }
	}
	
	if (done)
	  close (events[i].data.fd);
      }	
    }  
  }
  free (events);
  close (sock_fd); 
}

