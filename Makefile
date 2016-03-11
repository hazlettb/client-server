#  
#  Barbara Hazlett
#  hazlettb@onid.oregonstate.edu
#  CS372
#  program 2
#

CC=g++
DEBUG=-g
CFLAGS=$(DEBUG) -Wall
PROGS=ftserver


all: $(PROGS)

ftserver: ftserver.o
	$(CC) $(CFLAGS) -o ftserver ftserver.o

ftserver.o: ftserver.cpp
	$(CC) $(CFLAGS) -c ftserver.cpp

clean:
	rm -f $(PROGS) *.o *~

