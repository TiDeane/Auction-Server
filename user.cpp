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

int fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

int main(int argc, char **argv) {

    bool logged_in = false;
    char UID_current[7];
    char pw[9];

    /* Default IP and PORT values */
    char* ASIP = (char*) "127.0.0.1";
    char* ASport = (char*) PORT;

    if (argc >= 2) /* At least one argument */
        for (int i = 1; i < argc; i += 2) {
            if (strcmp(argv[i], "-n") == 0)
                ASIP = argv[i+1]; // The IP follows after "-n"
            else if (strcmp(argv[i], "-p") == 0)
                ASport = argv[i+1]; // The Port follows after "-p"
        }
    
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
        char *token = strtok(buffer, " \n"); // gets the first word

        if (token != NULL && strcmp(token, "login") == 0) { // Make it into function "parse_login()"?

            printf("entered login command\n");

            char UID[7];
            token = strtok(NULL, " ");
            if (token != NULL)
                strcpy(UID, token);
            token = strtok(NULL, " ");
            if (token != NULL)
                strcpy(pw, token); // Check if password format is correct?

            // sends to AS using UDP
            // AS logs in if UID and PW are correct, or registers user if UID not present
            // prints result of operation
        }
        else if (token != NULL && strcmp(token, "logout") == 0) {
            // sends to AS using UDP
            // Logs out currently logged in user
            // prints result of operation

            printf("entered logout command\n");
        }
        else if (token != NULL && strcmp(token, "unregister") == 0) {
            // sends to AS using UDP
            // asks to unregister currently logged in user. Should also be logged out
            // prints result of operation

            printf("entered unregister command\n");
        }
        else if (token != NULL && strcmp(token, "exit") == 0) {
            // if user is logged in, asks to log out
            // if logged out, terminate the application
            // doesn't communicate with AS

            printf("entered exit command\n");

            if (logged_in == true)
                printf("You must log out before exiting!\n");
            else
                break;
            
        }
        else if (token != NULL && strcmp(token, "open") == 0) {
            // establishes TCP connection
            // sends message asking to open new auction (with auction's info)
            // AS returns whether request was successful and the auction's AID
            // closes connection
            
            printf("entered open command\n");
        }
        else if (token != NULL && strcmp(token, "close") == 0) {
            // sends to AS using TCP
            // asks to close auction with given AID that had been started by logged in user
            // AS returns result of operation
            // closes connection
        }
        else if (token != NULL && (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0)) {
            printf("entered ma command\n");
        }
        else if (token != NULL && (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0)) {
            printf("entered mb command\n");
        }
        else if (token != NULL && (strcmp(token, "list") == 0 || strcmp(token, "l") == 0)) {
            printf("entered l command\n");
        }
        else if (token != NULL && (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0)) {
            printf("entered sa command\n");
        }
        else if (token != NULL && (strcmp(token, "bid") == 0 || strcmp(token, "b") == 0)) {
            printf("entered b command\n");
        }
        else if (token != NULL && (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0)) {
            printf("entered sr command\n");
        } else
            printf("entered invalid command\n");
    }

    freeaddrinfo(res);
    close(fd);

    return 0;
}

/*
UDP Protocol:

n=sendto(fd,"Hello!\n",7,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) error exit(1);


    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) error exit(1);

    write(1,"echo: ",6); write(1,buffer,n); 
*/
