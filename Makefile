CFLAGS=-Wall -g

all: user AS

user: user.c utils.o
	gcc $(CFLAGS) utils.o user.c -o user

AS: AS.c utils.o
	gcc $(CFLAGS) utils.o AS.c -o AS

clean:
	rm -f user AS *.o

reset:
	rm -rf USERS/* AUCTIONS/*