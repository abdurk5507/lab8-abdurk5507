# Makefile for client and server

CC = gcc
OBJCS = drone8.c
#OBJCSD = driverLab5.c

CFLAGS =  -g -Wall -lm
# setup for system
nLIBS =

all: drone8 

drone8: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

#driverLab5: $(OBJCSD)
	

clean:
	rm drone8 
