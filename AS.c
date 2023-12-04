#include "utils.h"

#include <signal.h>

#define PORT "58002"

#define BUFSIZE 90

int UDP_fd,TCP_fd,errcode,n;
char buffer[BUFSIZE];

socklen_t addrlen;
struct addrinfo hints,*res;
struct sockaddr_in UDP_addr;
//struct addrinfo TCP_hints,*TCP_res;
//struct sockaddr_in TCP_addr;

bool verbose_mode = false;


void login_command(char *buffer) {
    char UID[UID_LEN+1], password[PW_LEN+1];
    char UID_directory[UID_DIR_PATH_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char UID_pass_file_path[UID_PASS_FILE_LEN+1];

    sscanf(buffer, "LIN %s %s\n", UID, password);

    if (!check_UID_format(UID)) {
        char response[] = "RLI ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }
    else if (!check_password_format(password)) {
        char response[] = "RLI ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }

    // Constructs path to user's directory
    snprintf(UID_directory, sizeof(UID_directory), "USERS/%s", UID);

    struct stat info;
    if (stat(UID_directory, &info) != 0) { // User directory does not exist
        //printf("Directory doesn't exist.\n");
        if (mkdir(UID_directory, 0700) == -1) {
            // Failed to create the directory
            perror("mkdir");
            return;
        }

        snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
        snprintf(UID_pass_file_path, sizeof(UID_pass_file_path), "USERS/%s/%s_pass.txt", UID, UID);

        FILE *login_file = fopen(UID_login_file_path, "wb");
        if (login_file == NULL) {
            perror("Error creating the login file");
            rmdir(UID_directory);
            return;
        }
        FILE *pass_file = fopen(UID_pass_file_path, "wb");
        if (pass_file == NULL) {
            perror("Error creating the password file");
            remove(UID_login_file_path);
            rmdir(UID_directory);
            return;
        }

        fwrite(password,1,PW_LEN,pass_file);

        fclose(login_file);
        fclose(pass_file);

        char response[] = "RLI REG\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);

        return;

    } else if (S_ISDIR(info.st_mode)) { // User directory already exists
        //printf("Directory exists.\n");

        snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
        snprintf(UID_pass_file_path, sizeof(UID_pass_file_path), "USERS/%s/%s_pass.txt", UID, UID);

        FILE *pass_file = fopen(UID_pass_file_path, "r");
        if (pass_file == NULL) {
            perror("Error creating the password file");
            remove(UID_login_file_path);
            rmdir(UID_directory);
            return;
        }

        fgets(buffer, 10, pass_file); // Reads saved password into the buffer

        if (strncmp(buffer,password,PW_LEN) == 0) { // If password matches
            FILE *login_file = fopen(UID_login_file_path, "w"); // Creates the login file
            if (login_file == NULL) {
                perror("Error creating the login file");
                rmdir(UID_directory);
                return;
            }

            char response[] = "RLI OK\n";
            n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
            if(n==-1) /*error*/ exit(1);

            return;
        } else { // Password doesn't match
            char response[] = "RLI NOK\n";
            n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
            if(n==-1) /*error*/ exit(1);

            return;
        }

        FILE *login_file = fopen(UID_login_file_path, "wb");
        if (login_file == NULL) {
            perror("Error creating the login file");
            rmdir(UID_directory);
            return;
        }

    } else { // File exists, but it is not a directory
        remove(UID_directory);
        char response[] = "RLO ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }

    return;
}

void logout_command(char* buffer) {
    char UID[UID_LEN+1], password[PW_LEN+1];
    char UID_directory[UID_DIR_PATH_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];

    sscanf(buffer, "LOU %s %s\n", UID, password);

    if (!check_UID_format(UID)) {
        char response[] = "RLO ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }
    else if (!check_password_format(password)) {
        char response[] = "RLO ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_directory, sizeof(UID_directory), "USERS/%s", UID);

    struct stat info;
    if (stat(UID_directory, &info) != 0) { // User directory was not registered
        char response[] = "RLO UNR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);

    if (file_exists(UID_login_file_path)) { // User is logged in
        remove(UID_login_file_path);

        char response[] = "RLO OK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    } else { // User is not logged in
        char response[] = "RLO NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,addrlen);
        if(n==-1) /*error*/ exit(1);
        return;
    }
}

int main(int argc, char **argv) {
    fd_set inputs, testfds;
    struct timeval timeout;

    int out_fds, ret;

    char in_str[128];
    char command[4];

    char* ASport = (char*) PORT;

    if (argc >= 2) /* At least one argument */
        for (int i = 1; i < argc; i += 2) {
            if (strcmp(argv[i], "-p") == 0)
                ASport = argv[i+1]; // The port follows after "-p"
            else if (strcmp(argv[i], "-v") == 0)
                verbose_mode = true; // Opens in verbose mode
        }
    
    // Ignore SIGPIPE signal
    struct sigaction act;
    memset(&act,0,sizeof act);
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL)==-1)/*error*/exit(1);

    /*// Register the custom signal handler for SIGINT
    struct sigaction act_int;
    memset(&act_int, 0, sizeof(act_int));
    act_int.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &act_int, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }*/
    
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

    if((errcode=getaddrinfo(NULL,ASport,&hints,&res))!=0)
        exit(1);

    UDP_fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if(UDP_fd==-1)
        exit(1);

    if(bind(UDP_fd,res->ai_addr,res->ai_addrlen)==-1) {
        sprintf(buffer,"Bind error UDP server\n");
        write(1,buffer,strlen(buffer));
        exit(1);
    }
    freeaddrinfo(res);

    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0,&inputs); // Set standard input channel on
    FD_SET(UDP_fd,&inputs); // Set UDP channel on

    /*TCP_fd=socket(AF_INET,SOCK_STREAM,0);
    if (TCP_fd==-1) /error/ exit(1);

    memset(&TCP_hints,0,sizeof hints);
    TCP_hints.ai_family=AF_INET;//IPv4
    TCP_hints.ai_socktype=SOCK_STREAM;//UDP socket
    TCP_hints.ai_flags=AI_PASSIVE;
    
    errcode=getaddrinfo(NULL,ASport,&hints,&res);
    if (errcode!=0) /error/ exit(1);

    if(bind(TCP_fd,res->ai_addr,res->ai_addrlen) == -1)
        exit(1); //error*/

    while(1)
    {
        testfds = inputs; // Reload mask
//        printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        out_fds=select(FD_SETSIZE,&testfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) &timeout);
// testfds is now '1' at the positions that were activated
//        printf("testfds byte: %d\n",((char *)&testfds)[0]); // Debug

        switch(out_fds)
        {
            case 0:
                printf("\n ---------------Timeout event-----------------\n");
                break;
            case -1:
                perror("select");
                exit(1);
            default:
                if(FD_ISSET(0,&testfds)) // Detects keyboard input
                {
                    fgets(in_str,50,stdin);
                    printf("---Input at keyboard: %s\n",in_str);
                }
                if(FD_ISSET(UDP_fd,&testfds)) // Received from UDP
                {
                    addrlen = sizeof(UDP_addr);
                    ret=recvfrom(UDP_fd,buffer,BUFSIZE,0,(struct sockaddr *)&UDP_addr,&addrlen);
                    if(ret>=0) {
                        if(strlen(buffer)>0)
                            buffer[ret-1]=0;
                        
                        sscanf(buffer, "%s ", command);

                        if (strcmp(command, "LIN") == 0) {
                            login_command(buffer);
                        }
                        else if (strcmp(command, "LOU") == 0) {
                            logout_command(buffer);
                        }
                        //TODO: other commands
                        else {
                            snprintf(buffer,sizeof(buffer), "ERR\n");
                            n=sendto(UDP_fd,buffer,4,0,(struct sockaddr*)&UDP_addr,addrlen);
                            if(n==-1) /*error*/ exit(1);
                        }
                    }
                }
        }
    }
    freeaddrinfo(res);
    close(UDP_fd);

    return 0;
}


/*
I'm pretty sure this is used for verbose mode

char host[NI_MAXHOST], service[NI_MAXSERV];
errcode=getnameinfo( (struct sockaddr *) &UDP_addr,addrlen,host,sizeof host, service,sizeof service,0);
if(errcode==0)
    printf("       Sent by [%s:%s]\n",host,service);
*/