#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

#define LISTENQ 10
#define MAXLINE 1000

int open_listenfd(int port)  
{ 
  int listenfd, optval=1; 
  struct sockaddr_in serveraddr; 
   
  /* Create a socket descriptor */ 
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    return -1; 
  
  /* Eliminates "Address already in use" error from bind. */ 
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,  
		 (const void *)&optval , sizeof(int)) < 0) 
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


void echo(int connfd)  
{ 
  //initialization part.
  size_t n;  
  char buf[MAXLINE];
  char *shift_char;
  char *path;
  FILE * fp;

  //for encryption
  int numShift;
  int numEncrypt;
  
  //results which could be printed
  char ok[] = "HTTP/1.0 200 OK\r\n\r\n";
  char notFound[] = "HTTP/1.0 404 Not Found\r\n\r\n";
  char forbidden[] = "HTTP/1.0 403 Forbidden\r\n\r\n";
    
  read(connfd, buf, MAXLINE);

  path = strtok(buf, " ");
  path = strtok(NULL, " ");
  shift_char = strtok(NULL, " ");
  numShift = atoi(shift_char);
  numShift = numShift % 26;
  
  if(access(path, F_OK)){
    //if it is not OK, print 404
    write(connfd, notFound, strlen(notFound));
  }
  else if(access(path, R_OK)){
    //if it is forbidden, print 403
    write(connfd, forbidden, strlen(forbidden));
  }
  else{
    //else print OK and process it.
    write(connfd, ok, strlen(ok));
      //Open the file.
      fp = fopen(path, "r");
      int index = 0;
      while(!feof(fp)){
        bzero(buf, MAXLINE);
	      fread(buf, sizeof(char), MAXLINE-1, fp);
	      while(buf[index] != '\0' &&  index < strlen(buf)){
          // Do the encryption.
	        if(isalpha(buf[index])){
	          if(((int)buf[index]) > 96)
	            numEncrypt = (((int)buf[index] - numShift - 71) % 26 + 97);
	          else
	            numEncrypt = (((int)buf[index] - numShift - 39) % 26 + 65);
	          buf[index] = (char)(numEncrypt);
	        }
	        index++;
	      }
	      index = 0;
        //write the file after encryption.
	      write(connfd, buf, strlen(buf));
      }
      fclose(fp);
  }
} 


int main(int argc, char **argv) {
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  port = atoi(argv[1]); /* the server listens on a port passed 
			   on the command line */
  listenfd = open_listenfd(port); 

  while (1) {
    clientlen = sizeof(clientaddr); 
    connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

    //if fork not return a 0, wait for next connection.
    if(!fork()){
      close(listenfd);
      // read it just like part 2 did.
      echo(connfd);
      close(connfd);
      exit(0);
    }

    close(connfd);
  }
}

