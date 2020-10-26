#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <unistd.h>


#define MAXLINE 200


int open_clientfd(char *hostname, int port) 
{ 
	int clientfd; 
	struct hostent *hp; 
	struct sockaddr_in serveraddr; 

	if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return -1; /* check errno for cause of error */ 

	/* Fill in the server's IP address and port */ 
	if ((hp = gethostbyname(hostname)) == NULL) 
		return -2; /* check h_errno for cause of error */ 
	bzero((char *) &serveraddr, sizeof(serveraddr)); 
	serveraddr.sin_family = AF_INET; 
	bcopy((char *)hp->h_addr,  
			(char *)&serveraddr.sin_addr.s_addr, hp->h_length); 
	serveraddr.sin_port = htons(port); 

	/* Establish a connection with the server */ 
	if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		return -1; 
	return clientfd; 
} 

// -------------------TODO---------------------//
// 1. Establishes a connection to the correct server port 
// 2. Sends a “GET” command to the server. 
// 3. Prints the server response. 
// 4. If the response code is not 200 (HTTP OK), terminate the client program. Otherwise, continue to step 5. 
// 5. Sends a second “GET” request to the server using the extracted content as the new file name 
// 6. Prints the complete server response (response header + content) 
// -------------------TODO---------------------//

int main(int argc, char**argv){
    int clientfd, port;
    char* host, buf[MAXLINE], prebuf[MAXLINE];
    char http[] = "HTTP/1.1";
	char code[] = "200";
    FILE* fp;

    host = argv[1];
    port = atoi(argv[2]);

    clientfd = open_clientfd(host, port);
    sprintf(buf, "GET %s HTTP/1.0\r\n\r\n", argv[3]);

    // print error if connection failed.
    if(clientfd < 0) {
        printf("Something wrong when opening connection!\n");
        exit(0);
    }
    
    fp = fopen("output1.txt", "w");
    write(clientfd, buf, strlen(buf));
    read(clientfd, buf, (MAXLINE-1));

    while(strlen(buf) >= 1)
	{
		fprintf(fp, "%s", buf);
		bzero(buf, MAXLINE);
		read(clientfd, buf, (MAXLINE-1));
		fputs(buf, stdout); 
	}
	fclose(fp);
    //clear the bufs.
	bzero(buf, MAXLINE);
	bzero(prebuf, MAXLINE);
	fp = fopen("output1.txt","r");
	while(1)
	{
		fscanf(fp, "%s", buf);
		if(!strcmp(prebuf, http))
			if(strcmp(buf, code))
				exit(0);
		if(feof(fp)){break;}
		strcpy(prebuf, buf);
		bzero(buf, MAXLINE);
	}
	fclose(fp);
    
	clientfd = open_clientfd(host, port);
    sprintf(buf, "GET %s HTTP/1.0\r\n\r\n", prebuf);

    // print error if connection failed.
	if(clientfd < 0) {
        printf("Something wrong when opening connection2!\n");
        exit(0);
    }

	fp = fopen("output2.txt","w");

	write(clientfd, buf, strlen(buf)); 
	read(clientfd, buf, (MAXLINE-1));

	while(strlen(buf) >= 1){
        fprintf(fp, "%s", buf);
		bzero(buf, MAXLINE);
		read(clientfd, buf, (MAXLINE-1));
		fputs(buf, stdout);
    }
    printf("\n");

    fclose(fp);
    close(clientfd);
    exit(0);
}
