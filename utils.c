#include "utils.h"

/* Checks if UID is a 6 digit number */
bool check_UID_format(char* UID) {
    if (UID == NULL || strlen(UID) != UID_LEN) {
        return false;
    }
    
    for (int i = 0; i < UID_LEN; i++) {
        if (!isdigit(UID[i])) {
            return false;
        }
    }
    
    return true;
}

/* Checks if password is composed of 8 alphanumeric characters */
bool check_password_format(char* password) {
    if (password == NULL || strlen(password) != PW_LEN) {
        return false;
    }
    
    for (int i = 0; i < PW_LEN; i++) {
        if (!isalnum(password[i])) {
            return false;
        }
    }
    
    return true;
}

bool check_desc_name_format(char* name) {
    if (name == NULL)
        return false;

    int name_len = strlen(name);
    if (name_len > MAX_DESC_NAME_LEN)
        return false;
    
    for (int i = 0; i < name_len; i++) 
        if (!isalnum(name[i]))
            return false;
            
    return true;
}

bool check_fname_format(char* fname) {
    if (fname == NULL)
        return false;

    int fname_len = strlen(fname);
    if (fname_len > MAX_FNAME_LEN)
        return false;
    
    for (int i = 0; i < fname_len; i++) 
        if (!isalnum(fname[i]) && fname[i] != '-' && fname[i] != '_' && fname[i] != '.')
            return false;
    
    return true;
}

bool valid_AID(int AID) {
    return (AID <= MAX_AUCTIONS);
}

bool valid_value(int value) {
    return (value <=MAX_VALUE);
}

bool valid_timeactive(int timeactive) {
    return (timeactive <= 99999);
}

bool valid_filesize(long fsize) {
    return (fsize <= MAX_FILESIZE);
}

bool file_exists (char *filename) {
  struct stat info;   
  return (stat (filename, &info) == 0);
}

bool dir_exists (char *dirpath) {
    struct stat info;
    return (stat(dirpath, &info) == 0 && S_ISDIR(info.st_mode));
}