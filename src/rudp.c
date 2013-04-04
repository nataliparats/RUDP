#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "event.h"
#include "rudp.h"
#include "rudp_api.h"
#include "list.h"
#include "retransmit.h"


ssize_t  sendtowithdropsendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen){
	if (DROP!= 0 && rand() % DROP == 1){
		//printf("packet droped\n");
		return 0;
	}		
	else 
		return sendto(sockfd,buf,len,flags,dest_addr,addrlen);
}

/* 
 * rudp_socket: Create a RUDP socket. 
 * May use a random port by setting port to zero. 
 */



rudp_socket_t rudp_socket(int port){	
	int sock;
	if (port<0){
		printf("ERROR port<0\n");
		return NULL;
	}
	int *i=malloc(sizeof(int));
	struct sockaddr_in* addr=malloc(sizeof(struct sockaddr_in));
	if ((sock=socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP))==-1){
		perror("Error rudp_sock: socket: ");
		return NULL;
	}
	addr->sin_family = AF_INET;
        srand(time(NULL));
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
	if (port == 0) {
		port = (rand() % (65535 - 8081)) + 8081;
       		addr->sin_port = htons(port);
		while (bind(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in))==-1){
			port = (rand() % (65535 - 8081)) + 8081;
	       		addr->sin_port = htons(port);
		}
	}
	else{
		addr->sin_port = htons(port);
		if (bind(sock, (struct sockaddr *)addr, sizeof(struct sockaddr_in))==-1){
			perror("Error rudp_sock: bind: ");
			return NULL;
		}
	}
	*i=sock;
	if (event_fd(sock,recvhandler,i,"RUDP")<0){
		return NULL;
	}
	
	return i;
}

/* 
 *rudp_close: Close socket 
 */ 

int rudp_close(rudp_socket_t rsocket) {
	struct timeval now, interval, t;
	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);
	int fd = ((struct socketnode*)rsocket)->socket_fd;
	inititerator();
	struct sockaddr_in* to;
	int i=0;
	//printf("sock %d buffersize %d\n",fd,sizebufferTo(fd));
	if (sizebufferTo(fd)==0){
		while(next(fd,i)!=NULL){
			to=currentTo(fd);
			if (to==NULL) {return 0;}
			i=1;
			if (getState(fd,to)!=CLOSED){
				//no data, send FIN
				struct rudp_hdr* fin = malloc(sizeof(struct rudp_hdr));
				fin->version = RUDP_VERSION;
				fin->type = RUDP_FIN;
				fin->seqno = getSeq(fd,to)+1;
				getS(fd,to)->retransmit=0;
				setState(fd,to,CLOSE_WAIT);
				sendtowithdropsendto(fd, fin, sizeof(struct rudp_hdr),0,(struct sockaddr*)to,sizeof(struct sockaddr));
				setSeq(fd,to,fin->seqno);
				event_timeout(t,retransmitfin,getS(fd,to),"timeout");
				free(fin);

			}
		}
	}
	else{
		inititerator();
		while(next(fd,i)!=NULL){
			to=currentTo(fd);
			if (to==NULL) return 0;
			setClose(fd,to,1);
			i=1;
		}
	}

	return 0;
}





int recvhandler(int fd, void *arg){
	struct timeval now, interval, t;
	socklen_t addrlen=sizeof(struct sockaddr);
	struct sockaddr *src_addr=malloc(sizeof(struct sockaddr));
	char* buf=malloc(RUDP_MAXPKTSIZE+sizeof(struct rudp_hdr));
	int datalen;
    	int (*handler)(rudp_socket_t,struct sockaddr_in *, char *, int);
        handler = (int (*)(rudp_socket_t, struct sockaddr_in*, char *, int))arg;
	struct rudp_hdr* header;
	datalen=recvfrom(fd,buf,RUDP_MAXPKTSIZE+sizeof(struct rudp_hdr),0,src_addr, &addrlen);
	if (datalen<=0){
		return -1;
	}
	header = (struct rudp_hdr*) buf;
	if(header->version!=1){
		return 0;
	}
	struct sockaddr_in* to=(struct sockaddr_in*)src_addr;
	if (getSeq(fd,to)==-1){
		addsock(fd,to,arg);
	}

	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);
        void *data;
        data = malloc(datalen -  sizeof (struct rudp_hdr));
	memcpy(data,buf+sizeof(struct rudp_hdr),datalen-sizeof (struct rudp_hdr));
	int type = header->type;
	int state = getState(fd,to);
	u_int32_t seq = header->seqno;
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *)=find(fd)->h;
	struct rudp_hdr* ack_reply = malloc(sizeof(struct rudp_hdr));
	ack_reply->version = RUDP_VERSION;
	ack_reply->type = RUDP_ACK;
	ack_reply->seqno = seq+1;
	int cur_w;
	switch(state){
		case LISTEN:
				if (type==RUDP_SYN){
					//send ACK with seq+1
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
					//goto ESTABLISH
					setState(fd,to,ESTABLISH);
					setSeq(fd,to,seq+1);
				}
				else if(type==RUDP_FIN){
					//send an ACK
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
					
				}
				return 0;
				break;
		case SYN_SENT :			
				if (type==RUDP_ACK && seq==getSeq(fd,to)+1){
					setSeq(fd,to,seq);
					for (cur_w=0;cur_w<RUDP_WINDOW;cur_w++){
						if (sizebuffer(fd,to)>cur_w){					
							struct socketnode* copy=malloc(sizeof(struct socketnode));
							setSeqData(fd,to,cur_w,getSeq(fd,to)+cur_w);
							copy->socket_fd=fd;
							copy->to=to;
							copy->buff_data=getData(fd,to,cur_w);
							copy->Next=NULL;
	

							setNextpacket(fd,to,getNextpacket(fd,to)+1);
							sendtowithdropsendto(fd,getData(fd,to,cur_w)->data, sizeof(struct rudp_hdr)+getData(fd,to,cur_w)->len,0,src_addr,addrlen);	
							
							event_timeout(t,retransmitdata,copy,"timeout");				
						}
					}
					//goto ESTABLISH
					setState(fd,to,ESTABLISH);
				}
				else if(type==RUDP_FIN){
					//send an ACK
					setState(fd,to,LISTEN);
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
					
				}
				return 0;
				break;
		case ESTABLISH : 		
				if (type==RUDP_DATA){
					//send ACK
					if (seq<=getSeq(fd,to)){
						if (seq==getSeq(fd,to))
							handler(getS(fd,to),(struct sockaddr_in *) src_addr, data, datalen - sizeof (struct rudp_hdr));

						sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
						
						if (seq==getSeq(fd,to))
							setSeq(fd,to,seq+1);
					}

				}
				else if(type==RUDP_ACK){
					//Check the sequence number && continue to send data
					if (getSeqData(fd,to,0)!=-1){
						while(seq>getSeqData(fd,to,0) && seq<=getSeqData(fd,to,0)+RUDP_WINDOW && sizebuffer(fd,to)>0){
				
							if (sizebuffer(fd,to)>=getNextpacket(fd,to))
								setSeqData(fd,to,getNextpacket(fd,to),getSeqData(fd,to,0)+getNextpacket(fd,to));
							deleteData(fd,to);
							setNextpacket(fd,to,getNextpacket(fd,to)-1);
							if (sizebuffer(fd,to)>getNextpacket(fd,to)){
								struct socketnode* copy=malloc(sizeof(struct socketnode));
								copy->socket_fd=fd;
								copy->to=to;
								copy->buff_data=getData(fd,to,getNextpacket(fd,to));
								copy->Next=NULL;
								event_timeout(t,retransmitdata,copy,"timeout");
								sendtowithdropsendto(fd, getData(fd,to,getNextpacket(fd,to))->data, 
										sizeof(struct rudp_hdr)+getData(fd,to,getNextpacket(fd,to))->len
										,0,src_addr,addrlen);
							}
							if (sizebuffer(fd,to)>getNextpacket(fd,to))
								setNextpacket(fd,to,getNextpacket(fd,to)+1);

							if (sizebuffer(fd,to)==0){
								break;
							}
						}
						if (seq>getSeq(fd,to))
							setSeq(fd,to,seq);
					}				
					if(sizebuffer(fd,to)==0 && getClose(fd,to)==1)
						rudp_close(getS(fd,to));				

				}
				else if(type==RUDP_FIN){
					//send an ACK
					//printf("FIN received\n");
					setState(fd,to,LISTEN);					
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);

				}
				else if(type==RUDP_SYN){
					//send an ACK
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
				}				
				break;
		case CLOSE_WAIT :
				if (type==RUDP_ACK && seq==getSeq(fd,to)+1){
					//printf("FIn-ACK received\n");
					//goto to closed
					setState(fd,to,CLOSED);
					setClose(fd,to,1);
					setSeq(fd,to,0);
					if (notclosed(fd)==0){
						event_fd_delete(recvhandler,arg);
					}
					h(getS(fd,to)->i,RUDP_EVENT_CLOSED,NULL);
					if (notclosed(fd)==0){
						close(fd);
					}					
				}				
				else{
					return 0;
				}
				break;
		case CLOSED :
				if(type==RUDP_FIN){
					//send an ACK
					sendtowithdropsendto(fd, ack_reply, sizeof(struct rudp_hdr),0,src_addr,addrlen);
				}
				return 0;
				break;
		default :
				return 0;
				break;
	}
	//free(ack_reply);
	return 0;
}

/* 
 *rudp_recvfrom_handler: Register receive callback function 
 */ 

int rudp_recvfrom_handler(rudp_socket_t rsocket,int (*handler)(rudp_socket_t, struct sockaddr_in *, char *, int)) {
	event_fd_delete(recvhandler,rsocket);
	int sock = *(int*)rsocket;
	if (sock==-1){
		return -1;
	}
	if (event_fd(sock,recvhandler,handler,"RUDP")<0){
		return -1;
	}


	return 0;
}

/* 
 *rudp_event_handler: Register event handler callback function 
 */ 
int rudp_event_handler(rudp_socket_t rsocket, 
		       int (*handler)(rudp_socket_t, rudp_event_t, 
				      struct sockaddr_in *)) {
	addhandler(*(int*)rsocket,handler);
	return 0;
}


/* 
 * rudp_sendto: Send a block of data to the receiver. 
 */




int rudp_sendto(rudp_socket_t rsocket, void* data, int len, struct sockaddr_in* to) {
	if (len>RUDP_MAXPKTSIZE){
		return -1;
	}

	struct timeval now, interval, t;
	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);

	int fd=*(int*)rsocket;
	if (getSeq(fd,to)==-1){
		addsock(fd,to,(int*)rsocket);
	}
	if (getState(fd,to)==CLOSED){
		printf("connection closed(max retransmit reach?)\n");
		return 0;
	}
	int state = getState(fd,to);
	getS(fd,to)->retransmit=0;
	struct rudp_hdr* syn = malloc(sizeof(struct rudp_hdr));
	syn->version = RUDP_VERSION;
	syn->type = RUDP_SYN;
	syn->seqno = rand()%65535;
	int cur_w;
	int oldsizebuffer = sizebuffer(fd,to);
	if(state==LISTEN){

		sendtowithdropsendto(fd, syn, sizeof(struct rudp_hdr),0,(struct sockaddr*)to,sizeof(struct sockaddr));

		setState(fd,to,SYN_SENT);
		setSeq(fd,to,syn->seqno);
		event_timeout(t,retransmitsyn,getS(fd,to),"timeout");
	}

	addData(getS(fd,to),data,len);	

	if (state==ESTABLISH && sizebuffer(fd,to)<RUDP_WINDOW){
	
		for (cur_w=oldsizebuffer;cur_w<RUDP_WINDOW;cur_w++){
			if (sizebuffer(fd,to)>cur_w){
				struct socketnode* copy=malloc(sizeof(struct socketnode));	
				setSeqData(fd,to,cur_w,getSeq(fd,to)+cur_w);
				copy->socket_fd=fd;
				copy->Next=NULL;
				copy->to=to;
				copy->buff_data=getData(fd,to,cur_w);
				event_timeout(t,retransmitdata,copy,"timeout");
				sendtowithdropsendto(fd,getData(fd,to,cur_w)->data, 
						sizeof(struct rudp_hdr)+getData(fd,to,cur_w)->len,
						0,(struct sockaddr *)to,sizeof(struct sockaddr_in));
				setNextpacket(fd,to,getNextpacket(fd,to)+1);
			}
		}
	}

	free(syn);
	return 0;
}

