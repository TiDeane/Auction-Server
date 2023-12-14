#include "utils.h"

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

char UID_current[UID_LEN+1];
char password_current[PW_LEN+1];
bool logged_in = false;

int user_recvfrom(char* buffer) {
	char in_str[64];
	struct timeval timeout;
	memset((void *)&timeout,0,sizeof(timeout));
	timeout.tv_sec = 10;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(UDP_fd, &readfds);
	FD_SET(0, &readfds);
	int maxfd = (UDP_fd > 0) ? (UDP_fd + 1) : 1;

	while (1) {
		int out_fds = select(maxfd, &readfds, NULL, NULL, &timeout);
		if (out_fds == -1) {
			perror("select");
			return -1;
		} else if (out_fds == 0) {
			printf("Timeout occurred. No response received from server.\n");
			return 0;
		} else {
			if (FD_ISSET(UDP_fd, &readfds)) {
				int n = recvfrom(UDP_fd, buffer, BUFSIZE, 0, (struct sockaddr*)&addr, &addrlen);
				if (n == -1) {
					perror("recvfrom");
					return -1;
				}
				return n;
			} else if (FD_ISSET(0, &readfds)) {
				fgets(in_str, sizeof(in_str), stdin);
				printf("---Input at keyboard: %s", in_str);
				return -2;
			}
		}
	}
	return 0;
}

/* Logs into the Auction Server. Command format is "login UID password" */
void login_command(char* buffer) {
	char UID[UID_LEN+1];
	char password[PW_LEN+1];
	char LIN_command[STATUS_LEN+UID_LEN+PW_LEN+4];

	if (logged_in) {
		printf("ERR: user is already logged in\n");
		return;
	}

	if (sscanf(buffer,"login %s %s\n",UID,password) != 2) {
		printf("ERR: Incorrect number of arguments\n");
		return;
	} else if (!check_UID_format(UID)) {
		printf("ERR: UID must be a 6 digit number\n");
		return;
	} else if (!check_password_format(password)) {
		printf("ERR: Password must consist of 8 alphanumerical characters\n");
		return;
	}

	snprintf(LIN_command, sizeof(LIN_command), "LIN %s %s\n", UID, password);

	n=sendto(UDP_fd,LIN_command,strlen(LIN_command),0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

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
	else if (strncmp(buffer,"RLI ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Logs out of the Auction Server. Command format is "logout" */ 
void logout_command() {
	if (!logged_in) {
		printf("ERR: user must be logged in\n");
		return;
	}

	char LOU_command[STATUS_LEN+UID_LEN+PW_LEN+4];
	snprintf(LOU_command, sizeof(LOU_command), "LOU %s %s\n", UID_current, password_current);

	n=sendto(UDP_fd,LOU_command,strlen(LOU_command),0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

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
	else if (strncmp(buffer,"RLO ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Unregisters user from the Auction Server. Command format is "unregister" */ 
void unregister_command(){
	if(!logged_in){
		printf("ERR: user must be logged in\n");
		return;
	}

	char UNR_Command[STATUS_LEN+UID_LEN+PW_LEN+4];
	snprintf(UNR_Command, sizeof(UNR_Command), "UNR %s %s\n", UID_current, password_current);

	n=sendto(UDP_fd,UNR_Command,strlen(UNR_Command),0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

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
	else if (strncmp(buffer,"RUR ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Opens a new auction. Command format is "open name asset_fname start_value timeactive" */
void open_command(char* buffer) {
	char status[STATUS_LEN+1];
	char name[MAX_DESC_NAME_LEN+1];
	char asset_fname[MAX_FNAME_LEN+1];
	int AID, command_length, start_value, timeactive;
	long Fsize;

	if (!logged_in) {
		printf("ERR: user must be logged in\n");
		return;
	}

	if (sscanf(buffer,"open %s %s %d %d\n",name,asset_fname,&start_value,&timeactive) != 4) {
		printf("ERR: Incorrect number of arguments\n");
		return;
	} else if (!check_desc_name_format(name)) {
		printf("ERR: Invalid description name\n");
		return;
	} else if (!check_fname_format(asset_fname)) {
		printf("ERR: Invalid file name\n");
		return;
	} else if (!valid_value(start_value)) {
		printf("ERR: Invalid start value\n");
		return;
	} else if (!valid_timeactive(timeactive)) {
		printf("ERR: Invalid auction duration\n");
		return;
	}

	FILE *file = fopen(asset_fname, "rb");
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
	Fsize = file_info.st_size;
	
	if (!valid_filesize(Fsize)) {
		printf("ERR: File is too large\n");
		fclose(file);
		return;
	}

	command_length = sprintf(buffer, "OPA %s %s %s %d %d %s %ld ",
								  UID_current, password_current, name, start_value,
								  timeactive, asset_fname, Fsize);

	TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if (TCP_fd==-1) exit(1); //error

	memset(&TCP_hints,0,sizeof TCP_hints);
	TCP_hints.ai_family=AF_INET; //IPv4
	TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

	errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
	if(errcode!=0)/*error*/exit(1);

	n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
	if(n==-1)/*error*/exit(1);

	n=write_TCP(TCP_fd,buffer,command_length);
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

	n=read_TCP(TCP_fd,buffer);
	if(n==-1)/*error*/exit(1);

	fclose(file);
	freeaddrinfo(TCP_res);
	close(TCP_fd);

	if (sscanf(buffer, "ROA %s %d\n", status, &AID) == 0) {
		printf("ERR: Invalid response from server\n");
		return;
	}

	if (strcmp(status, "OK") == 0) {
		printf("Auction [%03d] successfully created\n", AID);
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
	else if (strcmp(status,"ERR\n") == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Closes an ongoing auction. Command format is "close AID" */
void close_command(char* buffer) {
	char CLS_command[STATUS_LEN+UID_LEN+PW_LEN+AID_LEN+5];
	int AID;

	if (!logged_in) {
		printf("User must be logged in\n");
		return;
	}

	if (sscanf(buffer, "close %d", &AID) != 1) {
		printf("ERR: Incorrect number of arguments\n");
		return;
	} else if (!valid_AID(AID)) {
		printf("ERR: Invalid AID\n");
		return;
	}

	sprintf(CLS_command, "CLS %s %s %03d\n", UID_current, password_current, AID);

	TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if (TCP_fd==-1) exit(1); //error

	memset(&TCP_hints,0,sizeof TCP_hints);
	TCP_hints.ai_family=AF_INET; //IPv4
	TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

	errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
	if(errcode!=0)/*error*/exit(1);

	n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
	if(n==-1)/*error*/exit(1);

	n=write_TCP(TCP_fd,CLS_command,strlen(CLS_command));
	if(n==-1)/*error*/exit(1);

	n=read_TCP(TCP_fd,buffer);
	if(n==-1)/*error*/exit(1);

	freeaddrinfo(TCP_res);
	close(TCP_fd);

	if(strncmp(buffer,"RCL OK\n", 7) == 0){
		printf("Auction was successfully closed\n");
		return;
	}
	else if(strncmp(buffer,"RCL EAU\n", 8) == 0){
		printf("The auction %03d does not exist\n",AID);
		return;
	}
	else if(strncmp(buffer,"RCL EOW\n", 8) == 0){
		printf("You do not own this auction\n");
		return;
	}
	else if(strncmp(buffer,"RCL END\n", 8) == 0){
		printf("The auction has already ended\n");
		return;
	}
	else if(strncmp(buffer,"RCL NLG\n", 8) == 0){
		printf("User was not logged into the Auction Server\n");
		return;
	}
	else if (strncmp(buffer,"RCL ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Lists the current user's auctions. Format is "myauctions" or "ma" */ 
void myauctions_command(){
	char LMA_Command[STATUS_LEN+UID_LEN+3];
	int command_length;

	if(!logged_in){
		printf("ERR: must login first\n");
		return;
	}
	
	command_length = sprintf(LMA_Command, "LMA %s\n", UID_current);
	n=sendto(UDP_fd,LMA_Command,command_length,0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

	if (buffer[n-1]=='\n')
		buffer[n]='\0';
	else
		return; // error
	
	if(strncmp(buffer,"RMA OK",6)==0){
		printf("My auctions:\n%s", buffer + 7);
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
	else if (strncmp(buffer,"RMA ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Lists the current user's bids. Command format is "mybids" or "mb" */
void mybids_command(){
	char LMB_Command[STATUS_LEN+UID_LEN+3];
	int command_length;

	if(!logged_in){
		printf("ERR: must login first\n");
		return;
	}

	command_length = sprintf(LMB_Command, "LMB %s\n", UID_current);
	n=sendto(UDP_fd,LMB_Command,command_length,0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

	if (buffer[n-1]=='\n')
		buffer[n]='\0';
	else
		return; // error

	if(strncmp(buffer,"RMB OK",6)==0){
		printf("My bids:\n%s", buffer + 7);
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
	else if (strncmp(buffer,"RMB ERR\n",8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Lists all currently active auctions. Command format is "list" or "l" */ 
void list_command() {

	char LST_command[] = "LST\n";
	n=sendto(UDP_fd,LST_command,strlen(LST_command),0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

	if (buffer[n-1]=='\n')
		buffer[n]='\0';
	else
		return; // error

	if(strncmp(buffer,"RLS OK", 6) == 0){
		printf("List of auctions:\n%s", buffer + 7);
		return;
	}
	else if(strncmp(buffer,"RLS NOK", 7) == 0){
		printf("No auctions have been started\n");
		return;
	}
	else if (strncmp(buffer,"RLS ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Shows an auction's asset. Command format is "show_asset AID" or "sa AID" */
void show_asset_command(char* buffer) {
	char SAS_command[STATUS_LEN+AID_LEN+3];
	char status[STATUS_LEN+1];
	char fname[MAX_FNAME_LEN+1];
	char *ptr;
	int AID, sscanf_ret, data_start, fsize_len, fname_len;
	long fsize;
	ssize_t nleft,nwritten,nread,data_received;
	
	sscanf_ret = sscanf(buffer, "sa %3d", &AID);
	if (sscanf_ret == 0)
		sscanf_ret = sscanf(buffer, "show_asset %3d", &AID);

	if (sscanf_ret == 0 || !valid_AID(AID)) {
		printf("ERR: Invalid AID\n");
		return;
	}

	sprintf(SAS_command, "SAS %03d\n", AID);
	
	TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if (TCP_fd==-1) exit(1); //error

	memset(&TCP_hints,0,sizeof TCP_hints);
	TCP_hints.ai_family=AF_INET; //IPv4
	TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

	errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
	if(errcode!=0)/*error*/exit(1);

	n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
	if(n==-1)/*error*/exit(1);

	n=write_TCP(TCP_fd,SAS_command,strlen(SAS_command));
	if(n==-1)/*error*/exit(1);

	nread=read(TCP_fd,buffer,BUFSIZE);
	if(nread==-1)/*error*/exit(1);

	sscanf_ret = sscanf(buffer, "RSA %s %s %ld", status, fname, &fsize);

	if (sscanf_ret == 0 || strcmp(status, "NOK") == 0) {
		printf("There is no file to be sent, or a problem ocurred\n");
		close(TCP_fd);
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
	else if (strcmp(status,"ERR\n") == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strcmp(status, "OK") == 0) {

		if (sscanf_ret == 1) { // Only status was read
			nread = read(TCP_fd, buffer, BUFSIZE);
			sscanf_ret = sscanf(buffer, "%s %ld", fname, &fsize);

			fsize_len = snprintf(NULL, 0, "%ld", fsize);
			fname_len = strlen(fname);
			data_start = fname_len + fsize_len + 2;
		}
		else {
			fsize_len = snprintf(NULL, 0, "%ld", fsize);
			fname_len = strlen(fname);
			data_start = fname_len + fsize_len + 9;
		}
		ptr = buffer;
		ptr += data_start; // Points to start of data

		if (!check_fname_format(fname) || !valid_filesize(fsize)) {
			printf("There is no file to be sent, or a problem ocurred\n");
			close(TCP_fd);
			return;
		}

		FILE *file = fopen(fname, "wb");
		if (file == NULL) {
			perror("Error opening the file");
			close(TCP_fd);
			return;
		}

		if (fsize > (nread - data_start))
			data_received = nread - data_start;
		else
			data_received = fsize;

		if (data_received>0)
			nwritten = fwrite(ptr, 1, data_received, file);
		else
			nwritten = 0;

		nleft = fsize - nwritten;

		while (nleft > 0){
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

		getcwd(buffer, BUFSIZE);
		printf("File [%s] of size [%ldB] stored at %s\n", fname,fsize,buffer);

		return;
	}
}

/* Places a bid on an auctions. Command format is "bid AID value" or "b AID value" */
void bid_command(char* buffer) {
	int AID, value, sscanf_ret, command_len;

	if(!logged_in){
		printf("ERR: must login first\n");
		return;
	}

	sscanf_ret = sscanf(buffer, "b %3d %d", &AID, &value);
	if (sscanf_ret != 2)
		sscanf_ret = sscanf(buffer, "bid %3d %d", &AID, &value);

	if (sscanf_ret != 2) {
		printf("ERR: Invalid AID or value\n");
		return;
	}

	if (!valid_AID(AID)) {
		printf("ERR: Invalid AID\n");
		return;
	}
	else if (!valid_value(value)) {
		printf("ERR: Invalid value\n");
		return;
	}

	char BID_command[STATUS_LEN+UID_LEN+PW_LEN+AID_LEN+MAX_VALUE_LEN+6];
	command_len = sprintf(BID_command, "BID %s %s %03d %d\n",UID_current, password_current, AID, value);

	TCP_fd=socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if (TCP_fd==-1) exit(1); //error

	memset(&TCP_hints,0,sizeof TCP_hints);
	TCP_hints.ai_family=AF_INET; //IPv4
	TCP_hints.ai_socktype=SOCK_STREAM; //TCP socket

	errcode=getaddrinfo(ASIP,ASport,&TCP_hints,&TCP_res);
	if(errcode!=0)/*error*/exit(1);

	n=connect(TCP_fd,TCP_res->ai_addr,TCP_res->ai_addrlen);
	if(n==-1)/*error*/exit(1);

	n=write_TCP(TCP_fd,BID_command,command_len);
	if(n==-1)/*error*/exit(1); 

	n=read_TCP(TCP_fd,buffer);
	if(n==-1)/*error*/exit(1);

	if (strncmp(buffer, "RBD NOK\n", 8) == 0) {
		printf("Auction %03d is not active\n", AID);
		close(TCP_fd);
		return;
	}
	else if (strncmp(buffer, "RBD ACC\n", 8) == 0) {
		printf("Bid was accepted\n");
		close(TCP_fd);
		return;
	}
	else if (strncmp(buffer, "RBD REF\n", 8) == 0) {
		printf("Bid was refused: a larger (or equal) bid was previously placed\n");
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
	else if (strncmp(buffer,"RBD ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

/* Shows the record of the given auction. Format is "show_asset AID" or "sa AID" */
void show_record_command(char *buffer){
	char SRC_command[STATUS_LEN+AID_LEN+3];
	int AID, i, sscanf_ret;

	sscanf_ret = sscanf(buffer, "sr %3d\n", &AID);
	if (sscanf_ret != 1)
		sscanf_ret = sscanf(buffer, "show_record %3d\n", &AID);

	if (sscanf_ret != 1 || !valid_AID(AID)) {
		printf("ERR: Invalid AID\n");
		return;
	}
	
	sprintf(SRC_command, "SRC %03d\n", AID);

	n=sendto(UDP_fd,SRC_command,strlen(SRC_command),0,res->ai_addr,res->ai_addrlen);
	if(n==-1) /*error*/ exit(1);

	addrlen=sizeof(addr);
	n=user_recvfrom(buffer);
	if(n==-1) /*error*/ exit(1);
	else if(n==0||n==-2) return; // Timeout or keyboard input

	if (buffer[n-1]=='\n')
		buffer[n]='\0';
	else
		return; // error

	if (strncmp(buffer, "RRC NOK\n",8) == 0){
		printf("Auction %03d does not exist\n", AID);
		return;
	}
	else if(strncmp(buffer,"RRC OK",6) == 0){
		for (i = 1; i < n; i++) {
			if (buffer[i-1] == ' ' && (buffer[i] == 'B'|| buffer[i] == 'E'))
				buffer[i-1] = '\n';
		}

		printf("%s",buffer + 7);
		return;
	}
	else if (strncmp(buffer,"RCL ERR\n", 8) == 0){
		printf("Incorrect or badly formatted arguments received by server\n");
		return;
	}
	else if (strncmp(buffer,"ERR\n", 4) == 0){
		printf("Unexpected protocol message received by server\n");
		return;
	}
}

int main(int argc, char **argv) {
	char command[12];

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
		if (sscanf(buffer, "%s", command) == 0)
			continue;

		if (strcmp(command, "login") == 0) {
			login_command(buffer);
		}
		else if (strcmp(command, "logout") == 0) {
			logout_command();
		}
		else if (strcmp(command, "unregister") == 0) {
			unregister_command();
		}
		else if (strcmp(command, "exit") == 0) {
			// Exit command
			if (logged_in == true) {
				printf("You need to logout before exiting!\n");
				continue;
			}
			break;
		}
		else if (strcmp(command, "open") == 0) {
			open_command(buffer);
		}
		else if (strcmp(command, "close") == 0) {
			close_command(buffer);
		}
		else if ((strcmp(command, "myauctions") == 0 || strcmp(command, "ma") == 0)) {
			myauctions_command(buffer);
		}
		else if ((strcmp(command, "mybids") == 0 || strcmp(command, "mb") == 0)) {
			mybids_command();
		}
		else if ((strcmp(command, "list") == 0 || strcmp(command, "l") == 0)) {
			list_command();
		}
		else if ((strcmp(command, "show_asset") == 0 || strcmp(command, "sa") == 0)) {
			show_asset_command(buffer);
		}
		else if ((strcmp(command, "bid") == 0 || strcmp(command, "b") == 0)) {
			bid_command(buffer);
		}
		else if ((strcmp(command, "show_record") == 0 || strcmp(command, "sr") == 0)) {
			show_record_command(buffer);
		}
		else
			printf("ERR: Invalid command\n");
	}

	printf("successfully exited\n");

	freeaddrinfo(res);
	close(UDP_fd);

	return 0;
}