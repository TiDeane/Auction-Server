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

#define MAX_FILENAME 24
#define UID_LEN 6
#define PW_LEN 8

#define UID_DIR_PATH_LEN (UID_LEN+6) // USERS/(uid)
#define UID_LOGIN_FILE_LEN (UID_DIR_PATH_LEN+1+UID_LEN+10) // USERS/(uid)/(uid)_login.txt
#define UID_PASS_FILE_LEN (UID_DIR_PATH_LEN+1+UID_LEN+9) // USERS/(uid)/(uid)_pass.txt

bool check_UID_format(char* UID);
bool check_password_format(char* password);
bool check_fname_format(char* fname);

#endif /* UTILS_H */
