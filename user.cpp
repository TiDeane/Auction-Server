#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>

#define PORT "58011"

int UDP_fd,TCP_fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
char buffer[128];

char UID_current[7];
char password_current[9];
bool logged_in = false;

bool check_UID_format(char* UID) {
    if (UID == NULL || strlen(UID) != 6) {
        printf("ERR: UID must be a 6-digit number\n");
        return false;
    }
    
    for (int i= 0; i < 6; i++)
        if (!isdigit(UID[i])) {
            printf("ERR: UID must be a 6-digit number\n");
            return false;
        }
    
    return true;
}

bool check_password_format(char* password) {
    if (password == NULL || strlen(password) != 8) {
        printf("ERR: Password must be composed of 8 alphanumeric characters\n");
        return false;
    }
    
    for (int i= 0; i < 8; i++)
        if (!isalnum(password[i])) {
            printf("ERR: Password must be composed of 8 alphanumeric characters\n");
            return false;
        }
    
    return true;
}

void login_command(char* token) {

    if (logged_in) {
        printf("ERR: must logout first\n");
        return;
    }

    char UID[7];
    token = strtok(NULL, " \n");

    if (check_UID_format(token))
        strcpy(UID, token);

    char password[9];
    token = strtok(NULL, " \n");

    if (check_password_format(token))
        strcpy(password, token);

    char AS_command[20] = "LIN ";
    strcat(AS_command, UID);
    strcat(AS_command, " ");
    strcat(AS_command, password);
    strcat(AS_command, "\n");

    n=sendto(UDP_fd,AS_command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (strncmp(buffer, "RLI REG\n", 8) == 0) {
        printf("new user registered\n");

        logged_in = true;
        strcpy(UID_current, UID);
        strcpy(password_current, password);
        return;
    }
    else if (strncmp(buffer, "RLI OK\n", 7) == 0) {
        printf("successful login\n");

        logged_in = true;
        strcpy(UID_current, UID);
        strcpy(password_current, password);
        return;
    }
    else if (strncmp(buffer, "RLI NOK\n", 8) == 0) {
        printf("incorrect login attempt\n");
        return;
    }
}

void logout_command(char* token) {
    if (!logged_in) {
        printf("ERR: must login first!\n");
        return;
    }

    char AS_command[20] = "LOU ";

    strcat(AS_command, UID_current);
    strcat(AS_command, " ");
    strcat(AS_command, password_current);
    strcat(AS_command, "\n");

    n=sendto(UDP_fd,AS_command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (strncmp(buffer, "RLO OK\n", 7) == 0) {
        printf("successful logout\n");
        logged_in = false;
        return;
    }
    else if (strncmp(buffer, "RLO NOK\n", 8) == 0) {
        printf("user not logged in\n");
        return;
    }
    else if (strncmp(buffer, "RLO UNR\n", 8) == 0) {
        printf("unknown user\n");
        return;
    }

}

void unregister_command(char *token){

    if(!logged_in){
        printf("ERR: must login first\n");
        return;
    }

    char UNR_Command[20]="UNR";
    strcat(UNR_Command, UID_current);
    strcat(UNR_Command, " ");
    strcat(UNR_Command, password_current);
    strcat(UNR_Command, "\n");

    n=sendto(UDP_fd,UNR_Command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if(strcmp(buffer,"RUR OK")){
        logout_command(token);
        printf("successful unregister\n");
        return;
    }
    if(strcmp(buffer,"RUR UNR")){
        printf("unknown user\n");
        return;
    }
    if(strcmp(buffer,"RUR NOK")){
        printf("incorrect unregister attempt\n");
        return;
    }
    

}

void myactions_command(char *token){
    if(!logged_in){
        printf("ERR: must login first\n");
        return;
    }
    char LMA_Command[20]="LMA ";
    strcat(LMA_Command, UID_current);
    strcat(LMA_Command, "\n");
    n=sendto(UDP_fd,LMA_Command,strlen(LMA_Command),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,128,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);
    if(strncmp(buffer,"RMA OK",6)==0){
        return;
    }
    else if(strncmp(buffer,"RMA NLG",7)==0){
        printf("user not logged in!\n");
        return;
    }
    else if(strncmp(buffer,"RMA NOK",7)==0){
        printf("user has no active auction bids\n");
        return;
    }
}

int main(int argc, char **argv) {

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

    UDP_fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(UDP_fd==-1) /*error*/ exit(1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP socket

    errcode=getaddrinfo(ASIP,ASport,&hints,&res);
    if(errcode!=0) /*error*/ exit(1);

    while(1) {
        
        fgets(buffer, 128, stdin);
        char *token = strtok(buffer, " \n"); // gets the first word

        if (token != NULL && strcmp(token, "login") == 0) { // Make it into function "parse_login()"?
            // Login command
            login_command(token);
        }

        else if (token != NULL && strcmp(token, "logout") == 0) {
            // Logout command
            logout_command(token);
        }
        else if (token != NULL && strcmp(token, "unregister") == 0) {
            // sends to AS using UDP
            // asks to unregister currently logged in user. Should also be logged out
            // prints result of operation
            unregister_command(token);

        }

        else if (token != NULL && strcmp(token, "exit") == 0) {
            // if user is logged in, asks to log out
            // if logged out, terminate the application
            // doesn't communicate with AS

            if (logged_in == true) {
                printf("ERR: must log out before exiting\n");
                continue;
            }
            else {
                printf("successfully exited\n");
                break;
            }
            
        }
        else if (token != NULL && strcmp(token, "open") == 0) {
            // establishes TCP connection
            // sends message asking to open new auction (with auction's info)
            // AS returns whether request was successful and the auction's AID
            // closes connection
            
            //open_command(token);
        }
        else if (token != NULL && strcmp(token, "close") == 0) {
            // sends to AS using TCP
            // asks to close auction with given AID that had been started by logged in user
            // AS returns result of operation
            // closes connection

        }
        else if (token != NULL && (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0)) {
            myactions_command(token);
        }
        else if (token != NULL && (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "list") == 0 || strcmp(token, "l") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "bid") == 0 || strcmp(token, "b") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0)) {

        } else
            printf("ERR: Invalid command\n");
    }

    freeaddrinfo(res);
    close(UDP_fd);

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
