
.PHONY: all clean

all: TCP_Sender TCP_Receiver 
# RUDP_Receiver RUDP_Sender 

TCP_Sender: TCP_Sender.c
	gcc -Wall -o TCP_Sender TCP_Sender.c

TCP_Receiver: TCP_Receiver.c
	gcc -Wall -o TCP_Receiver TCP_Receiver.c 

clean:
	rm -f *.o TCP_Sender TCP_Receiver *.o *.h.gch


