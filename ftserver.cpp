/* 
 Barbara Hazlett
 CS 372 
 3/3/2015 
 Program #2 - client-server network application
 
 This program is the server.  It takes a port# on the command line,
 creates a socket and binds to it.  It then listens for connections and
 connects when one is found (from the client).  Then based on the command
 from the client it either sends a directory listing or a file on the data
 port which was sent from the client.

 Ihis program heavily uses Beej's stream socket server demo found at:
 http://beej.us/guide/bgnet/output/html/multipage/clientserver.html#simpleserver
  
 Other help sources:
 http://en.cppreference.com/w/c/io/fscanf
 https://www.classes.cs.uchicago.edu/archive/1999/winter/CS219/projects/project1/project1.html
 https://github.com/KamalChaya/ftp-with-socket-programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>

#define BACKLOG 10     // how many pending connections queue will hold

#define MAX_MESSAGE 50  // maximum message size

using namespace std;

//sighandler for child 
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

//sighandler for control C
void sighandler(int s)
{
    printf("Server: exiting\n");
	exit(1);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//get size of a file
long FdGetFileSize(int fd)
{
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
/****************************************************************************************
 ** Function: start_up
 ** Description: Creates the connection socket
 ** Input Parameters: portNum
 ** Output Parameter: sockfd
 ***************************************************************************************/
int startup(char *portNum) {
	int sockfd;  // listen on sock_fd
    struct addrinfo hints, *servinfo, *p;
    int yes=1;    
    int rv;	
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
	
	 if ((rv = getaddrinfo(NULL, portNum, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    
	return sockfd;
}

/****************************************************************************************
 ** Function: createDataConnection
 ** Description: Creates the data socket 
 ** Input Parameters: portNum, hostName
 ** Output Parameter: datafd
 ***************************************************************************************/
int createDataConnection(char * portNum, char * hostName) {
	int datafd;  // listen on sock_fd
    struct addrinfo hints, *servinfo, *p;
    int rv;	
	//char * PORT = "30051";
	
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE; // use my IP	
	
	if ((rv = getaddrinfo(hostName, portNum, &hints, &servinfo)) != 0) { //try flip.engr. ...  here
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
	
	// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((datafd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(datafd, p->ai_addr, p->ai_addrlen) == -1) {
            close(datafd);
            perror("client: connect");
            continue;
        }

        break;
    }
    //for testing
	//send(datafd,"sending on data connection end",40,0);
    
	return datafd;
}
/****************************************************************************************
 ** Function: sendList
 ** Description: sends the directory to the client
 ** Input Parameters: datafd
 ***************************************************************************************/
void sendList(int datafd) {
    char buffer[500];
	int fd;
	int nbytes;	
	long size;
	
	//put directory contents in a file
	system( "dir > file_names.txt" );
	
	//open file, get size and read
	fd = open("file_names.txt", O_RDONLY, 0);
	if (fd < 0)
		return;
	size = FdGetFileSize(fd);
	nbytes = read(fd,buffer,size);
	
	//send directory
	send(datafd, buffer, size,0);   
	printf("List directory requested.\n");
	printf("Sending directory contents to client.\n");
	
	close(fd);	
	close(datafd);	
}
 
/****************************************************************************************
 ** Function: sendFile
 ** Description: sends requested file to client
 ** Input Parameters: new_fd
 ***************************************************************************************/
 void sendFile(int datafd, char * filename){
	char buffer[1500];
	char message[100];
	int fd;
	int nbytes;
	long size;
	
	//open file, get size and send size to client
	fd = open(filename, O_RDONLY, 0);
	if (fd < 0) {
		send(datafd, "Error: FILE NOT FOUND", 21,0);
		printf("File not found");
		return;	
	}
	size = FdGetFileSize(fd);	
	nbytes = read(fd,buffer,size);
		
	if (size < 1500) {
		send(datafd, "small",5,0);		
		recv(datafd,message,8,0); 
		send(datafd, buffer, size,0);
		printf("File requested.\n");
		printf("Sending %s to client\n", filename);		
	}
	
	else
		printf("big file!");
	
	
	//while (nbytes > 0) {
   /* Read nbytes of data.  Do something with it */
	//	send(datafd, buffer, 512,0);
   /* read the next chunk of data */
		//nbytes = read(fd,buffer,512);
	//}*/

	close(fd);
	close(datafd);
 }
/****************************************************************************************
 ** Function: handleRequest
 ** Description: gets client command, parses and calls function to create data socket and either 
 ** list directory or get specified file
 ** Input Parameters: new_fd
 ***************************************************************************************/
 void handleRequest(int new_fd) {
	char client_command[MAX_MESSAGE];
	int cn;	
	char command[4];
	char dataNum[8];
	char hostName[25];
	char fileName[25];
	int datafd;
	int parse;
		
	//get command
	memset(client_command, 0, MAX_MESSAGE); //clear out message	
	cn = recv(new_fd,client_command,sizeof(client_command),0);  
	//strcpy(command, client_command);
	//cout << client_command << endl;	
     
    parse = sscanf(client_command, "%s%s%s%s", dataNum, command, hostName, fileName);
 		
	//send validation or error message to client 
	if ((strcmp(command, "list") == 0) || (strcmp(command, "get") == 0)){
		send(new_fd,"valid command received",22,0);
		datafd = createDataConnection(dataNum, hostName);  //(30051, "flip1.engr.oregonstate.edu");
	}	
	else
		send(new_fd,"error: bad command received",27,0);	
	
	//call function to send directory
	if ((strcmp(command, "list") == 0)){		
		sendList(datafd);
	}
	
	//call function to send requested file
	if ((strcmp(command, "get") == 0)){
		sendFile(datafd, fileName);		
	}	
	
	close(datafd);	
}	
 
int main(int argc, char*argv[])
{
    int sockfd;  // listen on sock_fd, new connection on new_fd
	int new_fd = 0;  
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;    
    char s[INET6_ADDRSTRLEN];    
	char *servNum;
	bool loop = true;
	  
	//check that port# is on command line 
	if (argc != 2) {
		printf("usage: ./ftserve <server-port>");
		exit(1);
	}	
	servNum = argv[1];
	
	//if user hits control C
	if(signal(SIGINT, sighandler) == SIG_ERR){
		perror("signal");
		close(new_fd);
	}
	
	//call function to create and bind to the control socket
	sockfd = startup(servNum);
   
    //handle sigchild exit
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");		
        exit(1);
    }

    printf("server: waiting for connections...\n");
	
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); //accept connection
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: open on %s\n", servNum);			

			
		if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
			while((loop = true)) {
			
				handleRequest(new_fd);
			
			}
			close(new_fd);
            exit(0);					
		}
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}


