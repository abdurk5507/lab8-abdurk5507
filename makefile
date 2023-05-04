# Makefile for client and server

CC = gcc
OBJCS = drone7.c
#OBJCSD = driverLab5.c

CFLAGS =  -g -Wall -lm
# setup for system
nLIBS =

all: drone7 

drone7: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

#driverLab5: $(OBJCSD)
	

clean:
	rm drone7 
