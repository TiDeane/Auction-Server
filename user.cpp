#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>

#define PORT "58011"

#define BUFSIZE 2048

int UDP_fd,TCP_fd,errcode;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in addr;
struct addrinfo TCP_hints,*TCP_res;
struct sockaddr_in TCP_addr;
char buffer[BUFSIZE];

/* Default IP and PORT values */
char* ASIP = (char*) "127.0.0.1";
char* ASport = (char*) PORT;

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
        printf("ERR: user must be logged out\n");
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

    char LIN_command[21]; // Use buffer?
    snprintf(LIN_command, sizeof(LIN_command), "LIN %s %s\n", UID, password);

    n=sendto(UDP_fd,LIN_command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
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

void logout_command() {
    if (!logged_in) {
        printf("ERR: user must be logged in\n");
        return;
    }

    char LOU_command[21]; // Use buffer?
    snprintf(LOU_command, sizeof(LOU_command), "LOU %s %s\n", UID_current, password_current);

    n=sendto(UDP_fd,LOU_command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
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

void unregister_command(){

    if(!logged_in){
        printf("ERR: user must be logged in\n");
        return;
    }

    char UNR_Command[21]; // Use buffer?
    snprintf(UNR_Command, sizeof(UNR_Command), "UNR %s %s\n", UID_current, password_current);

    n=sendto(UDP_fd,UNR_Command,20,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if(strncmp(buffer,"RUR OK\n", 7) == 0){
        logged_in = false;
        printf("successful unregister\n");
        return;
    }
    else if(strncmp(buffer,"RUR UNR\n", 8) == 0){
        printf("unknown user\n");
        return;
    }
    else if(strncmp(buffer,"RUR NOK\n", 8) == 0){
        printf("incorrect unregister attempt\n");
        return;
    }
}

void open_command(char* token) {

    if (!logged_in) {
        printf("ERR: user must be logged in\n");
        return;
    }
    
    char name[47]; // One word
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(name, token);

    char asset_fname[25]; // Check if it's 24 alphanumerical plus '-', '_' and '.' characters
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(asset_fname, token);

    char start_value[11];
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(start_value, token);

    char timeactive[11];
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(timeactive, token);

    FILE *file = fopen(asset_fname, "rb");
    if (file == NULL) {
        perror("Error opening the file");
        return;
    }
    
    // Get file size using fstat
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        fclose(file);
        perror("Error getting file information");
        return;
    }
    long Fsize = file_info.st_size;
    
    if (Fsize > 99999999) {
        printf("File size is too large\n");
        fclose(file);
        return;
    }

    int command_length = snprintf(buffer, sizeof(buffer), "OPA %s %s %s %s %s %s %ld ",
                                  UID_current, password_current, name, start_value, timeactive,
                                  asset_fname, Fsize);

    TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if (TCP_fd==-1) exit(1); //error

    memset(&TCP_hints,0,sizeof TCP_hints);
    TCP_hints.ai_family=AF_INET; //IPv4
    TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

    errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
    if(errcode!=0)/*error*/exit(1);

    n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    n=write(TCP_fd,buffer,command_length); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    int file_fd = fileno(file);
    off_t offset = 0;
    off_t remaining = Fsize;
    while (remaining > 0) {
        ssize_t sent_bytes = sendfile(TCP_fd, file_fd, &offset, remaining);
        if (sent_bytes == -1) {
            perror("Error sending file content");
            fclose(file);
            return;
        }
        remaining -= sent_bytes;
    }

    n=read(TCP_fd,buffer,512); // Read multiple times?
    if(n==-1)/*error*/exit(1);

    fclose(file);
    freeaddrinfo(TCP_res);
    close(TCP_fd);

    char status[4];
    int AID;
    sscanf(buffer, "ROA %s %d", status, &AID);
    
    if (strcmp(status, "OK") == 0) {
        printf("Auction [%d] successfully created\n", AID);
        return;
    }
    else if (strcmp(status, "NOK") == 0) {
        printf("Auction could not be started\n");
        return;
    }
    else if (strcmp(status, "NLG") == 0) {
        printf("User was not logged into the server\n");
        return;
    }
}

int main(int argc, char **argv) {

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
        
        fgets(buffer, BUFSIZE, stdin);
        char *token = strtok(buffer, " \n"); // gets the first word

        if (token != NULL && strcmp(token, "login") == 0) {
            // Login command
            login_command(token);
        }

        else if (token != NULL && strcmp(token, "logout") == 0) {
            // Logout command
            logout_command();
        }
        else if (token != NULL && strcmp(token, "unregister") == 0) {
            // Unregister command
            unregister_command();
        }

        else if (token != NULL && strcmp(token, "exit") == 0) {
            // Exit command

            if (logged_in == true) {
                printf("ERR: user must logout before exiting\n");
                continue;
            }
            else {
                printf("successfully exited\n");
                break;
            }
            
        }
        else if (token != NULL && strcmp(token, "open") == 0) {
            // Open command
            open_command(token);
        }
        else if (token != NULL && strcmp(token, "close") == 0) {
            // sends to AS using TCP
            // asks to close auction with given AID that had been started by logged in user
            // AS returns result of operation
            // closes connection

        }
        else if (token != NULL && (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0)) {

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