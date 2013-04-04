CC = gcc
CFLAGS = -g -Wall

all: clean vs_send vs_recv

vs_send: vs_send.o rudp.o event.o list.o retransmit.o
	$(CC) $(CFLAGS) $^ -o $@

vs_recv: vs_recv.o rudp.o event.o list.o retransmit.o
	$(CC) $(CFLAGS) $^ -o $@

vs_send.o vs_recv.o rudp.o: rudp.h rudp_api.h event.h

event.c: event.h

list.c: list.h

retransmit.c: retransmit.h

rudp.tar: retransmit.c retransmit.h list.c list.h vs_send.c vs_recv.c vsftp.h Makefile rudp_api.h rudp.h event.h \
	event.c rudp.c
	tar cf rudp.tar $^


tar: retransmit.c retransmit.h list.c list.h vs_send.c vs_recv.c vsftp.h Makefile rudp_api.h rudp.h event.h \
	event.c rudp.c
	tar cf rudp.tar $^

clean:
	/bin/rm -f -r *~ vs_send vs_recv *.o rudp.tar *.zip *.tar

