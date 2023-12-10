#include "utils.h"

/* Checks if UID is a 6 digit number */
bool check_UID_format(char* UID) {
    if (UID == NULL || strlen(UID) != UID_LEN) {
        printf("ERR: UID must be a 6-digit number\n");
        return false;
    }
    
    for (int i = 0; i < UID_LEN; i++) {
        if (!isdigit(UID[i])) {
            printf("ERR: UID must be a 6-digit number\n");
            return false;
        }
    }
    
    return true;
}

/* Checks if password is composed of 8 alphanumeric characters */
bool check_password_format(char* password) {
    if (password == NULL || strlen(password) != PW_LEN) {
        printf("ERR: Password must be composed of 8 alphanumeric characters\n");
        return false;
    }
    
    for (int i = 0; i < PW_LEN; i++) {
        if (!isalnum(password[i])) {
            printf("ERR: Password must be composed of 8 alphanumeric characters\n");
            return false;
        }
    }
    
    return true;
}

bool check_AID_format(char* AID) {
    if (AID == NULL)
        return false;

    int AID_len = strlen(AID);
    if (AID_len != AID_LEN)
        return false;
    
    for (int i = 0; i < AID_len; i++) 
        if (!isdigit(AID[i]))
            return false;
    
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

bool check_value_format(char* value) {
    if (value == NULL)
        return false;

    int value_len = strlen(value);
    if (value_len > MAX_VALUE_LEN)
        return false;
    
    for (int i = 0; i < value_len; i++) 
        if (!isdigit(value[i]))
            return false;
    
    return true;
}

bool check_timeactive_format(char* timeactive) {
    if (timeactive == NULL)
        return false;

    int timeactive_len = strlen(timeactive);
    if (timeactive_len > MAX_DURATION_LEN)
        return false;
    
    for (int i = 0; i < timeactive_len; i++) 
        if (!isdigit(timeactive[i]))
            return false;
    
    return true;
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