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


void retransmit(void *arg,int fd,int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *),struct sockaddr_in* to,int (*retransmithandler)(int, void *)){
	setState(fd,to,CLOSED);
	setClose(fd,to,1);
	while (sizebuffer(fd,to)>0){
		deleteData(fd,to);
	}
	event_timeout_delete(retransmithandler,arg);	
	if (notclosed(fd)==0){
		//printf("timeout delete handler  sock :%d\n",fd);
		event_fd_delete(recvhandler,getS(fd,to)->i);
	}
	if (h!=NULL)
		h(getS(fd,to)->i,RUDP_EVENT_TIMEOUT,to);
	if (notclosed(fd)==0){
		close(fd);
	}
	else{
		rudp_close(getS(fd,to)->i);
	}
}



int retransmitsyn(int f, void *arg){	
	event_timeout_delete(retransmitsyn,arg);	
	struct timeval now, interval, t;
	struct socketnode* rsocket=(struct socketnode*)arg;
	int fd = rsocket->socket_fd;
	struct sockaddr_in* to = rsocket->to;
	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);
	struct rudp_hdr* syn = malloc(sizeof(struct rudp_hdr));
	syn->version = RUDP_VERSION;
	syn->type = RUDP_SYN;
	syn->seqno = getSeq(fd,to);
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *)=NULL;
	if (find(fd)!=NULL)
		h=find(fd)->h;
	if (rsocket->retransmit<=RUDP_MAXRETRANS && rsocket->state==SYN_SENT){

		setSeq(fd,to,syn->seqno);
		incRetransmit(fd,to);
		sendtowithdropsendto(fd, syn, sizeof(struct rudp_hdr),0,(struct sockaddr *)to,sizeof(struct sockaddr));
		event_timeout(t,retransmitsyn,arg, "timeout");
	}
	if (rsocket->retransmit>RUDP_MAXRETRANS){
		//printf("maxreached syn (sock:%d)\n",fd);
		retransmit(arg,fd,h,to,retransmitsyn);
	} 
	free(syn);
	return 0;
}


int retransmitdata(int f, void *arg){	

	event_timeout_delete(retransmitdata,arg);
	struct socketnode* copy=(struct socketnode*)arg;
	if((copy->buff_data)->data==NULL){
		return 0;
	}
	struct timeval now, interval, t;
	int fd = ((struct socketnode*)arg)->socket_fd;
	struct sockaddr_in* to= ((struct socketnode*)arg)->to;
	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *)=find(fd)->h;

	if ((copy->buff_data)->retransmit<RUDP_MAXRETRANS){
		(copy->buff_data)->retransmit=(copy->buff_data)->retransmit+1;
		sendtowithdropsendto(fd, (copy->buff_data)->data, 
					sizeof(struct rudp_hdr)+(copy->buff_data)->len
					,0,(struct sockaddr *)to,sizeof(struct sockaddr));
		event_timeout(t,retransmitdata,arg, "timeout");
	}
	else {
		//printf("maxreached data (sock:%d)\n",fd);
		retransmit(arg,fd,h,to,retransmitdata);
	} 

	return 0;
}





int retransmitfin(int f, void *arg){	
	event_timeout_delete(retransmitfin,arg);	
	struct timeval now, interval, t;
	int fd = ((struct socketnode*)arg)->socket_fd;
	struct sockaddr_in* to= ((struct socketnode*)arg)->to;
	gettimeofday(&now, NULL);
	interval.tv_sec = RUDP_TIMEOUT/1000;
	interval.tv_usec = (RUDP_TIMEOUT%1000)*1000;
	timeradd(&now, &interval, &t);
	struct rudp_hdr* fin = malloc(sizeof(struct rudp_hdr));
	fin->version = RUDP_VERSION;
	fin->type = RUDP_FIN;
	fin->seqno = getSeq(fd,to);
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *);
	if (find(fd)!=NULL)
		h=find(fd)->h;
	else 
		h=NULL;
	if (getRetransmit(fd,to)<RUDP_MAXRETRANS && getState(fd,to)==CLOSE_WAIT){
		incRetransmit(fd,to);
		sendtowithdropsendto(fd, fin, sizeof(struct rudp_hdr),0,(struct sockaddr *)to,sizeof(struct sockaddr));
		event_timeout(t,retransmitfin,arg, "timeout");
	}
	if (getRetransmit(fd,to)==RUDP_MAXRETRANS){
		//printf("maxreached fin (sock:%d)\n",fd);
		retransmit(arg,fd,h,to,retransmitfin);
	} 
	free(fin);
	return 0;
}


