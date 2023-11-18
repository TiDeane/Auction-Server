#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define PORT "58011"

int main(int argc, char **argv) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer[128];


    const char* ASIP = argv[1];
    char* ASport;
    if (argc == 3)
        ASport = argv[2];
    else
        ASport = (char*) PORT;
    
    printf("ASIP = %s, ASport = %s\n", ASIP, ASport);

    fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fd==-1) /*error*/ exit(1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP socket

    errcode=getaddrinfo(ASIP,ASport,&hints,&res);
    if(errcode!=0) /*error*/ exit(1);

    while(1) {
        fgets(buffer, 128, stdin);
        char *token = strtok(buffer, " ");
        if (token != NULL && strcmp(token, "login") == 0) {
            token = strtok(NULL, " ");
            char UID[7];
            strcpy(UID, token);
            char *pw;
            /*token = strtok(NULL, " ");
            strcpy(pw, token);*/
            printf("UID = %s, password = %s\n", UID, pw);
        }
    }

    freeaddrinfo(res);
    close(fd);
}

/*
n=sendto(fd,"Hello!\n",7,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) error exit(1);


    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) error exit(1);

    write(1,"echo: ",6); write(1,buffer,n); 
*/
