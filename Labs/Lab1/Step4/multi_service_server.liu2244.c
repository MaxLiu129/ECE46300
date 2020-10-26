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
#include <fcntl.h>
#include <sys/select.h>
#include <math.h>

#define LISTENQ 100
#define MAXLINE 1024

int open_listenfd_http(int port)  
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

int open_listenfd_ping(int port)  
{ 
	int listenfd, optval=1; 
	struct sockaddr_in serveraddr; 

	/* Create a socket descriptor */ 
	if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
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
	//initialize variables.
	int listenfd, listenfd_udp, connfd, port, ping, clientlen, childpid_http;
	struct sockaddr_in clientaddr, udpaddr;

	struct hostent *hp;
	struct hostent *udphost;
	char *haddrp;

	//initianize variables for udp
	fd_set rfds;
	struct sockaddr_in serveraddr;
	int fdmax;
	int len;
	int retval;
	uint32_t  seq_num;
	char data[MAXLINE];
	char buf[MAXLINE];
	struct in_addr ipv4addr;
	port = atoi(argv[1]); /* the server listens on a port passed 
							 on the command line */
	ping = atoi(argv[2]);
	listenfd = open_listenfd_http(port); 
	listenfd_udp = open_listenfd_ping(ping);

	while(1) {
		FD_ZERO(&rfds);
		FD_SET(listenfd, &rfds);
		FD_SET(listenfd_udp, &rfds);
		fdmax = listenfd > listenfd_udp ? listenfd+1 : listenfd_udp+1;
		retval = select(fdmax, &rfds, NULL, NULL, NULL);
		//TCP
		if(FD_ISSET(listenfd, &rfds)){
			clientlen = sizeof(clientaddr); 
			connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
			hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
			haddrp = inet_ntoa(clientaddr.sin_addr);
			if (!(childpid_http = fork())){
				echo(connfd);
				close(listenfd);
				exit(0);
			}
			close(connfd);
		}
		//UDP
		else if(FD_ISSET(listenfd_udp, &rfds)){

			len = sizeof(clientaddr);
			int num_byte = recvfrom(listenfd_udp, buf, MAXLINE, 0, (struct sockaddr *)&udpaddr, &len);
			
			memcpy(data, buf,num_byte - 4);
			data[num_byte - 4] = '\0';
			memcpy(&seq_num,buf + num_byte - 4, 4);
			seq_num = htonl(ntohl(seq_num) + 1);

			inet_pton(AF_INET, data, &ipv4addr);
			udphost = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
			//copy them.
			bzero(buf, MAXLINE);
			memcpy(buf, udphost->h_name, strlen(udphost->h_name));
			memcpy(buf + strlen(udphost->h_name), &seq_num, 4); 
			//send it
			sendto(listenfd_udp, buf, strlen(udphost->h_name) + 4, 0, (struct sockaddr *)&udpaddr, len);
	    }
	}
}