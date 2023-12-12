#include "utils.h"

#define PORT "58002"

#define BUFSIZE 6010
#define MAX_AUCTION 999

int UDP_fd,TCP_fd,errcode;
char buffer[BUFSIZE];

socklen_t addrlen, TCP_addrlen;
struct addrinfo hints,*res;
struct sockaddr_in UDP_addr;
struct addrinfo TCP_hints,*TCP_res;
struct sockaddr_in TCP_addr;

bool verbose_mode = false;

void sigint_handler(int signum) {
    printf("Terminating server...\n");
    close(UDP_fd);
    close(TCP_fd);
    exit(signum);
}

void init_signals() {
    struct sigaction act;
    memset(&act,0,sizeof act);
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL)==-1)/*error*/exit(1);

    struct sigaction act_child;
    memset(&act_child, 0, sizeof(act_child));
    act_child.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act_child, NULL) == -1)
        exit(1);
    
    struct sigaction act_sev;
    memset(&act_sev, 0, sizeof(act_sev));
    act_sev.sa_handler = SIG_IGN;
    if (sigaction(SIGSEGV, &act_sev, NULL) == -1)
        exit(1);

    struct sigaction act_int;
    memset(&act_int, 0, sizeof(act_int));
    act_int.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &act_int, NULL) == -1)
        exit(1);
}

int get_new_AID() {
    struct dirent **filelist;
    int n_entries, AID;

    char dirname[] = "AUCTIONS/";
    n_entries = scandir(dirname, &filelist, NULL, alphasort);
    if (n_entries == 3) {
        free(filelist[0]); // "."
        free(filelist[1]); // ".."
        free(filelist[2]); // ".gitkeep"
        free(filelist);
        return 1;
    }
    AID = atoi(filelist[n_entries-1]->d_name) + 1;
    for (int i = 0; i < n_entries; i++)
        free(filelist[i]);
    free(filelist);

    return AID;
}

int create_login_files(char *UID, char *password) {
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char UID_pass_file_path[UID_PASS_FILE_LEN+1];

    snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
    snprintf(UID_pass_file_path, sizeof(UID_pass_file_path), "USERS/%s/%s_pass.txt", UID, UID);

    FILE *login_file = fopen(UID_login_file_path, "wb");
    if (login_file == NULL) {
        perror("Error creating the login file");
        return -1;
    }
    FILE *pass_file = fopen(UID_pass_file_path, "wb");
    if (pass_file == NULL) {
        perror("Error creating the password file");
        remove(UID_login_file_path);
        fclose(login_file);
        return -1;
    }

    fwrite(password,1,PW_LEN,pass_file);

    fclose(login_file);
    fclose(pass_file);

    return 0;
}

/* Ends auction AID, that lasted duration_sec, at current_time */
void end_auction(int AID, long duration_sec, time_t current_time) {
    char end_info[DATE_LEN+TIME_LEN+MAX_FULLTIME+3];
    char pathname[32];
    char end_datetime[DATE_LEN+TIME_LEN+2];
    
    sprintf(pathname,"AUCTIONS/%03d/END_%03d.txt",AID,AID);
    if (file_exists(pathname))
        return;
    FILE *end_file = fopen(pathname, "w");
    if (end_file == NULL ) {
        perror("Error creating the end file");
        return;
    }
    
    struct tm *timeinfo = gmtime(&current_time);
    if (timeinfo != NULL)
        strftime(end_datetime, sizeof(end_datetime), "%Y-%m-%d %H:%M:%S", timeinfo);
    else
        return;
    
    sprintf(end_info,"%s %ld",end_datetime,duration_sec);
    fwrite(end_info,1,strlen(end_info),end_file);
    fclose(end_file);
}

void login_command(char *buffer) {
    char UID[UID_LEN+1], password[PW_LEN+1];
    char UID_directory[UID_DIR_PATH_LEN+1];
    char UID_HOST_directory[UID_HOST_DIR_PATH_LEN+1];
    char UID_BID_directory[UID_BID_DIR_PATH_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char UID_pass_file_path[UID_PASS_FILE_LEN+1];
    int n, sscanf_ret;

    sscanf_ret = sscanf(buffer, "LIN %s %s\n", UID, password);

    if (sscanf_ret != 2 || !check_UID_format(UID) || !check_password_format(password)) {
        char response[] = "RLI ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_directory, sizeof(UID_directory), "USERS/%s", UID);

    if (!dir_exists(UID_directory)) { // User directory does not exist, first time registering

        if (mkdir(UID_directory, 0700) == -1) {
            perror("mkdir");
            return;
        }

        if (create_login_files(UID, password) == -1) {
            printf("ERR: could not create UID's files\n");
            return;
        }

        snprintf(UID_HOST_directory, sizeof(UID_HOST_directory), "USERS/%s/HOSTED", UID);
        snprintf(UID_BID_directory, sizeof(UID_BID_directory), "USERS/%s/BIDDED", UID);

        if (mkdir(UID_HOST_directory, 0700) == -1) {
            perror("mkdir");
            return;
        }
        if (mkdir(UID_BID_directory, 0700) == -1) {
            perror("mkdir");
            return;
        }
        
        char response[] = "RLI REG\n"; // New user registered
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else { // User directory already exists, has registered before

        snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
        snprintf(UID_pass_file_path, sizeof(UID_pass_file_path), "USERS/%s/%s_pass.txt", UID, UID);

        if (file_exists(UID_pass_file_path)) { // User is registered

            FILE *pass_file = fopen(UID_pass_file_path, "r");
            if (pass_file == NULL) {
                perror("Error creating the password file");
                remove(UID_login_file_path);
                rmdir(UID_directory);
                return;
            }

            fgets(buffer, PW_LEN+1, pass_file); // Reads saved password into the buffer

            if (strncmp(buffer,password,PW_LEN) == 0) { // If password matches
                FILE *login_file = fopen(UID_login_file_path, "w"); // Creates the login file
                if (login_file == NULL) {
                    perror("Error creating the login file");
                    rmdir(UID_directory);
                    fclose(pass_file);
                    return;
                }

                char response[] = "RLI OK\n"; // Successfully logged in
                n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
                if(n==-1) /*error*/ exit(1);

                fclose(login_file);
                fclose(pass_file);
                return;
            } else { // Password doesn't match
                char response[] = "RLI NOK\n"; // Unsuccessful login
                n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
                if(n==-1) /*error*/ exit(1);

                fclose(pass_file);
                return;
            }
        } else { // Directory exists but user is not registered, returning user
            if (create_login_files(UID, password) == -1) {
                printf("ERR: could not create UID's files\n");
                return;
            }

            char response[] = "RLI REG\n"; // Successfully registered again
            n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
            if(n==-1) /*error*/ exit(1);
            return;
        }
    }

    return;
}

void logout_command(char* buffer) {
    char UID[UID_LEN+1], password[PW_LEN+1];
    char UID_directory[UID_DIR_PATH_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    int n, sscanf_ret;

    sscanf_ret = sscanf(buffer, "LOU %s %s\n", UID, password);

    if (sscanf_ret != 2 || !check_UID_format(UID) || !check_password_format(password)) {
        char response[] = "RLO ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_directory, sizeof(UID_directory), "USERS/%s", UID);

    if (!dir_exists(UID_directory)) { // User directory doesn't exist
        char response[] = "RLO UNR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);

    if (file_exists(UID_login_file_path)) { // User is logged in
        remove(UID_login_file_path);

        char response[] = "RLO OK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else { // User is not logged in
        char response[] = "RLO NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }
}

void unregister_command(char* buffer) {
    char UID[UID_LEN+1], password[PW_LEN+1];
    char UID_directory[UID_DIR_PATH_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char UID_pass_file_path[UID_PASS_FILE_LEN+1];
    int n, sscanf_ret;

    sscanf_ret = sscanf(buffer, "UNR %s %s\n", UID, password);

    if (sscanf_ret != 2 || !check_UID_format(UID) || !check_password_format(password)) {
        char response[] = "RUR ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_directory, sizeof(UID_directory), "USERS/%s", UID);

    if (!dir_exists(UID_directory)) { // User directory does not exist
        char response[] = "RUR UNR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
    
    if (file_exists(UID_login_file_path)) { // User was logged in
        snprintf(UID_pass_file_path, sizeof(UID_pass_file_path), "USERS/%s/%s_pass.txt", UID, UID);

        remove(UID_login_file_path);
        remove(UID_pass_file_path);

        char response[] = "RUR OK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else {
        char response[] = "RUR NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }
}

void open_command(char* buffer, int new_fd, int nread) {
    char UID[UID_LEN+1], UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char password[PW_LEN+1];
    char dirname[21];
    char pathname[38];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char date_time[DATE_LEN+TIME_LEN+2];
    char sfilecontents[UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                       MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8];
    char* ptr;
    int svalue, timeactive, n, nleft, nwritten, AID, sscanf_ret;
    long fsize;
    ssize_t data_received;

    sscanf_ret = sscanf(buffer, "OPA %s %s %s %d %d %s %ld ", UID, password, name, &svalue, &timeactive, fname, &fsize);

    if (sscanf_ret != 7||!check_UID_format(UID)||!check_password_format(password)||!check_desc_name_format(name)||
        !valid_value(svalue)||!valid_timeactive(timeactive)||!valid_filesize(fsize)) {
        char response[] = "ROA ERR\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }
    
    snprintf(UID_login_file_path, sizeof(UID_login_file_path), "USERS/%s/%s_login.txt", UID, UID);
    if (!file_exists(UID_login_file_path)) { // User is not logged in
        char response[] = "ROA NLG\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    AID = get_new_AID(); 
    if (!valid_AID(AID)) { 
        char response[] = "ROA NOK\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(dirname, "AUCTIONS/%03d/", AID);
    if (mkdir(dirname, 0700) == -1) {
        perror("mkdir");
        return;
    }
    sprintf(dirname, "AUCTIONS/%03d/BIDS/", AID);
    if (mkdir(dirname, 0700) == -1) {
        sprintf(dirname, "AUCTIONS/%03d/", AID);
        remove(dirname);
        perror("mkdir");
        return;
    }
    sprintf(pathname, "AUCTIONS/%03d/START_%03d.txt", AID, AID);
    FILE *start_file = fopen(pathname, "wb");
    if (start_file == NULL) {
        sprintf(dirname, "AUCTIONS/%03d/", AID);
        remove(dirname);
        perror("Error creating the login file");
        return;
    }

    time_t fulltime = time(NULL);
    struct tm *timeinfo = gmtime(&fulltime);
    if (timeinfo != NULL)
        strftime(date_time, sizeof(date_time), "%Y-%m-%d %H:%M:%S", timeinfo);
    else
        return;

    sprintf(sfilecontents,"%s %s %s %d %d %s %ld",
            UID,name,fname,svalue,timeactive,date_time,fulltime);
    fwrite(sfilecontents,1,strlen(sfilecontents),start_file);

    sprintf(pathname,"USERS/%s/HOSTED/%03d.txt", UID, AID);
    FILE *hosted_file = fopen(pathname, "w");
    if (hosted_file == NULL) {
        sprintf(dirname, "AUCTIONS/%03d/", AID);
        remove(dirname);
        perror("Error creating the login file");
        return;
    }

    ptr = buffer;
    int name_len = strlen(name);
    int fname_len = strlen(fname);
    int svalue_len = snprintf(NULL, 0, "%d", svalue);
    int timeactive_len = snprintf(NULL, 0, "%d", timeactive);
    int fsize_len = snprintf(NULL, 0, "%ld", fsize);
    int data_start = 3+UID_LEN+PW_LEN+name_len+svalue_len+timeactive_len+fname_len+fsize_len+8;
    ptr += data_start; // Points to start of data

    sprintf(pathname,"AUCTIONS/%03d/%s",AID,fname);
    FILE *image_file = fopen(pathname, "wb");
    if (image_file == NULL) {
        perror("Error opening the file");
        remove(dirname);
        return;
    }

    if (fsize > (nread - data_start))
        data_received = nread - data_start;
    else
        data_received = fsize;

    if (data_received>0)
        nwritten = fwrite(ptr, 1, data_received, image_file);
    else
        nwritten = 0;

    nleft = fsize - nwritten;

    while (nleft > 0){
        if (nleft > BUFSIZE)
            nread = read(new_fd, buffer, BUFSIZE);
        else
            nread = read(new_fd, buffer, nleft);

        if (nread == -1) /*error*/ return;
        else if (nread == 0)
            break;//closed by peer

        nwritten += fwrite(buffer, 1, nread, image_file);

        nleft -= nread;
    }

    if (timeactive == 0)
        end_auction(AID, timeactive, fulltime);

    char response[12];
    sprintf(response, "ROA OK %03d\n", AID);
    n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    return;
}

void close_command(char* buffer, int new_fd) {
    char UID_given[UID_LEN+1];
    char UID_start[UID_LEN+1];
    char password[PW_LEN+1];
    char dirname[21];
    char pathname[32];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char sdate[DATE_LEN+1], stime[TIME_LEN+1];
    long stime_seconds, end_sec_time;
    int AID, n, svalue, timeactive, sscanf_ret;
    int start_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                     MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    time_t current_time;

    sscanf_ret = sscanf(buffer,"CLS %s %s %d\n", UID_given, password, &AID);

    if (sscanf_ret != 3 || !check_UID_format(UID_given) ||
        !check_password_format(password) || !valid_AID(AID)) {
        char response[] = "RCL ERR\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(dirname, "USERS/%s/", UID_given);
    if (!dir_exists(dirname)) {
        char response[] = "RCL NOK\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(pathname,"USERS/%s/%s_pass.txt",UID_given,UID_given);
    FILE *pass_file = fopen(pathname, "r");
    if (pass_file == NULL) {
        perror("Error creating the password file");
        return;
    }
    fgets(buffer, PW_LEN+1, pass_file); // Reads saved password into the buffer
    if (strncmp(buffer,password,PW_LEN) != 0) { // If password doesn't match
        fclose(pass_file);
        char response[] = "RCL NOK\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }
    fclose(pass_file);

    sprintf(pathname,"USERS/%s/%s_login.txt",UID_given,UID_given);
    if (!file_exists(pathname)) { // User is not logged in
        char response[] = "RCL NLG\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }
    sprintf(dirname, "AUCTIONS/%03d/", AID);
    if (!dir_exists(dirname)) { // Auction doesn't exist
        char response[] = "RCL EAU\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(pathname, "AUCTIONS/%03d/START_%03d.txt",AID,AID);
    FILE *start_file = fopen(pathname, "r");
    if (start_file == NULL) {
        perror("Error creating the password file");
        return;
    }
    fgets(buffer, start_size, start_file);
    sscanf(buffer,"%s %s %s %d %d %s %s %ld",UID_start,name,fname,&svalue,&timeactive,sdate,stime,&stime_seconds);
    fclose(start_file);
    
    current_time = time(NULL);
    end_sec_time = difftime(current_time,(time_t)stime_seconds);
    if (end_sec_time >= timeactive) { // Ends auction preemptively if it's duration has been exceeded
        end_sec_time = timeactive;
        current_time = stime_seconds + timeactive;
        end_auction(AID,end_sec_time,current_time);
    }

    if (strcmp(UID_start,UID_given) != 0) { // Auction is not owned by given UID
        char response[] = "RCL EOW\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(pathname, "AUCTIONS/%03d/END_%03d.txt",AID,AID);
    if (file_exists(pathname)) { // Auction has already ended
        char response[] = "RCL END\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    } else {   
        end_auction(AID,end_sec_time,current_time);

        char response[] = "RCL OK\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }
}

void show_asset_command(char* buffer, int new_fd) {
    char pathname[46];
    char UID[UID_LEN+1];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char sdate[DATE_LEN+1], stime[TIME_LEN+1];
    int AID, n, svalue, timeactive, command_length, sscanf_ret;
    int start_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                     MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    long stime_seconds, end_sec_time;
    time_t current_time;

    sscanf_ret = sscanf(buffer, "SAS %d\n", &AID);

    if (sscanf_ret != 1 || !valid_AID(AID)) {
        char response[] = "RSA ERR\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }

    sprintf(pathname,"AUCTIONS/%03d/START_%03d.txt",AID,AID);
    FILE *start_file = fopen(pathname, "r");
    if (start_file == NULL) {
        perror("Error opening the START file");
        return;
    }
    fgets(buffer,start_size,start_file);
    sscanf(buffer,"%s %s %s %d %d %s %s %ld",UID,name,fname,&svalue,&timeactive,sdate,stime,&stime_seconds);
    fclose(start_file);

    current_time = time(NULL);
    end_sec_time = difftime(current_time,(time_t)stime_seconds);
    if (end_sec_time >= timeactive) { // Ends auction if it's duration has been exceeded
        end_sec_time = timeactive;
        current_time = stime_seconds + timeactive;
        end_auction(AID,end_sec_time,current_time);
    }

    sprintf(pathname,"AUCTIONS/%03d/%s",AID,fname);
    if (!file_exists(pathname)) {
        char response[] = "RSA NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }
    FILE *file = fopen(pathname, "rb");
    if (file == NULL) {
        perror("Error opening the file");
        return;
    }

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

    command_length = sprintf(buffer, "RSA OK %s %ld ", fname, Fsize);
    n=write(new_fd,buffer,command_length); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);

    off_t offset = 0;
    ssize_t bytes_sent;
    while (offset < Fsize) {
        bytes_sent = sendfile(new_fd, file_fd, &offset, Fsize - offset);
        if (bytes_sent == -1) {
            perror("Error sending file content");
            fclose(file);
            return;
        }
    }
    fclose(file);
}

void myauctions_command(char* buffer) {
    struct dirent **filelist;
    int sscanf_ret, AID, svalue, timeactive, n, n_entries, i, len;
    int start_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                     MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    long stime_seconds, end_sec_time;
    char sfilecontents[start_size];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char sdate[DATE_LEN+1], stime[TIME_LEN+1];
    char UID[UID_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char dirname[21];
    char pathname[32];
    char AID_state[AID_LEN+3];
    time_t current_time;

    sscanf_ret = sscanf(buffer, "LMA %s\n", UID);
    if (sscanf_ret != 1 || !check_UID_format(UID)) {
        char response[] = "RMA ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    sprintf(UID_login_file_path, "USERS/%s/%s_login.txt", UID, UID);
    if (!file_exists(UID_login_file_path)) {
        char response[] = "RMA NLG\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    sprintf(dirname,"USERS/%s/HOSTED/", UID);
    n_entries = scandir(dirname, &filelist, NULL, alphasort);
    if (n_entries == 2) { // user has no auctions
        free(filelist[0]); // "."
        free(filelist[1]); // ".."
        free(filelist);
        char response[] = "RMA NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else if (n_entries<=0)
        return;

    strcpy(buffer,"RMA OK");

    for (i = 0; i < n_entries; i++){
        len = strlen(filelist[i]->d_name);
        if (len == 7) {
            strcat(buffer," ");

            sscanf(filelist[i]->d_name,"%03d.txt",&AID);

            // Opens START file to see if auction has exceeded its duration
            sprintf(pathname,"AUCTIONS/%03d/START_%03d.txt",AID,AID);
            FILE *start_file = fopen(pathname, "r");
            if (start_file == NULL) {
                perror("Error opening the START file");
                return;
            }
            fgets(sfilecontents,start_size,start_file);
            sscanf(sfilecontents,"%s %s %s %d %d %s %s %ld",UID,name,fname,
                                &svalue,&timeactive,sdate,stime,&stime_seconds);
            fclose(start_file);

            current_time = time(NULL);
            end_sec_time = difftime(current_time,(time_t)stime_seconds);
            if (end_sec_time >= timeactive) { // Ends auction if it's duration has been exceeded
                end_sec_time = timeactive;
                current_time = stime_seconds + timeactive;
                end_auction(AID,end_sec_time,current_time);
            }

            sprintf(pathname,"AUCTIONS/%03d/END_%03d.txt",AID,AID);
            if (file_exists(pathname))
                sprintf(AID_state,"%03d 0", AID);
            else
                sprintf(AID_state,"%03d 1", AID);
            
            strcat(buffer, AID_state);
        }
        free(filelist[i]);
    }
    free(filelist);

    strcat(buffer,"\n");
    n=sendto(UDP_fd,buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
    if(n==-1) /*error*/ exit(1);

    return;
}

void mybids_command(char* buffer) {
    struct dirent **filelist;
    int sscanf_ret, AID, svalue, timeactive, n, n_entries, i, len;
    int start_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                     MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    long stime_seconds, end_sec_time;
    char sfilecontents[start_size];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char sdate[DATE_LEN+1], stime[TIME_LEN+1];
    char UID[UID_LEN+1];
    char UID_login_file_path[UID_LOGIN_FILE_LEN+1];
    char dirname[21];
    char pathname[32];
    char AID_state[AID_LEN+3];
    time_t current_time;

    sscanf_ret = sscanf(buffer, "LMB %s\n", UID);
    if (sscanf_ret != 1 || !check_UID_format(UID)) {
        char response[] = "RMB ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    sprintf(UID_login_file_path, "USERS/%s/%s_login.txt", UID, UID);
    if (!file_exists(UID_login_file_path)) {
        char response[] = "RMB NLG\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    sprintf(dirname,"USERS/%s/BIDDED/", UID);
    n_entries = scandir(dirname, &filelist, NULL, alphasort);
    if (n_entries == 2) { // user has no bids
        free(filelist[0]); // "."
        free(filelist[1]); // ".."
        free(filelist);
        char response[] = "RMB NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else if (n_entries<=0)
        return;

    strcpy(buffer,"RMB OK");

    for (i = 0; i < n_entries; i++){
        len = strlen(filelist[i]->d_name);
        if (len == 7) {
            strcat(buffer," ");

            sscanf(filelist[i]->d_name,"%03d.txt",&AID);

            // Opens START file to see if auction has exceeded its duration
            sprintf(pathname,"AUCTIONS/%03d/START_%03d.txt",AID,AID);
            FILE *start_file = fopen(pathname, "r");
            if (start_file == NULL) {
                perror("Error opening the START file");
                return;
            }
            fgets(sfilecontents,start_size,start_file);
            sscanf(sfilecontents,"%s %s %s %d %d %s %s %ld",UID,name,fname,
                                &svalue,&timeactive,sdate,stime,&stime_seconds);
            fclose(start_file);

            current_time = time(NULL);
            end_sec_time = difftime(current_time,(time_t)stime_seconds);
            if (end_sec_time >= timeactive) { // Ends auction if it's duration has been exceeded
                end_sec_time = timeactive;
                current_time = stime_seconds + timeactive;
                end_auction(AID,end_sec_time,current_time);
            }

            sprintf(pathname,"AUCTIONS/%03d/END_%03d.txt",AID,AID);
            if (file_exists(pathname))
                sprintf(AID_state,"%03d 0", AID);
            else
                sprintf(AID_state,"%03d 1", AID);
            
            strcat(buffer, AID_state);
        }
        free(filelist[i]);
    }
    free(filelist);

    strcat(buffer,"\n");
    n=sendto(UDP_fd,buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
    if(n==-1) /*error*/ exit(1);

    return;
}

void list_command(char* buffer) {
    struct dirent **filelist;
    int AID, svalue, timeactive, n, n_entries, i, len;
    int start_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                     MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    long stime_seconds, end_sec_time;
    char sfilecontents[start_size];
    char UID[UID_LEN+1];
    char name[MAX_DESC_NAME_LEN+1], fname[MAX_FNAME_LEN+1];
    char sdate[DATE_LEN+1], stime[TIME_LEN+1];
    char dirname[10];
    char pathname[32];
    char AID_state[AID_LEN+3];
    time_t current_time;

    sprintf(dirname,"AUCTIONS/");
    n_entries = scandir(dirname, &filelist, NULL, alphasort);
    if (n_entries == 3) { // no auction has been started yet
        free(filelist[0]); // "."
        free(filelist[1]); // ".."
        free(filelist[2]); // ".gitkeep"
        free(filelist);
        char response[] = "RLS NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    } else if (n_entries<=0)
        return;

    strcpy(buffer,"RLS OK");

    for (i = 0; i < n_entries; i++){
        len = strlen(filelist[i]->d_name);
        if (len == 3) {
            strcat(buffer," ");
            
            sscanf(filelist[i]->d_name,"%03d",&AID);

            // Opens START file to see if auction has exceeded its duration
            sprintf(pathname,"AUCTIONS/%03d/START_%03d.txt",AID,AID);
            FILE *start_file = fopen(pathname, "r");
            if (start_file == NULL) {
                perror("Error opening the START file");
                return;
            }
            fgets(sfilecontents,start_size,start_file);
            sscanf(sfilecontents,"%s %s %s %d %d %s %s %ld",UID,name,fname,
                                &svalue,&timeactive,sdate,stime,&stime_seconds);
            fclose(start_file);

            current_time = time(NULL);
            end_sec_time = difftime(current_time,(time_t)stime_seconds);
            if (end_sec_time >= timeactive) { // Ends auction if it's duration has been exceeded
                end_sec_time = timeactive;
                current_time = stime_seconds + timeactive;
                end_auction(AID,end_sec_time,current_time);
            }

            sprintf(pathname,"AUCTIONS/%03d/END_%03d.txt",AID,AID);
            if (file_exists(pathname))
                sprintf(AID_state,"%03d 0", AID);
            else
                sprintf(AID_state,"%03d 1", AID);
            
            strcat(buffer, AID_state);
        }
        free(filelist[i]);
    }
    free(filelist);

    strcat(buffer,"\n");
    n=sendto(UDP_fd,buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
    if(n==-1) /*error*/ exit(1);

    return;
}

void bid_command(char* buffer, int new_fd) {
    struct dirent **filelist;
    char UID[UID_LEN+1];
    char password[PW_LEN+1];
    int AID, value, sscanf_ret, n;

    printf("buffer inside command: %s\n", buffer);

    sscanf_ret = sscanf(buffer,"BID %s %s %d %d",UID,password,&AID,&value);

    if (sscanf_ret != 4||!check_UID_format(UID)||!check_password_format(password)||!valid_AID(AID)||!valid_value(value)) {
        char response[] = "RBD ERR\n";
        n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
        if(n==-1)/*error*/exit(1);
        return;
    }
    char response[] = "ERR\n";
    n=write(new_fd,response,strlen(response)); // TODO: Do multiple writes to guarantee
    if(n==-1)/*error*/exit(1);
    return;
}

void show_record_command(char* buffer) {
    struct dirent **bidlist;
    int sscanf_ret, n, n_bids, len, AID, timeactive, bids_read = 0;
    char UID[UID_LEN+1];
    char dirname[13];
    char pathname[32];
    char auction_name[MAX_DESC_NAME_LEN+1];
    char auction_fname[MAX_FNAME_LEN+1];
    char value[MAX_VALUE_LEN+1]; // Used for the auction's start value and the bids' value
    char sdate[DATE_LEN+1]; // Used for the auction's start date and the bids' date
    char stime[TIME_LEN+1]; // Used for the auction's start time and the bids' time
    char duration_sec[MAX_DURATION_LEN+1]; // Used for the auction's time active and the bids' time
    int info_size = UID_LEN+MAX_DESC_NAME_LEN+MAX_FNAME_LEN+MAX_VALUE_LEN+
                    MAX_DURATION_LEN+DATE_LEN+TIME_LEN+MAX_FULLTIME+8;
    char info[info_size+1];
    long end_sec_time, stime_seconds;
    time_t current_time;

    sscanf_ret = sscanf(buffer, "SRC %03d", &AID);
    if (sscanf_ret != 1 || !valid_AID(AID)) {
        char response[] = "RRC ERR\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }

    sprintf(dirname,"AUCTIONS/%03d",AID);
    if (!dir_exists(dirname)) { // Auction doesn't exists
        char response[] = "RRC NOK\n";
        n=sendto(UDP_fd,response,strlen(response),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
        if(n==-1) /*error*/ exit(1);
        return;
    }
    sprintf(pathname,"AUCTIONS/%03d/START_%03d.txt",AID,AID);
    FILE *start_file = fopen(pathname, "r");
    if (start_file == NULL) {
        perror("Error opening the START file");
        return;
    }
    fread(info,1,info_size,start_file);
    fclose(start_file);

    strcpy(buffer,"RRC OK ");
    sscanf(info,"%s %s %s %s %s %s %s %ld",UID,auction_name,auction_fname,
            value,duration_sec,sdate,stime,&stime_seconds);
    sprintf(info,"%s %s %s %s %s %s %s",UID,auction_name,auction_fname,
            value,sdate,stime,duration_sec);
    strcat(buffer,info);

    current_time = time(NULL);
    end_sec_time = difftime(current_time,(time_t)stime_seconds);
    timeactive = atoi(duration_sec);
    if (end_sec_time >= timeactive) { // Ends auction if it's duration has been exceeded
        end_sec_time = timeactive;
        current_time = stime_seconds + timeactive;
        end_auction(AID,end_sec_time,current_time);
    }

    sprintf(pathname,"AUCTIONS/%03d/BIDS/",AID);
    n_bids = scandir(pathname, &bidlist, NULL, alphasort);
    if (n_bids<=0)
        return;
    while (n_bids--) {
        len = strlen(bidlist[n_bids]->d_name);
        if (len == 10 && bids_read < 50) {
            sprintf(pathname,"AUCTIONS/%03d/BIDS/%s",AID,bidlist[n_bids]->d_name);
            FILE *bid_file = fopen(pathname, "r");
            if (bid_file == NULL) {
                perror("Error opening a bid's file");
                return;
            }
            n = fread(info,1,info_size,bid_file);
            info[n] = '\0';
            strcat(buffer," B ");
            strcat(buffer,info);

            fclose(bid_file);
            bids_read++;
        }
        free(bidlist[n_bids]);
    }
    free(bidlist);

    sprintf(pathname,"AUCTIONS/%03d/END_%03d.txt",AID,AID);
    if (file_exists(pathname)) {
        FILE *end_file = fopen(pathname, "r");
        if (end_file == NULL) {
            perror("Error opening a bid's file");
            return;
        }
        n = fread(info,1,info_size,end_file);
        info[n] = '\0';

        strcat(buffer, " E ");
        strcat(buffer,info);
    }

    strcat(buffer,"\n");
    n=sendto(UDP_fd,buffer,strlen(buffer),0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
    if(n==-1) /*error*/ exit(1);
    return;
}


int main(int argc, char **argv) {
    pid_t pid;
    fd_set inputs, testfds;
    struct timeval timeout;

    int out_fds, ret, n, new_fd;

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
    
    init_signals();
    
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

    TCP_fd=socket(AF_INET,SOCK_STREAM,0);
    if (TCP_fd==-1) /*error*/ exit(1);

    memset(&TCP_hints,0,sizeof TCP_hints);
    TCP_hints.ai_family=AF_INET;//IPv4
    TCP_hints.ai_socktype=SOCK_STREAM;//TCP socket
    TCP_hints.ai_flags=AI_PASSIVE;
    
    errcode=getaddrinfo(NULL,ASport,&TCP_hints,&TCP_res);
    if (errcode!=0) /*error*/ exit(1);

    if(bind(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen) == -1)
        exit(1); //error
    
    if(listen(TCP_fd,5)==-1)/*error*/exit(1);

    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0,&inputs); // Set standard input channel on
    FD_SET(UDP_fd,&inputs); // Set UDP channel on
    FD_SET(TCP_fd,&inputs); // Set TCP channel on

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

                        if((pid=fork())==-1) /*error*/ exit(1);
                        else if (pid==0) { // Child process
                            if (strcmp(command, "LIN") == 0) {
                                login_command(buffer);
                            }
                            else if (strcmp(command, "LOU") == 0) {
                                logout_command(buffer);
                            }
                            else if (strcmp(command, "UNR") == 0) {
                                unregister_command(buffer);
                            }
                            else if (strcmp(command, "LMA") == 0) {
                                myauctions_command(buffer);
                            }
                            else if (strcmp(command, "LMB") == 0) {
                                mybids_command(buffer);
                            }
                            else if (strcmp(command, "LST") == 0) {
                                list_command(buffer);
                            }
                            else if (strcmp(command, "SRC") == 0) {
                                show_record_command(buffer);
                            } else {
                                char response[] = "ERR\n";
                                n=sendto(UDP_fd,response,4,0,(struct sockaddr*)&UDP_addr,sizeof(UDP_addr));
                                if(n==-1) /*error*/ exit(1);
                            }
                            exit(0); // Ends child process
                        }
                    }
                }
                if(FD_ISSET(TCP_fd,&testfds)) // Received from TCP
                {
                    TCP_addrlen = sizeof(TCP_addr);
                    if((new_fd=accept(TCP_fd,(struct sockaddr*)&TCP_addr,&TCP_addrlen))==-1)
                        /*error*/exit(1);
                    n=read(new_fd,buffer,BUFSIZE); // Read multiple times?
                    if(n==-1)/*error*/exit(1);

                    if(n>=0) {
                        if(strlen(buffer)>0)
                            buffer[n-1]=0;
                        
                        sscanf(buffer, "%s ", command);

                        if((pid=fork())==-1) /*error*/ exit(1);
                        else if (pid==0) { // Child process
                            if (strcmp(command, "OPA") == 0) {
                                open_command(buffer, new_fd, n);
                            }
                            else if (strcmp(command, "CLS") == 0) {
                                close_command(buffer, new_fd);
                            }
                            else if (strcmp(command, "SAS") == 0) {
                                show_asset_command(buffer, new_fd);
                            }
                            else if (strcmp(command, "BID") == 0) {
                                bid_command(buffer, new_fd);
                            } else {
                                strcpy(buffer, "ERR\n");
                                n = write(new_fd, buffer, strlen(buffer));
                                if (n == -1) /*error*/ exit(1);
                            }
                            close(new_fd);
                            exit(0); // Ends child process
                        }
                        do n=close(new_fd);while(n==-1&&errno==EINTR);
                        if(n==-1)/*error*/exit(1);
                    }
                }
        }
    }
    freeaddrinfo(TCP_res);
    freeaddrinfo(res);
    close(UDP_fd);
    close(TCP_fd);

    return 0;
}


/*
I'm pretty sure this is used for verbose mode

char host[NI_MAXHOST], service[NI_MAXSERV];
errcode=getnameinfo( (struct sockaddr *) &UDP_addr,addrlen,host,sizeof host, service,sizeof service,0);
if(errcode==0)
    printf("       Sent by [%s:%s]\n",host,service);
*/