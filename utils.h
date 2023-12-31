#ifndef UTILS_H
#define UTILS_H

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
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define PORT "58002"

#define BUFSIZE 6010

#define STATUS_LEN 3
#define AID_LEN 3
#define UID_LEN 6
#define PW_LEN 8
#define MAX_DESC_NAME_LEN 10
#define MAX_VALUE_LEN 6
#define MAX_FNAME_LEN 24
#define MAX_DURATION_LEN 5
#define DATE_LEN 10
#define TIME_LEN 8
#define MAX_FULLTIME 10
#define BID_LEN 6
#define MAX_AUCTIONS 999
#define MAX_FILESIZE 99999999
#define MAX_VALUE 999999
#define MAX_DURATION 99999

#define UID_DIR_PATH_LEN (UID_LEN+6) // USERS/(uid)
#define UID_HOST_DIR_PATH_LEN (UID_DIR_PATH_LEN+7) // USERS/(uid)/HOSTED
#define UID_BID_DIR_PATH_LEN (UID_DIR_PATH_LEN+7) // USERS/(uid)/BIDDED
#define UID_LOGIN_FILE_LEN (UID_DIR_PATH_LEN+1+UID_LEN+10) // USERS/(uid)/(uid)_login.txt
#define UID_PASS_FILE_LEN (UID_DIR_PATH_LEN+1+UID_LEN+9) // USERS/(uid)/(uid)_pass.txt

bool check_UID_format(char* UID);
bool check_password_format(char* password);
bool check_desc_name_format(char* name);
bool check_fname_format(char* fname);
bool valid_AID(int AID);
bool valid_value(int value);
bool valid_timeactive(int timeactive);
bool valid_filesize(long fsize);

bool file_exists (char *filename);
bool dir_exists (char *dirpath);

int read_TCP(int TCP_fd, char* buffer);
int write_TCP(int TCP_fd, char* buffer, int command_len);
int sendto_user(int UDP_fd, char* message, struct addrinfo *res);
int recvfrom_user(int UDP_fd, char* buffer, struct sockaddr_in addr, socklen_t addrlen);
int sendto_server(int UDP_fd, char* buffer, struct sockaddr_in UDP_addr);

#endif /* UTILS_H */
