#include "utils.h"

/* Checks if UID is a 6 digit number */
bool check_UID_format(char* UID) {
    if (UID == NULL || strlen(UID) != 6) {
        printf("ERR: UID must be a 6-digit number\n");
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        if (!isdigit(UID[i])) {
            printf("ERR: UID must be a 6-digit number\n");
            return false;
        }
    }
    
    return true;
}

/* Checks if password is composed of 8 alphanumeric characters */
bool check_password_format(char* password) {
    if (password == NULL || strlen(password) != 8) {
        printf("ERR: Password must be composed of 8 alphanumeric characters\n");
        return false;
    }
    
    for (int i = 0; i < 8; i++) {
        if (!isalnum(password[i])) {
            printf("ERR: Password must be composed of 8 alphanumeric characters\n");
            return false;
        }
    }
    
    return true;
}

bool check_fname_format(char* fname) {
    int fname_len = strlen(fname);
    if (fname == NULL || fname_len > 24)
        return false;
    
    for (int i = 0; i < fname_len; i++) 
        if (!isalnum(fname[i]) && fname[i] != '-' && fname[i] != '_' && fname[i] != '.')
            return false;
    
    return true;
}

bool file_exists (char *filename) {
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}