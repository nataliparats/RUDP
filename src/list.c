#include "list.h"

struct socketnode* getS(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
	printf("NULL\n");
   return NULL;
}  


void addsock(int socket_fd,struct sockaddr_in* to,int* i){  

   struct socketnode *temp;  
   temp=(struct socketnode *)malloc(sizeof(struct socketnode));  
   temp->socket_fd=socket_fd;
   temp->state=0;
   temp->seq=0;
   temp->i=i;
   temp->nextpacket=0;
   temp->to=malloc(sizeof(struct sockaddr_in));
   memcpy(temp->to,to,sizeof(struct sockaddr_in));
   temp->retransmit=0;
   temp->close_ask=0;
   temp->buff_data=NULL;
  
   if (sockets == NULL){  
      sockets=temp;  
      sockets->Next=NULL;  
   }  
   else{  
      temp->Next=sockets;  
      sockets=temp;  
   }  
}  


/*
int deletesock(int f, void *arg){	
	struct socketnode* sock = (struct socketnode*)arg;
  	struct socketnode *prev_ptr, *cur_ptr;    
  	cur_ptr=sockets; 
  
	while(cur_ptr != NULL){  
		if(cur_ptr->socket_fd == sock->socket_fd && memcmp(cur_ptr->to,sock->to,sizeof(struct sockaddr_in))==0){  
			if(cur_ptr==sockets){  
				free(cur_ptr->i);
				free(cur_ptr->to);
				sockets=cur_ptr->Next;  
				free(cur_ptr);  
				return 0;  
			}  
			else{  
				free(cur_ptr->i);
				free(cur_ptr->to);
				prev_ptr->Next=cur_ptr->Next;  
				free(cur_ptr);  
				return 0;  
		 	}  
		}  
		else{  
			prev_ptr=cur_ptr;  
			cur_ptr=cur_ptr->Next;  
		}  
	}  
	return 1;

}



void delsock(int fd,struct sockaddr_in* to){  
	struct timeval now, interval, t;
	gettimeofday(&now, NULL);
	interval.tv_sec = 3*RUDP_TIMEOUT/1000;
	interval.tv_usec = ((3*RUDP_TIMEOUT)%1000)*1000;
	timeradd(&now, &interval, &t);
	event_timeout(t,deletesock,getS(fd,to), "timeout");
}  



*/





void setSeq(int socket_fd,struct sockaddr_in* to,int seq){  
   struct socketnode *cur_ptr;    

   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){ 
	    cur_ptr->seq=seq;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  
void setClose(int socket_fd,struct sockaddr_in* to,int close){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    cur_ptr->close_ask=close;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  
void setState(int socket_fd,struct sockaddr_in* to,int state){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    cur_ptr->state=state;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  

int getState(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr->state;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return -1;
}  

int getClose(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr->close_ask;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return -1;
}  

int getSeq(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr->seq;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return -1;
}  


void setSeqData(int fd,struct sockaddr_in* to,int position,int seq){
   struct buffer_data_list* gd=getData(fd,to,position);
   if (gd==NULL)
	return;
   struct rudp_hdr* reply=(struct rudp_hdr* )gd->data;
   reply->seqno=seq;
   gd->seq=seq;
}


int getSeqData(int fd,struct sockaddr_in* to,int position){
   struct buffer_data_list* gd=getData(fd,to,position);
   if (gd==NULL)
	return(-1);
   struct rudp_hdr* reply=(struct rudp_hdr* )gd->data;
   return(reply->seqno);
}

void addData(struct socketnode* socket,char *data,int len){  
   struct buffer_data_list  *ptr;
   char* copydata=malloc(len*sizeof(char)+sizeof(struct rudp_hdr));
   struct rudp_hdr* reply = (struct rudp_hdr*)copydata;
   reply->version = RUDP_VERSION;
   reply->type = RUDP_DATA;
   reply->seqno = -1;
   memcpy(copydata+sizeof(struct rudp_hdr),data,len*sizeof(char));
   struct buffer_data_list  *buffer= malloc(sizeof(struct buffer_data_list));
   
    ptr=socket->buff_data;
    if (ptr==NULL){
	    buffer->len=len;
	    buffer->seq=-1;
	    buffer->data=copydata;
	    socket->buff_data=buffer;
	    buffer->Next=NULL;
    }
    else{
	while(ptr->Next!=NULL){
		ptr=ptr->Next;
	}
	    ptr->Next=buffer;
	    buffer->len=len;
	    buffer->seq=-1;
	    buffer->data=copydata;
	    buffer->Next=NULL;		
    }

}  

struct buffer_data_list* getData(int socket_fd,struct sockaddr_in* to,int position){
   struct socketnode *cur_ptr;    
   struct buffer_data_list  *ptr;
   cur_ptr=sockets; 
   int i=0;
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    ptr=cur_ptr->buff_data;
	    if (ptr==NULL){
		   return NULL;
	    }
	    else{
		while(ptr!=NULL){
			if (i==position)
				break;	
			i++;	
			ptr=ptr->Next;	
		}
		return ptr;
	    }
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return NULL;
}


int sizebuffer(int socket_fd,struct sockaddr_in* to){
   struct socketnode *cur_ptr;    
   struct buffer_data_list *data_ptr;
   cur_ptr=sockets; 
   int res=0;
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
           data_ptr=cur_ptr->buff_data;
	   while(data_ptr!=NULL){
	 	data_ptr=data_ptr->Next;
		res++;
	   }
	   return res;

      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return res;
}  

int notclosed(int fd){
   struct socketnode *cur_ptr = sockets; 
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == fd && cur_ptr->state!=CLOSED){  
         return 1;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return 0;
}  


int sizebufferTo(int socket_fd){
   struct socketnode *cur_ptr = sockets; 
   int res=0;
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && cur_ptr->state!=CLOSED){  
         res=res+sizebuffer(socket_fd,cur_ptr->to);
         cur_ptr=cur_ptr->Next;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return res;
}  

int sizeTo(int socket_fd){
   struct socketnode *cur_ptr = sockets; 
   int res=0;
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd){  
           res++;

      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return res;
}  


void deleteData(int socket_fd,struct sockaddr_in* to){
   struct socketnode *cur_ptr=sockets;    
   struct buffer_data_list *save;

   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	   save=(cur_ptr->buff_data)->Next;
	   free(cur_ptr->buff_data->data);
	   cur_ptr->buff_data->data=NULL;
	   //free(cur_ptr->buff_data);
	   cur_ptr->buff_data=save;
	   return;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   printf("NOT FIND(list.c 270)\n");
}  




void incRetransmit(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    cur_ptr->retransmit=cur_ptr->retransmit+1;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  

void setRetransmit(int socket_fd,struct sockaddr_in* to,int retransmit){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    cur_ptr->retransmit=retransmit;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  

int getRetransmit(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr->retransmit;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return -1;
}  







void setNextpacket(int socket_fd,struct sockaddr_in* to,int nextpacket){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    cur_ptr->nextpacket=nextpacket;
            return;  
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
}  

int getNextpacket(int socket_fd,struct sockaddr_in* to){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd && memcmp(cur_ptr->to,to,sizeof(struct sockaddr_in))==0){  
	    return cur_ptr->nextpacket;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return -1;
}  


struct sockaddr_in* getTo(int socket_fd){  
   struct socketnode *cur_ptr;    
   cur_ptr=sockets; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->socket_fd == socket_fd ){  
	    return cur_ptr->to;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return NULL;
}  


struct hand* find(int fd){
   struct hand *cur_ptr;    
   cur_ptr=handlers; 
  
   while(cur_ptr != NULL){  
      if(cur_ptr->fd == fd ){  
	    return cur_ptr;
      }  
      else{  
         cur_ptr=cur_ptr->Next;  
      }  
   }  
   return NULL;
}





void addhandler(int fd,int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *)){  

   struct hand *temp;  
   temp=(struct hand *)malloc(sizeof(struct hand));  
   temp->fd=fd;
   temp->h=h;
     
   if (sockets == NULL){  
      handlers=temp;  
      handlers->Next=NULL;  
   }  
   else{  
      temp->Next=handlers;  
      handlers=temp;  
   }  
} 


void inititerator(){
	iterator=sockets;
}

struct socketnode* next(int fd,int i){  
   if (iterator==NULL)
    	return NULL; 
   if (i!=0)
   	iterator=iterator->Next;
   if (iterator==NULL)
    	return NULL;  
   while(iterator->socket_fd != fd){  
	    iterator=iterator->Next;
	    if (iterator==NULL)
            	return NULL;  
   }  
   return iterator;
}



struct sockaddr_in* currentTo(int fd){ 
	if (iterator!=NULL) 
   		return iterator->to;
	else
		return NULL;
}





