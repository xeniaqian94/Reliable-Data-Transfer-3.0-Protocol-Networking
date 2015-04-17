#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define CPORT 59079
#define SPORT 59080

static float LOSS_RATE=0.0, ERR_RATE=0.0;

/* uncomment this part for part 1 */
#include "rdt-part1.h"
#define MSG_LEN PAYLOAD


/* uncomment this part for part 2 
#include "rdt-part2.h"
#define MSG_LEN PAYLOAD
*/

/* uncomment this part for part 3 
#include "rdt-part3.h"
#define MSG_LEN PAYLOAD*W
*/

int main(int argc, char *argv[]){

	int sockfd;
	FILE * testfile;
	int filelength, len;
	char * fname, * s;
	char msg[MSG_LEN];
	int sent = 0;
	struct timeval starttime, endtime;
	double lapsed;
	
	if (argc != 3) {
		printf("Usage: %s 'server hostname' 'filename'\n", argv[0]);
		exit(0);
	}	

	/* update random seed */
	srand(time(NULL));
	/* remove the above line if you want to get the same random 
	   sequence for each run - good for testing */
	
	/* read in packet loss rate and error rate */
	s = getenv("PACKET_LOSS_RATE");
	if (s != NULL) LOSS_RATE = strtof(s, NULL);
	s = getenv("PACKET_ERR_RATE");
	if (s != NULL) ERR_RATE = strtof(s, NULL);
	printf("PACKET_LOSS_RATE = %.2f, PACKET_ERR_RATE = %.2f\n", LOSS_RATE, ERR_RATE);

	fname=argv[2];
	//open file
	if (!(testfile = fopen(fname, "r"))) {
		printf("Open file failed.\nProgram terminated.");
		exit(0);
	}
	printf("Open file successfully \n");
	//get the file size
	fseek(testfile, 0L, SEEK_END); 
	filelength = ftell(testfile); 	
	printf("File bytes are %d \n",filelength);
	fseek(testfile, 0L, SEEK_SET);
		
	// create RDT socket
    sockfd = rdt_socket();
    
    //specify my own IP address & port number, because if I do not specify, others can not send things to me.
	rdt_bind(sockfd, CPORT);     
    
	//specify the IP address & port number of my partner 
    rdt_target(sockfd, argv[1], SPORT);

    /* a very simple handshaking protocol */ 
    //send the size of the file
    memset(msg, '\0', MSG_LEN);
    sprintf(msg, "%d", filelength);
    rdt_send(sockfd, msg, strlen(msg));  
      
    //send the file name to server
    rdt_send(sockfd, fname, strlen(fname));
    
	//wait for server response
    memset(msg, '\0', MSG_LEN);
	len = rdt_recv(sockfd, msg, MSG_LEN);
	if (strcmp(msg, "ERROR") == 0) {
		printf("Server experienced fatal error.\nProgram terminated.\n");
		goto END;
	} else
		printf("Receive server response\n");

	/* start the data transfer */
	printf("Start the file transfer . . .\n");
	gettimeofday(&starttime, NULL);
    // send the file contents
    while (sent < filelength) {
    	if ((filelength-sent) < MSG_LEN)
    		len = fread(msg, sizeof(char), filelength-sent, testfile);
    	else
    		len = fread(msg, sizeof(char), MSG_LEN, testfile);
    	rdt_send(sockfd, msg, len);
    	sent += len;
		usleep(1000);
    }
	gettimeofday(&endtime, NULL);
	printf("Complete the file transfer.\n");
	lapsed = (endtime.tv_sec - starttime.tv_sec)*1.0 + (endtime.tv_usec - starttime.tv_usec)/1000000.0;
	printf("Total elapse time: %.3f s\tThroughtput: %.2f KB/s\n", lapsed, filelength/lapsed/1000.0);
	
END:    
    // close the file
    fclose(testfile);
    
    // close the rdt socket
    rdt_close(sockfd);
	printf("Client program terminated\n");

    return 0;
}
/**************************************************************
 rdt-part1.h
 Student name:
 Student No. :
 Date and version:
 Development platform:
 Development language:
 Compilation:
	Can be compiled with
 *****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif
/**************************************************************
 rdt-part1.h
 Student name:
 Student No. :
 Date and version:
 Development platform:
 Development language:
 Compilation:
	Can be compiled with
 *****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif
/**************************************************************
 rdt-part1.h
 Student name:
 Student No. :
 Date and version:
 Development platform:
 Development language:
 Compilation:
	Can be compiled with
 *****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif
/**************************************************************
 rdt-part1.h
 Student name:
 Student No. :
 Date and version:
 Development platform:
 Development language:
 Compilation:
	Can be compiled with
 *****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif/**************************************************************
rdt-part1.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
Can be compiled with
*****************************************************************/

#ifndef RDT1_H
#define RDT1_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#define PAYLOAD 1000	//size of data payload of the RDT layer

//----- Type defines --------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
 return	-> the socket descriptor on success, -1 on error
 */
int rdt_socket() {
    return socket(AF_INET, SOCK_DGRAM, 0);
    
}

/* Application process calls this function to specify the IP address
 and port number used by itself and assigns them to the RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_bind(int fd, u16b_t port){
    struct sockaddr_in my_addr;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);
    memset(&(my_addr.sin_zero), 0, 8);
    
    return bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
}

/* Application process calls this function to specify the IP address
 and port number used by remote process and associates them to the
 RDT socket.
 return	-> 0 on success, -1 on error
 */
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET;
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1staddress of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct
    
    return connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr));
}

/* Application process calls this function to transmit a message to
 target (rdt_target) remote process through RDT socket.
 msg		-> pointer to the application's send buffer
 length	-> length of application message
 return	-> size of data sent on success, -1 on error
 */
int rdt_send(int fd, char * msg, int length){
    
    return send(fd,msg,length,0);
}

/* Application process calls this function to wait for a message
 from the remote process; the caller will be blocked waiting for
 the arrival of the message.
 msg		-> pointer to the receiving buffer
 length	-> length of receiving buffer
 return	-> size of data received on success, -1 on error
 */
int rdt_recv(int fd, char * msg, int length){
    
    return recv(fd,msg,length,0);
    
}

/* Application process calls this function to close the RDT socket.
 */
int rdt_close(int fd){
    return close(fd);
    
}

#endif

