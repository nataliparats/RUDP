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

#include "event.h"
#include "rudp.h"
#include "rudp_api.h"

struct hand{
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *);
	int fd;
	struct hand* Next;
} *handlers;

struct hand* find(int fd);
void addhandler(int socket_fd,int (*handler)(rudp_socket_t, rudp_event_t,struct sockaddr_in *));


struct buffer_data_list{
	char *data;
	int seq;
	int len;
	int retransmit;
	struct buffer_data_list* Next;
};
int notclosed(int socket_fd);
struct socketnode* next(int fd,int i);
void inititerator();
struct sockaddr_in* currentTo(int fd);
int sizeTo(int socket_fd);
int sizebufferTo(int socket_fd);
struct socketnode{  
	int socket_fd;
	struct sockaddr_in* to;
	int state;
	int seq;
	int retransmit;
	int close_ask;
	int nextpacket;
	int* i;
	struct buffer_data_list* buff_data;
	struct socketnode *Next;  
}* sockets, *iterator;  
int getRetransmit(int socket_fd,struct sockaddr_in* to);
struct sockaddr_in* getTo(int socket_fd);
void setNextpacket(int socket_fd,struct sockaddr_in* to,int nextpacket);
int getNextpacket(int socket_fd,struct sockaddr_in* to);
struct socketnode* getS(int socket_fd,struct sockaddr_in* to);
int getSeqData(int fd,struct sockaddr_in* to,int position);
void addsock(int socket_fd,struct sockaddr_in* to,int *i);
void delsock(int socket_fd,struct sockaddr_in* to);
void setSeq(int socket_fd,struct sockaddr_in* to,int seq);
void setState(int socket_fd,struct sockaddr_in* to,int state);
int getState(int socket_fd,struct sockaddr_in* to);
int getSeq(int socket_fd,struct sockaddr_in* to);
void addData(struct socketnode* socket,char *data,int len);
void setClose(int socket_fd,struct sockaddr_in* to,int close);
int getClose(int socket_fd,struct sockaddr_in* to);
struct buffer_data_list* getData(int socket_fd,struct sockaddr_in* to,int position);
void deleteData(int socket_fd,struct sockaddr_in* to);
void setSeqData(int socket_fd,struct sockaddr_in* to,int position,int seq);
int sizebuffer(int socket_fd,struct sockaddr_in* to);
void incRetransmit(int socket_fd,struct sockaddr_in* to);




struct twoparameters{
	void* p1;
	int p2;
};
