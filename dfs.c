/*
-File storage and retrieval 
-Server sockets that handle multiple connections 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>

#define BUFSIZE 8192
#define BUF 256
#define LISTENQ 1024

int startListen(int port);

int main(int argc, char *argv[]){
    int port; /* port to listen on */
    struct sockaddr_in clientaddr; /* client addr */
    int clientlen = sizeof(struct sockaddr_in); /* byte size of client's address */
    int sockfd; /* socket */
    int *clientSock; /*client request*/

    if(argc != 2){
        printf("Invalid number of arguments - usage: ./dfs </DFS{1-4}> <port #>\n;");
        return 1;
    }

    sockfd = startListen(port);

    port = atoi(argv[1]);

    sockfd = startListen(port);
    while (1) {
        /*Accept connection*/
	    clientSock = malloc(sizeof(int));
	    *clientSock = accept(sockfd, (struct sockaddr*)&clientaddr, &clientlen);
        if (clientSock < 0){
            perror("Accept failure");
            exit(EXIT_FAILURE);
        }
    }
}

int startListen(int port){
    int sockfd; /*socket*/
    struct sockaddr_in serveraddr; /*server's addr*/
    int optval = 1; /*flag value for setsockopt*/
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    if (listen(sockfd, LISTENQ) < 0)
        return -1;
    
    return sockfd;
} 