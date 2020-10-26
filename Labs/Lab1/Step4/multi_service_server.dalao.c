#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <math.h>

#define LISTENQ 100
#define MAXLINE 1024
#define PINGSIZE 68
int open_listenfd_http(int);
int open_listenfd_ping(int);
void echo_http(int);
//void echo_ping(int);

int main(int argc, char **argv)
{
  int listenfd_http, listenfd_ping;
  int connfd_http;
  int connfd_ping;
  int http_port;
  int ping_port;
  int clientlen_http;
  int clientlen_ping;
  struct sockaddr_in clientaddr_http;
  struct sockaddr_in clientaddr_ping;
  fd_set rfds;
  int retval;
  int maxfd;
  int childpid_http = 0;
  int childpid_ping = 0;
  uint32_t num;
  char buf[PINGSIZE];
  socklen_t len;
  struct hostent *hp;
  char *haddrp;

  //判断指令是否正确
  if (argc != 3)
    {
      printf("usage: ./multi_serivce_server <http service port> <ping service port>\n");
      return 1;
    }

  http_port = atoi(argv[1]); /* the server listens on a port passed
				on the command line */
  ping_port = atoi(argv[2]);
  listenfd_http = open_listenfd_http(http_port);
  listenfd_ping = open_listenfd_ping(ping_port);

  while (1) 
    {
      FD_ZERO(&rfds);
      FD_SET(listenfd_http, &rfds);
      FD_SET(listenfd_ping, &rfds);
      maxfd = (listenfd_http > listenfd_ping) ? listenfd_http+1 : listenfd_ping+1;
      retval = select(maxfd, &rfds, NULL, NULL, NULL);
     
     //it is a TCP
      if (FD_ISSET(listenfd_http, &rfds))
	{	  
	  clientlen_http = sizeof(clientaddr_http);
	  connfd_http = accept(listenfd_http, (struct sockaddr *)&clientaddr_http, &clientlen_http);
	  if ((childpid_http = fork()) == 0) {
	    close(listenfd_http);
	    echo_http(connfd_http);
	    exit(0);
	  }
	  close(connfd_http);
	}
      //it is a UDP
      else if (FD_ISSET(listenfd_ping, &rfds))
	{
	  int n;
	  //echo
	  n = recvfrom(listenfd_ping, buf, PINGSIZE, 0, (struct sockaddr *)&clientaddr_ping, &len);
	  num = ntohl(*((uint32_t *)buf));
	  num = num + 1;
	  *((uint32_t *)buf) = htonl(num);
	  sendto(listenfd_ping, buf, n, 0, (struct sockaddr *)&clientaddr_ping, len);
	}
    }
  return 0;
}


int open_listenfd_http(int port)
{
  int listenfd = -1;
  int optval = 1;
  struct sockaddr_in serveraddr;
  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;
  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
    return -1; 
  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  /* Make it a listening socket ready to accept
     connection requests */
  if (listen(listenfd, LISTENQ) < 0)
    return -1;
  return listenfd; 
}

int open_listenfd_ping(int port)
{
  int listenfd = -1;
  int optval = 1;
  struct sockaddr_in serveraddr;

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;
  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
    return -1; 
  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;
  return listenfd;
}

void echo_http(int connfd)
{
  int i;
  int k;
  int j;
  char buf[MAXLINE];
  int rd;
  char * path;
  int filestatus;
  FILE * fd;
  // char * welcome = "Connection Established\n";
  // read in client request
  read(connfd, buf, MAXLINE);
  // write(connfd, welcome, strlen(welcome));
  path = strtok(buf, " ");
  path = strtok(NULL, " "); //this is the real path string
    //check file
  if (access(path, R_OK) == 0)
    {
      write(connfd, "HTTP/1.0 200 OK\r\n\r\n", strlen("HTTP/1.0 200 OK\r\n\r\n"));
      fd = fopen(path, "r");
      while (!feof(fd))
	{
	  memset(buf,0,MAXLINE);
	  fread(buf, sizeof(char), MAXLINE-1, fd);
	  write(connfd, buf, strlen(buf));
	}
      fclose(fd);
    }
  else
    {
      if (access(path, F_OK) != 0)
	{
	  write(connfd, "HTTP/1.0 404 Not Found\r\n\r\n", strlen("HTTP/1.0 403 Not Found\r\n\r\n"));
	}
      else
	{
	  write(connfd, "HTTP/1.0 403 Forbidden\r\n\r\n", strlen("HTTP/1.0 403 Forbidden\r\n\r\n"));
	}
    } 
  
} 


/* void echo_ping (int connfd) */
/* { */
/*   uint32_t num; */
/*   char buf[PINGSIZE]; */
/*   struct sockaddr_in clientaddr; */
/*   socklen_t len; */
/*   recvfrom(connfd, buf, PINGSIZE, 0, (struct sockaddr *)&clientaddr, &len); */
/*   num = ntohl(*((uint32_t *)buf)); */
/*   num = num + 1; */
/*   *((uint32_t *)buf) = htonl(num); */
/*   sendto(connfd, buf, PINGSIZE, 0, (struct sockaddr *)&clientaddr, len); */
    
/* } */
