#include "utils.h"

#define PORT "58002"

#define BUFSIZE 6010

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

/* Logs into the Auction Server. Command format is "login UID password" */
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

/* Logs out of the Auction Server. Command format is "logout" */ 
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

/* Unregisters user from the Auction Server. Command format is "unregister" */ 
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

/* Opens a new auction. Command format is "open name asset_fname start_value timeactive" */
void open_command(char* token) {

    if (!logged_in) {
        printf("ERR: user must be logged in\n");
        return;
    }
    
    char name[11]; // 10 alphanumeric characters
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(name, token);
    else {
        printf("name argument missing\n");
        return;
    }

    char asset_fname[MAX_FILENAME+1]; // Check if it's 24 alphanumerical plus '-', '_' and '.' characters
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(asset_fname, token);
    else {
        printf("asset name argument missing\n");
        return;
    }

    char start_value[7]; // Up to 6 digits
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(start_value, token);
    else {
        printf("start value argument missing\n");
        return;
    }

    char timeactive[6]; // Up to 5 digits
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(timeactive, token);
    else {
        printf("time active argument missing\n");
        return;
    }

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

/* Closes an ongoing auction. Command format is "close AID" */
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

/* Lists the current user's auctions. Format is "myauctions" or "ma" */ 
void myauctions_command(){
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

    // Is bzero(buffer, BUFSIZE) faster?
    int buf_len = strlen(buffer);
    for (int i = 0; i < buf_len-1; i++)
        if (buffer[i] == '\n') {
            buffer[i+1] = '\0';
            break;
        }

    if(strncmp(buffer,"RMA OK",6)==0){
        printf("My auctions: %s", buffer + 7);
        return;
    }
    else if(strncmp(buffer,"RMA NLG",7)==0){
        printf("user not logged in!\n");
        return;
    }
    else if(strncmp(buffer,"RMA NOK",7)==0){
        printf("user has no ongoing auctions\n");
        return;
    }
}

/* Lists the current user's bids. Command format is "mybids" or "mb" */
void mybids_command(){
    if(!logged_in){
        printf("ERR: must login first\n");
        return;
    }

    char LMB_Command[11]="LMB ";
    int command_length = snprintf(LMB_Command, sizeof(LMB_Command), "LMB %s\n", UID_current);
    n=sendto(UDP_fd,LMB_Command,command_length,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    // Is bzero(buffer, BUFSIZE) faster?
    int buf_len = strlen(buffer);
    for (int i = 0; i < buf_len-1; i++)
        if (buffer[i] == '\n') {
            buffer[i+1] = '\0';
            break;
        }

    if(strncmp(buffer,"RMB OK",6)==0){
        printf("My bids: %s", buffer + 7);
        return;
    }
    else if(strncmp(buffer,"RMB NLG",7)==0){
        printf("user not logged in!\n");
        return;
    }
    else if(strncmp(buffer,"RMB NOK",7)==0){
        printf("user has no ongoing bids\n");
        return;
    }
}

/* Lists all currently active auctions. Command format is "list" or "l" */ 
void list_command() {

    char LST_command[] = "LST\n";
    n=sendto(UDP_fd,LST_command,strlen(LST_command),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if(strncmp(buffer,"RLS OK", 6) == 0){
        printf("List of auctions:\n%s\n", buffer + 7);
        return;
    }
    else if(strncmp(buffer,"RLS NOK", 7) == 0){
        printf("No auctions have been started\n");
        return;
    }
}

/* Shows an auction's asset. Command format is "show_asset AID" or "sa AID" */
void sas_command(char* token) {

    char AID[4]; // AID is a 3 digit number
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(AID, token);
    else {
        printf("AID argument missing\n");
        return;
    }

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
        ssize_t nleft,nwritten,nread;

        bzero(buffer, BUFSIZE);

        nread = read(TCP_fd,buffer,BUFSIZE); // Reads file name, size and data
        if (nread == -1)/*error*/exit(1);

        ptr = buffer;
        
        sscanf(buffer, "%s %ld ", fname, &fsize); // TODO: VERIFY IF RESPONSE FORMAT IS CORRECT

        int fsize_len = snprintf(NULL, 0, "%ld", fsize);
        int fname_len = strlen(fname);
        ptr += fname_len + fsize_len + 2; // Points to start of data

        FILE *file = fopen(fname, "wb");
        if (file == NULL) {
            perror("Error opening the file");
            close(TCP_fd);
            return;
        }

        ssize_t data_received;

        if (fsize > (nread - fname_len - fsize_len - 2))
            data_received = nread - fname_len - fsize_len - 2;
        else
            data_received = fsize;

        nwritten = fwrite(ptr, 1, data_received, file);

        nleft = fsize - nwritten;

        while (nleft > 0){
            bzero(buffer, BUFSIZE); // Nulls the buffer for reading

            if (nleft > BUFSIZE)
                nread = read(TCP_fd, buffer, BUFSIZE);
            else
                nread = read(TCP_fd, buffer, nleft);

            if (nread == -1) /*error*/ return;
            else if (nread == 0)
                break;//closed by peer

            nwritten += fwrite(buffer, 1, nread, file);

            nleft -= nread;
        }

        fclose(file);
        freeaddrinfo(TCP_res);
        close(TCP_fd);

        getcwd(buffer, sizeof(buffer)); // Recycles the data_buffer
        printf("File [%s] of size [%ldB] stored at %s\n", fname,fsize,buffer);

        return;
    }
}

/* Places a bid on an auctions. Command format is "bid AID value" or "b AID value" */
void bid_command(char* token) {
    if(!logged_in){
        printf("ERR: must login first\n");
        return;
    }

    char AID[4]; // AID is a 3 digit number
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(AID, token);
    else {
        printf("AID argument missing\n");
        return;
    }

    char value[9]; // Not sure what is the maximum value for bids
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(value, token);
    else {
        printf("value argument missing\n");
        return;
    }

    char BID_command[34];
    int command_len = snprintf(BID_command, sizeof(BID_command), "BID %s %s %s %s\n",
                               UID_current, password_current, AID, value);

    TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
    if (TCP_fd==-1) exit(1); //error

    memset(&TCP_hints,0,sizeof TCP_hints);
    TCP_hints.ai_family=AF_INET; //IPv4
    TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

    errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
    if(errcode!=0)/*error*/exit(1);

    n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
    if(n==-1)/*error*/exit(1);

    n=write(TCP_fd,BID_command,command_len); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    n=read(TCP_fd,buffer,BUFSIZE);
    if(n==-1)/*error*/exit(1);

    if (strncmp(buffer, "RBD NOK\n", 8) == 0) {
        printf("Auction %s is not active\n", AID);
        close(TCP_fd);
        return;
    }
    else if (strncmp(buffer, "RBD ACC\n", 8) == 0) {
        printf("Bid was accepted\n");
        close(TCP_fd);
        return;
    }
    else if (strncmp(buffer, "RBD REF\n", 8) == 0) {
        printf("Bid was refused: a larger bid was previously placed\n");
        close(TCP_fd);
        return;
    }
    else if (strncmp(buffer, "RBD ILG\n", 8) == 0) {
        printf("You cannot bid on an auction hosted by yourself\n");
        close(TCP_fd);
        return;
    }
    else if (strncmp(buffer, "RBD NLG\n", 8) == 0) {
        printf("User was not logged in\n");
        close(TCP_fd);
        return;
    }
}

void show_record_command(char *token){
    char AID[4]; // AID is a 3 digit number
    if ((token = strtok(NULL, " \n")) != NULL)
        strcpy(AID, token);
    else {
        printf("AID argument missing\n");
        return;
    }
    if(strlen(AID)!=3){
        printf("Err: AID must be a 3 digit number\n");
        return;
    }
    char SRC_command[10]; 
    snprintf(SRC_command, sizeof(SRC_command), "SRC %s\n", AID);

    n=sendto(UDP_fd,SRC_command,strlen(SRC_command),0,res->ai_addr,res->ai_addrlen);
    if(n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(UDP_fd,buffer,BUFSIZE,0,
    (struct sockaddr*)&addr,&addrlen);
    if(n==-1) /*error*/ exit(1);

    if (strncmp(buffer, "RRC NOK\n",8) == 0){
        printf("Auction %s is not active\n", AID);
        return;
    }
    else if(strncmp(buffer,"RRC OK",6) == 0){
        int bid_amount=0;

        // Moves to the first word after "RRC OK"
        token = strtok(buffer, " ");
        token=strtok(NULL," ");
        token=strtok(NULL," ");

        while(true){
            while (token != NULL) {
                printf("%s ", token);
                token = strtok(NULL, " ");
                if(strcmp(token,"E")==0 || strcmp(token,"B")==0){
                    printf("\n");
                    break;
                }
            }
            while(strcmp(token,"B")==0){
                bid_amount++;
                printf("%s ",token); /*B*/
                token=strtok(NULL," ");
                printf("%s ",token);/*UID*/
                token=strtok(NULL," ");
                printf("%s ", token); /*Value*/
                token=strtok(NULL," ");
                printf("%s ",token);/*Date-time*/
                token=strtok(NULL," ");
                printf("%s ",token); /*Sec-time*/
                token=strtok(NULL," ");
                if(strcmp(token,"E")==0|| bid_amount>50){
                    printf("\n");
                    break;
                }
            }
            if(strcmp(token,"E")==0){
                printf("%s ", token); /*E*/
                token=strtok(NULL," ");
                printf("%s ",token);/*start_time*/
                token=strtok(NULL," ");
                printf("%s ", token); /*end_time*/
                
            }
            printf("\n");
            token=strtok(NULL," ");
            break;
            
        }
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
            login_command(token);
        }
        else if (token != NULL && strcmp(token, "logout") == 0) {
            logout_command();
        }
        else if (token != NULL && strcmp(token, "unregister") == 0) {
            unregister_command();
        }
        else if (token != NULL && strcmp(token, "exit") == 0) {
            // Exit command
            if (logged_in == true)
                logout_command();
            break;
        }
        else if (token != NULL && strcmp(token, "open") == 0) {
            open_command(token);
        }
        else if (token != NULL && strcmp(token, "close") == 0) {
            close_command(token);
        }
        else if (token != NULL && (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0)) {
            myauctions_command(token);
        }
        else if (token != NULL && (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0)) {
            mybids_command();
        }
        else if (token != NULL && (strcmp(token, "list") == 0 || strcmp(token, "l") == 0)) {
            list_command();
        }
        else if (token != NULL && (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0)) {
            sas_command(token);
        }
        else if (token != NULL && (strcmp(token, "bid") == 0 || strcmp(token, "b") == 0)) {
            bid_command(token);
        }
        else if (token != NULL && (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0)) {
            show_record_command(token);
        }
        else
            printf("ERR: Invalid command\n");
    }

    printf("successfully exited\n");

    freeaddrinfo(res);
    close(UDP_fd);

    return 0;
}