/**************************************************************
rdt-part3.h
 Student name: Qian Xin
 Student No. : 3035134147
 Date and version: April 8, 2015, Version 1
 Development platform: Ubuntu 14.04 LTE Virtual Machine
 Development language: C
 Compilation: gcc
	Can be compiled with Mac OS X 10.10.2
*****************************************************************/

#ifndef RDT3_H
#define RDT3_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 1000		//size of data payload of the RDT layer
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving  
							//end for TWAIT time units before closing
							//For retransmission of missing last ACK
#define W 9					//For Extended S&W - define pipeline window size

//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

extern float LOSS_RATE, ERR_RATE;


/* this function is for calculating the 16-bit checksum of a message */
/***
 Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
 bytecount-how many bytes are there??
 ***/
u16b_t checksum(char* msg, u16b_t bytecount)
{
    u32b_t sum = 0;
    u16b_t * addr = (u16b_t *)msg;
    u16b_t word = 0;
    
    // add 16-bit by 16-bit
    while(bytecount > 1)
    {
        sum += *addr++;
        bytecount -= 2;
    }
    
    // Add left-over byte, if any
    if (bytecount > 0) {
        *(u8b_t *)(&word) = *(u8b_t *)addr;
        sum += word;
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    word = ~sum;
    
    return word;
}
void print_packet(char* packet, int length){  //print the packet content, e.g. for debugging convenience
    for (int i=0;i<8;i++){
        printf("%x ",packet[i]);
    }
}

char* make_packet(char* msg, int length,long seq) //every packet has a 8-byte header
{
    u8b_t* packet=(u8b_t*)malloc(length+8);
    packet[0]=1;   //1st byte: packet type: 1-data, 2-ACK
    packet[1]=0; //2nd byte: empty, simply for alignment
    
    packet[2]=0;  //initial empty seq# field, 4-byte in total
    packet[3]=0;  //initial empty seq# field
    packet[4]=0;  //initial empty seq# field
    packet[5]=0;  //initial empty seq# field
    memcpy(packet+2,&seq,4);
    
    packet[6]=0;
    packet[7]=0;
    
    memcpy(packet+8,msg,length); //data payload, maximum 1000 bytes
    
    u16b_t cksum=checksum((char*)packet,length+8); //calculate checksum
    memcpy(packet+6,&cksum,2); //copy checksum into packet field
    
//    printf("This packet is:");print_packet((char*)packet,length+8);printf("\n");
    
    return (char*)packet;
    
}

char* make_ACK(long seq) //every ACK is 8-byte
{
    u8b_t* packet=(u8b_t*)malloc(8);
    
    packet[0]=2;   //1st byte: packet type: 1-data, 2-ACK
    packet[1]=0;  //2nd byte: empty
    
    packet[2]=0;  //initial empty seq# field
    packet[3]=0;  //initial empty seq# field
    packet[4]=0;  //initial empty seq# field
    packet[5]=0;  //initial empty seq# field
    memcpy(packet+2,&seq,4);
    
    packet[6]=0;
    packet[7]=0;

    u16b_t cksum=checksum((char*)packet,8);
    memcpy(packet+6,&cksum,2);
    return (char*)packet;
}

int is_corrupted(char* packet,int bytes){
    
    u16b_t cksum=checksum(packet,bytes);
    
//    printf("Received packet is:");print_packet(packet,bytes);printf("\n");
    
    return (cksum!=0); //check if the packet is corrupted
    
}

int is_ACK(char* packet, int bytes){
    return (packet[0]==2); //check the packet type field
}

long get_seq(char* packet, int bytes){
    long seq_num=0;
    memcpy(&seq_num,packet+2,4);
    return seq_num; //get the packet's seq #
}

/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send 
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
	double randomNum = 0.0;

	/* simulate packet loss */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < LOSS_RATE){
		//simulate packet loss of unreliable send
		printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
		return pktLen;
	}

	/* simulate packet corruption */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < ERR_RATE){
		//clone the packet
		u8b_t errmsg[pktLen];
		memcpy(errmsg, pkt, pktLen);
		//change a char of the packet
		int position = rand() % pktLen;
		if (errmsg[position] > 1) errmsg[position] -= 2;
		else errmsg[position] = 254;
		printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
		return send(fd, errmsg, pktLen, 0);
	} else 	// transmit original packet
		return send(fd, pkt, pktLen, 0);
}


//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here
long current_ACK=0;  //the same as expected_seqnum in FSM
long current_pkt=0;  //the same as next_seqnum in FSM

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

    //same as part 1
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
    //same as part 1
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
int rdt_target(int fd, char * peer_name, u16b_t peer_port){
    //same as part 1
    
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
   target (rdt_target) remote process through RDT socket; this call will
   not return until the whole message has been successfully transmitted
   or when encountered errors.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length){
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    
    int N=length/PAYLOAD+(length%PAYLOAD!=0); //count no. of packets that will be generated
    
    char* sndpkt[N];
    int pktLen[N];
    long S=current_pkt;
    
    long k=S-1; //all sndpkt[] between S to K are ACKed
    
    int sent=0;
    
    for (int i=0;i<N;i++){  //compose and send all packets; each with unique sequence number
        if ((length-sent)<PAYLOAD){   //if is the last packet
            sndpkt[i]=make_packet(msg+(i*PAYLOAD),length-sent,current_pkt);
            udt_send(fd,sndpkt[i],8+length-sent,0);
            pktLen[i]=8+length-sent;
            printf("rdt_send: Sent one message of size %d\n",pktLen[i]);
        }
        else{          //if not the last packet
            sndpkt[i]=make_packet(msg+(i*PAYLOAD),PAYLOAD,current_pkt);
            udt_send(fd,sndpkt[i],8+PAYLOAD,0);
            pktLen[i]=8+PAYLOAD;
            printf("rdt_send: Sent one message of size %d\n",pktLen[i]);
            sent+=PAYLOAD;
            
        }
        current_pkt++;
    }
    struct timeval timer;
    int status;
    
    fd_set read_fds; //read file descriptor set
    
    for (;;){
        FD_ZERO(&read_fds);
        FD_SET(fd,&read_fds);
        
        timer.tv_sec=0;
        timer.tv_usec=TIMEOUT;  //set timeout interval
        if ((status=select(fd+1,&read_fds,NULL,NULL,&timer))==-1){  //select() failed
            perror("select");
            exit(4);
        }
        else if (status==0){  //timeout occured
            printf("rdt_send: Timeout!! Retransmit the packet(s): ");
            for (int i=k-S+1;i<N;i++){
                
                udt_send(fd,sndpkt[i],pktLen[i],0);   //retransmit the packet
                printf("%ld ",get_seq(sndpkt[i],pktLen[i]));
            }
            printf("\n");
            continue; //restart select() & timeout timer
        }
        if (FD_ISSET(fd,&read_fds)){ //packet arrival
            char rcv_packet[PAYLOAD+8];
            int rcv_bytes;
            if ((rcv_bytes = recv(fd, rcv_packet, PAYLOAD+8, 0)) <= 0)
                // got error or connection closed by client
                perror("rdt_recv");
            else if (is_corrupted(rcv_packet,rcv_bytes)){
                if (rcv_packet[0]==1) printf("rdt_send: Received a corrupted packet: Type = DATA, Length = %d\n",rcv_bytes);
                else if (rcv_packet[0]==2) printf("rdt_send: Received a corrupted packet: Type = ACK, Length = %d\n",rcv_bytes);
                printf("rdt_send: Drop the packet\n");  //ignore corrupted packet
                continue;
            }
            else if (is_ACK(rcv_packet,rcv_bytes)){
                
                long this_seq=get_seq(rcv_packet,rcv_bytes);
                if (this_seq<S||this_seq>S+N-1){ //ignore unexpected packet
                    //                    printf("unexpected ACK\n");
                    continue;
                }
                else if (this_seq<=S+N-2&&this_seq>=S){
                    
                    if (this_seq>k) {  //if this ACK is to ACK a previously unACKed packet
                        k=this_seq;
                        printf("rdt_send: Received the ACK:%ld (%ld-%ld)\n",this_seq,S,S+N-1);
                    }
                    else if (this_seq<=k){  //if this ACK is received before
                        printf("rdt_send: Received a wrong ACK packet\n");
                    }
                    
                    continue;
                }
                else if (this_seq==S+N-1){
                    printf("rdt_send: Received the ACK:%ld (%ld-%ld)\n",this_seq,S,S+N-1);
                    return 0;
                }
            }
            else if (!is_ACK(rcv_packet,rcv_bytes)){ //unexpected DATA packet
                if (get_seq(rcv_packet,rcv_bytes)<current_ACK){ //if is a retransmitted DATA packet
                    char* ACK=make_ACK(current_ACK-1); //ACK its seq#
                    udt_send(fd,ACK,8,0);
                }
                else if (get_seq(rcv_packet,rcv_bytes)>=current_ACK){
                    printf("rdt_send: Retransmit the packet(s): ");
                    for (int i=k-S+1;i<N;i++){
                        udt_send(fd,sndpkt[i],pktLen[i],0);
                        printf("%ld ",get_seq(sndpkt[i],pktLen[i]));
                    }
                    printf("\n");
                    continue;


                }
            }
            
        }
    }
    
    return -1;

}

/* Application process calls this function to wait for a message of any
   length from the remote process; the caller will be blocked waiting for
   the arrival of the message. 
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length){
//implement the Extended Stop-and-Wait ARQ logic
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    
    char rcv_packet[PAYLOAD+8];
    int rcv_bytes;
    //    printf("expecting pkt# %d\n",current_ACK);
    while ((rcv_bytes=recv(fd,rcv_packet,PAYLOAD+8,0))>0){ //looping until recv expected ACK
        if (is_corrupted(rcv_packet,rcv_bytes)){
            if (rcv_packet[0]==1) printf("rdt_send: Received a corrupted packet: Type = DATA, length = %d\n",rcv_bytes);
            else if (rcv_packet[0]==2) printf("rdt_send: Received a corrupted packet: Type = ACK, length = %d\n",rcv_bytes);
        }
        else if (!is_ACK(rcv_packet,rcv_bytes)){  //not corrupted && is DATA packet
            long this_seq=get_seq(rcv_packet, rcv_bytes);
            if (this_seq==current_ACK){ //if is expected packet, i.e. expected_seqnum==this
                printf("rdt_recv: Got an expected packet\n");
                char* ACK=make_ACK(current_ACK);
                udt_send(fd,ACK,8,0);
                memcpy(msg,rcv_packet+8,rcv_bytes-8); //pass data payload to upper layer
                current_ACK++;
                return rcv_bytes-8;
            }
            else if (this_seq>current_ACK){ //if it is an out-of-order DATA packet, i.e. expected_seqnum<this
                printf("rdt_recv: received out-of-order DATA packet - seqNo:%ld (expected:%ld)\n",this_seq,current_ACK);
                printf("rdt_recv: Drop the packet\n");
                char* ACK=make_ACK(current_ACK-1);  //ACK the old ACK#
                udt_send(fd,ACK,8,0);
            }
            else if (this_seq<current_ACK){ //if it is a previous DATA packet, i.e. expected_seqnum>this
                printf("rdt_recv: Retransmit the ACK packet\n");
                char* ACK=make_ACK(current_ACK-1);  //ACK the old ACK#
                udt_send(fd,ACK,8,0);
            }
        }
    }
    return -1;
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
//implement the Extended Stop-and-Wait ARQ logic
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    
    struct timeval timer;
    int status;
    
    fd_set read_fds; //read file descriptor set
    
    for (;;){
        FD_ZERO(&read_fds);
        FD_SET(fd,&read_fds);
        
        timer.tv_sec=0;
        timer.tv_usec=TWAIT;
        if ((status=select(fd+1,&read_fds,NULL,NULL,&timer))==-1){  //select() failed
            perror("select for rdt_close()");
            exit(4);
        }
        else if (status==0){  //timeout occured, no ACK received
            printf("rdt_close: Nothing happened for 500.00 milliseconds\n");
            printf("rdt_close: Release the socket\n");
            return close(fd);
        }
        if (FD_ISSET(fd,&read_fds)){ //packet arrival
            char rcv_packet[PAYLOAD+8];
            int rcv_bytes;
            if ((rcv_bytes = recv(fd, rcv_packet, PAYLOAD+8, 0)) <= 0)
                perror("got error or connection closed by the other side");
            else if (!is_corrupted(rcv_packet,rcv_bytes)&&get_seq(rcv_packet, rcv_bytes)<current_ACK){ //received previous DATA packet, re-enter TWAIT state
                char* ACK=make_ACK(current_ACK-1); //return ACK# with (expected_seqnum-1), i.e. the last sent ACK#
                udt_send(fd,ACK,8,0);
                continue;
            }
            else if (!is_corrupted(rcv_packet,rcv_bytes)&&get_seq(rcv_packet, rcv_bytes)>=current_ACK){ //ignore other non-retransmitted packet
                continue;
            }
            
        }
    }
    return 0;
}

#endif
