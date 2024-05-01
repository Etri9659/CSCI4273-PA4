/*
-Read config files to get server address and port numbers 
-Implement functions for list, get and put 

Put function:
-Split file into 4 equalish chunks(P1, P2, P3, P4)
-Group chunks into pairs using md5 hash
    -Use table in write up

List function:
- Lists availble file names on DFS servers 
- Checks if file chunks on avalible DFS servers are enough to reconstruct 
    - If not incomplete will be added to end of file name 

Get function:
- Downloads all avalible chunks from all avalible DFS servers 
- If possible reconstruct file 
- If not pring <filename> is incomplete 

-Communicate with DFS server
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

#define BUFSIZE 8192
#define MAX_FILENAME_LENGTH 256
#define MAX_SERVERS 4

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    char ip[16];
    int port;
} Server;

Server servers[MAX_SERVERS];
int numServer = 0;
char runningServers[4][MAX_FILENAME_LENGTH];
char runningSockfd[4];

void readSConfig();
void connectServers();
void listFunc();
void getFunc(char *fName);
void putFunc(char *fName);
void sendChunk(char* chunk, char* fname, char chunkChar, int dfsIdx, size_t len);
int sendtoDFS(char* msg, size_t len);

int main(int argc, char *argv[]) {
    if(argc < 2){
        printf("Usage: %s <command> [filename] ... [filename]\n", argv[0]);
        return 1;
    }

    readSConfig();
    connectServers();

    while(1){
        if (strcmp(argv[1], "list") == 0) {
            listFunc();
        } else if (strcmp(argv[1], "get") == 0) {
            if (argc < 3) {
                printf("Usage: %s get [filename]\n", argv[0]);
                return 1;
            }
            getFunc(argv[2]);
        } else if (strcmp(argv[1], "put") == 0) {
            if (argc < 3) {
                printf("Usage: %s put [filename]\n", argv[0]);
                return 1;
            }
            putFunc(argv[2]);
        } else {
            printf("Invalid command.\n");
            return 1;
        }
    }

    

    return 0;
}

void readSConfig(){
    FILE *f = fopen("dfc.conf", "r");
    if (f == NULL){
        perror("ERROR opening config file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_FILENAME_LENGTH];
    while(fgets(line, sizeof(line), f)){
        if(strncmp(line, "server", 6) == 0) {
            char serverName[MAX_FILENAME_LENGTH];
            char ipPort[MAX_FILENAME_LENGTH];
            if(sscanf(line, "server %s %s", serverName, ipPort) == 2){
                char *ipToken = strtok(ipPort, ":");
                char ip [MAX_FILENAME_LENGTH];
                strcpy(ip, ipToken);
                ipToken = strtok(NULL, ":");
                int port = atoi(ipToken);

                Server server;
                strcpy(server.name, serverName);
                strcpy(server.ip, ip);
                server.port = port;

                servers[numServer] = server;
                numServer++;
            } else{
                printf("ERROR parsing config file %s", line);
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(f);
}
void connectServers(){
    struct sockaddr_in serveraddr; /*server's addr*/
    int sockfd; /*socket*/
    int optval = 1; /*flag value for setsockopt*/
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    
    for(int i=0; i < numServer; i++){
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            return;

        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(servers[i].ip);
        serveraddr.sin_port = htons(servers[i].port);

        if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout , sizeof(timeout))){
            perror("Set sockopt fialed");
            close(sockfd);
            continue;
        }
        
        if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                printf("Server %s:%d is not available\n", servers[i].ip, servers[i].port);
            } else {
                perror("Connection failed");
            }
            close(sockfd);
            continue;
        }

        printf("Connected to server %s:%d\n", servers[i].ip, servers[i].port);
        strcpy(runningServers[i], servers[i].name);
        runningSockfd[i] = sockfd;
    }
    return;
}

void listFunc(){
    sendtoDFS("list", BUFSIZE);
}
void getFunc(char *fName){

}
void putFunc(char *fName){
    size_t fsize;

    FILE *f = fopen(fName, "rb");
    if(f == NULL){
        perror("ERROR requested put file does not exist");
        fclose(f);
        return;
    }

    /*Calculate file size*/
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    rewind(f);

    /*Split files into 4 chunks*/
    char p1[fsize/4];
    char p2[fsize/4];
    char p3[fsize/4];
    char p4[fsize - 3*(fsize/4)];

    /*Read files into the 4 chunks*/
    fread(p1, sizeof(p1), 1, f);
    fread(p2, sizeof(p2), 1, f);
    fread(p3, sizeof(p3), 1, f);
    fread(p4, sizeof(p4), 1, f);

    /*Implement send chunk and hashing*/
}

void sendChunk(char* chunk, char* fname, char chunkChar, int dfsIdx, size_t len){
    char fsize[BUFSIZE];
    sprintf(fsize, "%d", (int)len);

    char msg[strlen(fname) + strlen("put ") + strlen(fsize) + 5];

    strcpy(msg, "put");
    strcat(msg, fsize);
    strcat(msg, " ");
    strcat(msg, fname);
    strncat(msg, &chunkChar, 1);

    sendtoDFS(msg, strlen(msg) + 1);
}

int sendtoDFS(char* msg, size_t len){
    int n;

    for(int i = 0; i < numServer; i++){
        n = write(runningSockfd[i], msg, len);
        if(n < 0){
            printf("Error in sendto: errno %d\n", errno);
        }
    }
    return n;
}

