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
#define MAX_FILENAME 24

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
        printf("ERR: user is already logged in\n");
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
    
    char name[11]; // 10 alphanumeric characters
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(name, token);

    char asset_fname[MAX_FILENAME+1]; // Check if it's 24 alphanumerical plus '-', '_' and '.' characters
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(asset_fname, token);

    char start_value[7]; // Up to 6 digits
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(start_value, token);

    char timeactive[6]; // Up to 5 digits
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(timeactive, token);

    FILE *file = fopen(asset_fname, "rb");
    if (file == NULL) {
        perror("Error opening the file");
        return;
    }
    
    // Get file size using fstat
    struct stat file_info;
    int file_fd = fileno(file);
    if (fstat(file_fd, &file_info) != 0) {
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

    off_t offset = 0;
    ssize_t bytes_sent;
    while (offset < Fsize) {
        bytes_sent = sendfile(TCP_fd, file_fd, &offset, Fsize - offset);
        if (bytes_sent == -1) {
            perror("Error sending file content");
            fclose(file);
            close(TCP_fd);
            return;
        }
    }

    n=read(TCP_fd,buffer,BUFSIZE); // Read multiple times?
    if(n==-1)/*error*/exit(1);

    fclose(file);
    freeaddrinfo(TCP_res);
    close(TCP_fd);

    char status[4];
    int AID;
    sscanf(buffer, "ROA %s %d\n", status, &AID);
    
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

void close_command(char* token) {
    if (!logged_in) {
        printf("User must be logged in\n");
        return;
    }

    char AID[4]; // AID is a 3 digit number
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(AID, token);
    else {
        printf("AID argument missing\n");
        return;
    }

    char CLS_command[25];
    snprintf(CLS_command, sizeof(CLS_command), "CLS %s %s %s\n", UID_current, password_current, AID);

    TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if (TCP_fd==-1) exit(1); //error

    memset(&TCP_hints,0,sizeof TCP_hints);
    TCP_hints.ai_family=AF_INET; //IPv4
    TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

    errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
    if(errcode!=0)/*error*/exit(1);

    n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    n=write(TCP_fd,CLS_command,24); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    n=read(TCP_fd,buffer,512); // Read multiple times?
    if(n==-1)/*error*/exit(1);

    freeaddrinfo(TCP_res);
    close(TCP_fd);

    if(strncmp(buffer,"RCL OK\n", 7) == 0){
        printf("Auction was successfully closed\n");
        return;
    }
    else if(strncmp(buffer,"RCL EAU\n", 8) == 0){
        printf("The auction EAU does not exist\n");
        return;
    }
    else if(strncmp(buffer,"RCL EOW\n", 8) == 0){
        printf("You do not own this auction\n");
        return;
    }
    else if(strncmp(buffer,"RCL END\n", 8) == 0){
        printf("The auction has already finished\n");
        return;
    }
    else if(strncmp(buffer,"RCL NLG\n", 8) == 0){
        printf("User was not logged into the Auction Server\n");
        return;
    }
}

void myauctions_command(char *token){
    if(!logged_in){
        printf("ERR: must login first\n");
        return;
    }
    char LMA_Command[20]="LMA ";
    int command_length = snprintf(LMA_Command, sizeof(LMA_Command), "LMA %s\n", UID_current);
    n=sendto(UDP_fd,LMA_Command,command_length,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if(strncmp(buffer,"RMA OK",6)==0){
        printf("My auctions: %s", buffer + 7);
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

void list_command() {

    char LST_command[] = "LST\n";
    n=sendto(UDP_fd,LST_command,strlen(LST_command),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if(strncmp(buffer,"RLS OK", 6) == 0){
        printf("List of auctions:\n%s\n", buffer + 7); // Pôr cada par "AID state" entre parenteses retos?
        return;
    }
    else if(strncmp(buffer,"RLS NOK", 7) == 0){
        printf("No auctions have been started\n");
        return;
    }
}

void sas_command(char* token) {
    char AID[4]; // AID is a 3 digit number
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(AID, token);

    char SAS_command[9];
    snprintf(SAS_command, sizeof(SAS_command), "SAS %s\n", AID);
    
    TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if (TCP_fd==-1) exit(1); //error

    memset(&TCP_hints,0,sizeof TCP_hints);
    TCP_hints.ai_family=AF_INET; //IPv4
    TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

    errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
    if(errcode!=0)/*error*/exit(1);

    n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    n=write(TCP_fd,SAS_command,8); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    n=read(TCP_fd,buffer,BUFSIZE); // Reads the command and status
    if(n==-1)/*error*/exit(1);

    char status[4];
    sscanf(buffer, "RSA %3s", status);

    if (strcmp(status, "NOK") == 0) {
        printf("There is no file to be sent, or a problem ocurred\n");
        close(TCP_fd);
        return;
    }
    else if (strcmp(status, "OK") == 0) {
        char fname[MAX_FILENAME+1], *ptr;
        long fsize;
        ssize_t nbytes,nleft,nwritten,nread;

        nread = read(TCP_fd,buffer,BUFSIZE); // Reads file name, size and data
        if (nread == -1)/*error*/exit(1);

        char write_buffer[BUFSIZE];
        sscanf(buffer, "%s %ld %s", fname, &fsize, write_buffer);

        // aumentar o pointer para o inicio da data para escrever, ao inves de usar
        // o write buffer?

        FILE *file = fopen(fname, "wb");
        if (file == NULL) {
            perror("Error opening the file");
            close(TCP_fd);
            return;
        }

        nwritten = fwrite(write_buffer, 1, fsize, file);

        nleft = fsize - nwritten;
        ptr = buffer;
        while (nleft > 0){
            nread = read(TCP_fd, ptr, nleft);
            if (nread == -1) /*error*/ return;
            else if (nread == 0)
                break;//closed by peer

            fwrite(ptr, 1, nread, file);

            nleft -= nread;
            ptr += nread;
        }

        fclose(file);
        freeaddrinfo(TCP_res);
        close(TCP_fd);

        getcwd(write_buffer, sizeof(write_buffer));
        printf("File [%s] of size [%ldB] stored at %s\n", fname,fsize,write_buffer);

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
                logout_command();
                printf("successfully exited\n");
                break;
            } else {
                printf("successfully exited\n");
                break;
            }
        }
        else if (token != NULL && strcmp(token, "open") == 0) {
            // Open command
            open_command(token);
        }
        else if (token != NULL && strcmp(token, "close") == 0) {
            // Close command
            close_command(token);
        }
        else if (token != NULL && (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0)) {
            // My Auctions command
            myauctions_command(token);
        }
        else if (token != NULL && (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "list") == 0 || strcmp(token, "l") == 0)) {
            // List command
            list_command();
        }
        else if (token != NULL && (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0)) {
            sas_command(token);
        }
        else if (token != NULL && (strcmp(token, "bid") == 0 || strcmp(token, "b") == 0)) {

        }
        else if (token != NULL && (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0)) {

        }
        else
            printf("ERR: Invalid command\n");
    }

    freeaddrinfo(res);
    close(UDP_fd);

    return 0;
}