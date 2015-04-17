/**************************************************************
rdt-part1.h
Student name: Qian Xin
Student No. : 3035134147
Date and version: Feb.16th, 2015
Development platform: Ubuntu 14.04 LTE Virtual Machine
Development language:C
Compilation: gcc
	Can be compiled with 
 
 
 To compile, please use commands:
 •gcc test_client.c –Wall –o tclient
 •gcc test_server.c –Wall –o tserver
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
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    return sockfd;

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

    int sockbind;
    if ((sockbind=bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)) {
        perror("bind1");
        return -1;
    }
    return 0;
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the 
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char* peer_name, u16b_t peer_port){
    
    struct hostent* he = gethostbyname(peer_name); // lookup peer_name
    
    if (he == NULL) {  // get the host info
        perror("gethostbyname");
        return -1;
    }
    
    
    struct sockaddr_in peer_addr; // fill up the sockaddr_in structure for connecting to peername:peer_port
    peer_addr.sin_family= AF_INET; //which protocol family for addressing format
    peer_addr.sin_port= htons(peer_port);
    peer_addr.sin_addr= *((struct in_addr*)he->h_addr); // the 1st address of this host
    memset(&(peer_addr.sin_zero), 0, 8); // zero the rest of the struct

    if (connect(fd,(struct sockaddr *)&peer_addr,sizeof(struct sockaddr)) == -1) {
        perror("target");
        return -1;
    }
    return 0;
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
   length	-> length of receiving, buffer size of the buffer
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
