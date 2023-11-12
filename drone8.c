#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

//Homework 8
//abdurk5507

#define CURRENTVERSION 8
#define MAXPARTNERS 100
#define STDIN 0
#define TTL 6
//#define TESTING5
//#define TESTING
//#define PROMPT
//#define LAB4
//#define LAB5_TESTING
//#define LAB6_TESTING
//#define LAB7_TESTING
//#define LAB8_TESTING
#define MAXSEQNUMBER 100
#define STOREDMESSAGETOTAL 20

#define destPORT "toPort"
#define SOURCEPORT "fromPort"
#define SEQNUMBER "seqNumber"
#define SEQNUM "seqNumber"
#define MAXDISTANCE 2

struct _partnersHost{
  char ipAddress[25];
  int  portNumber;
  int location; 
  //New additions
  int currentSeqNumber;
  bool hasAcked[MAXSEQNUMBER];
  bool sentAck[MAXSEQNUMBER];
};
struct _partners{
  int maxHosts;
  struct _partnersHost hostInfo[MAXPARTNERS];
};

// new things for the select
struct  _tokens {
  char key[100];
  char value[100];
};

struct _messages{
  char message[200];
  int hopsRemaining; //TTL is 5
};

fd_set socketFDS; // the socket descriptor set
int maxSD; // tells the OS how many sockets are set
int ROWS, COLUMNS;
static int messagesIndex = 0;

#ifdef LAB7_TESTING
static int sendPathCount = 0;
#endif

int readConfigFile(struct _partners *Partners, int myPortNumber, int *myLoc);
void printPartners (struct _partners *Partners);
int mainClient(int sd, struct _partners *Partners, int myPort, int *myLoc);
void clientSend(char *buffer, int sd, struct _partners Partners, int myPort);
int sendData(char *buffer, int socketDescriptor, struct sockaddr_in server_address);
FILE * openFile();
char *rtrim(char *s);
void cleanse (char * buffer);

void makeSocket(int *sd, char *argv[], struct sockaddr_in *server_address, int *myPort);
void receiveMessage(char *buffer, int socketDescriptor, struct sockaddr_in *from_address);

int findTokens (char *buffer, struct _tokens *tokens);
int findIntToken (struct _tokens *tokens, int numberOfTokens, char * keyWanted);
int findTokenElementNum(struct _tokens *tokens, int numberOfTokens, char *keyWanted);
char *findCharToken (struct _tokens *tokens, int numberOfTokens, char * keyWanted);
void printTokens (struct _tokens *tokens, int numberOfTokens, int myLoc);
int recreateBuffer(char *buffer, int numberOfTokens, struct _tokens *tokens, int myLocation, int myPort, struct _partners *Partners);

//void sendACK(int fromPort, int toPort, struct sockaddr_in *from_address, struct _partners *Partners, int socketDescriptor);
int findPartner(int sourcePort, struct _partners Partners);
int findSeqNumber(int toPort, struct _partners *Partners);
int isWithinRange(int rowSize, int columnSize, int otherLocation, int myLocation);
//A split version of my isWithinRange function
int findDistance(int x, int y, int myX, int myY);
int findXYCoordinates(int squareNumber, int *row, int *columns);

void updateTTL(struct _tokens *tokens, int numberOfTokens, char *buffer);
int returnTTL(struct _tokens *tokens, int numberOfTokens);

void forwardSend(char *buffer, int sd, struct _partners Partners, struct _tokens *tokens, int numberOfTokens,int myLoc, int myPort, struct _messages *storedBuffer);
int checkIP(char *IP2Check);
int validIP(char *anIPAddress);
int validNum(char * aNumber);
void checkPortNum(int portNumber);
void makeSimpleSocket(int *sd);
void reSend(struct _messages *storedBuffer, int sd, struct _partners Partners, int myPort, int myLoc);
int isDuplicate(char message, struct _partners Partners, struct _tokens *tokens, struct _messages *storedBuffer);

int main(int argc, char *argv[]){
  int socketDescriptor; /* socket descriptor */
  int numberOfTokens = 0; // keep track of how many tokens i read in
  int myRow, myColumn;
  int senderLoc = 0;
  int distance;
  int myPort = 0, myLoc = 0;

  struct _partners Partners;  
  struct sockaddr_in server_address; /* my address */
  struct sockaddr_in from_address;  /* address of sender */
  struct _tokens tokens [100];
  struct _messages storedBuffer[STOREDMESSAGETOTAL];
  struct timeval timeout;

  char buffer[1000];
  char buffer2Send[1000];
  //char updateSendPath[100];
  
  //Check all input parameters
  //Switched to what Dr. Ogle was doing and took in rows and columns at CLI
  //Much easier for testing
  if (argc < 4){
    printf("usage is: server <portnumber> <#rows> <#cols>\n");
    exit (1);
  }

  //Create a socket
  makeSocket(&socketDescriptor, argv, &server_address, &myPort);
  
  //Do client stuff
  mainClient(socketDescriptor, &Partners, myPort,&myLoc);

  //Check that row is a number
  validNum(argv[2]);
  ROWS = strtol(argv[2], NULL, 10); /* many ways to do this */

  //Check that col is a number
  validNum(argv[3]);
  COLUMNS = strtol(argv[3], NULL, 10); /* many ways to do this */
  
  //Obtain coordinates
  findXYCoordinates( myLoc, &myRow, &myColumn);
  printf ("you are in location %d and that is in row %d, column %d\n",
	  myLoc, myRow, myColumn);
  
  for (;;){
    //Var declarations
    int destinationPort, sourcePort; 
    int returnCode = 0;
    int version = 0;
    fflush(stdout);

    //Prompt user to input string to send out
    printf ("Enter the string you'd like to send: \n");

    //Clear buffer
    memset (buffer, 0, 200);

    //Select statement variables 
    FD_ZERO(&socketFDS);// NEW                                 
    FD_SET(socketDescriptor, &socketFDS); //NEW - sets the bit for the initial sd socket
    FD_SET(STDIN, &socketFDS); // NEW tell it you will look at STDIN too
    
    if (STDIN > socketDescriptor) // figure out what the max sd is. biggest number
      maxSD = STDIN;
    else
      maxSD = socketDescriptor;

    //timeout values
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;
    
    // NEW block until something arrives
    returnCode = select (maxSD+1, &socketFDS, NULL, NULL, &timeout); 

    #ifdef TESTING
    printf ("select popped\n");
    #endif

    if(returnCode == 0){ //had a timeout!
      //Add a function here that resends the messages to all partners
      //while decremeneting ttl

      // I need to resend my messages
      //To-Do: (Do in a for loop)
      //1. Pull message from struct and put it in a buffer
      //2. Decrement the timeToLive in the struct. If you need to make changes to the buffer, do so now.
      //3. Send it out to everyone again
      reSend(storedBuffer, socketDescriptor, Partners, myPort, myLoc);
      printf("Timeout occurred!\n");
      continue;
    }
    else{
      if (FD_ISSET(STDIN, &socketFDS)){
        char *ptr = NULL;
        memset (buffer, '\0', 200);
        memset (buffer2Send, '\0', 300);
        ptr = fgets(buffer, sizeof(buffer), stdin);
        
        if (strlen(buffer)>200){
          printf ("HELLO - you are sending too many bytes, please reduce\n");
          continue;
        }
        if (strlen(buffer)<1){
          printf ("HELLO - you are sending too few bytes, please increase\n");
          exit(1);

        }

        /* Lab 6 - find the toPort - use it to figure out partner */
        /* Lab 6 - then add the seqNumber stuff. */
        //Keeping the following the same. Can't say I understand it all, but it works
        int length2Copy = 0;
        char *ptrStart = strstr(buffer, "toPort:"); 
        if (ptrStart == NULL){
          printf ("no toPort, can't send it\n");
          continue;
        }
        char *ptrColon = strstr(ptrStart,":"); 
        char *ptrEnd = strstr(ptrStart, " "); 
        if (ptrEnd == NULL) { 
          length2Copy = 6; 
        }
        else{
          length2Copy = ptrColon - ptrStart;
        }
        char toPortChar[6];
        strncpy(toPortChar, ptrColon+1, length2Copy-1); // ignore colon and blank
        int toPortInt = atoi(toPortChar);

        int seqNumber2Add = findSeqNumber(toPortInt, &Partners);
        #ifdef LAB7_TESTING
        printf ("toPortInt is %d\n", toPortInt);
        #endif
        if (seqNumber2Add < 0){
          printf ("Can't find partner for seqNumber \n");
          continue;
        }
        /* End of the lab 6 addition */
        
        //Affix null pointer to the end of the buffer
        buffer[strlen(buffer)-1] = '\0';

        //Add additional values to the buffer
        //Realized that I've been adding TTL in the initial message instead of doing so here
        sprintf (buffer2Send, "%s fromPort:%d send-path:%d seqNumber:%d TTL:%d  location:%d",
          buffer, myPort, myPort, seqNumber2Add, TTL, myLoc);

        #ifdef LAB5_TESTING
        printf("After inputting it into the buffer, my location is %d\n", myLoc);

        printf("buffer 2 send before first send: %s\n", buffer2Send);
        #endif
        
        //Send data to other drones
        clientSend(buffer2Send, socketDescriptor, Partners, myPort);
      }
      if (FD_ISSET(socketDescriptor, &socketFDS)){ 
        //Receive data
        receiveMessage(buffer, socketDescriptor, &from_address);

        /* cleanse the data, meaning i have to take into account    */
        /* there may be a msg...and that will have " as a delimeter */
        cleanse (buffer);

        numberOfTokens = findTokens (buffer, tokens);

        //Look for port #s
        destinationPort = findIntToken (tokens, numberOfTokens, destPORT);
        sourcePort = findIntToken(tokens, numberOfTokens, SOURCEPORT);

        //Check if port tokens were found 
        if (destinationPort == -1 || sourcePort == -1){
          #ifdef LAB6_TESTING
          printf ("No source and/or dest port. Skip message.\n");
          #endif
          continue;
        }
        
        //Find version # in message
        version = findIntToken (tokens, numberOfTokens, "version");

        //Check if version # was found
        if (version == -1){
          printf ("Didn't find a version in the string, skipping this message\n");
          continue;
        }

        //Check version # against current version
        if (version != CURRENTVERSION){
          // token not found, continue
          printf ("wrong version %d  skipping \n", version);
          continue;
        }

        #ifdef TESTING
        printf("Other location is %d\n", Partners.hostInfo[i].location);
        printf("My location is %d\n", myLoc);
        #endif

        //Find location in message
        senderLoc = findIntToken(tokens, numberOfTokens, "location");

        if (senderLoc == -1){
          printf ("no location found %d  skipping \n", senderLoc);
          continue;
        }

        //Obtain distance
        int senderRow, senderColumn;
        
        //Have to use this function instead of isWithinRange
        //Doesn't work with the rest of the code if I use the latter
        returnCode = findXYCoordinates(senderLoc, &senderRow, &senderColumn);

        if (returnCode == -1){
          printf ("Location is not in the grid\n");
          continue;
        }

        //Same here
        distance = findDistance(senderRow, senderColumn, myRow, myColumn);

        #ifdef LAB5_TESTING
        printf ("the distance between you and sender is %d \n", distance);

        if (distance <=2){
          printf ("dest port is %d, my port is %d\n", destinationPort, myPort);
        }
        #endif
        
        
        if (destinationPort == myPort){
          //Older checks
          if (distance > MAXDISTANCE){
            #ifdef LAB4
            printf("DARN\n");
            #endif
          }
          else{
            //Find the ACK
            int partnerPos = findPartner(sourcePort, Partners);
            int seqNum = findIntToken (tokens, numberOfTokens, SEQNUM);
            char *isAck = findCharToken (tokens, numberOfTokens, "type");
            
            //Check if we found an ACK
            if (isAck != NULL && !strcmp(isAck, "ACK")){
              #ifdef LAB6_TESTING
              printf ("Processing an ACK\n");
              #endif

              //Indicates that the partner has been found
              //Saddle up, partner
              if (partnerPos > -1){ 
                if (Partners.hostInfo[partnerPos].hasAcked[seqNum] == true){
                  printf ("Received a duplicate ACK for SeqNum %d fromPort %d\n",
                  seqNum, sourcePort);
                  
                }

                else{
                  printf ("Received an ACK for SeqNum %d fromPort %d\n", seqNum, sourcePort);
                  Partners.hostInfo[partnerPos].hasAcked[seqNum] = true;
                  printTokens (tokens, numberOfTokens, myLoc);
                }

              }

              //Partner was not found
              else { 
                printf ("Received an ACK for SeqNum %d fromPort %d but couldn't find partner\n",
                seqNum, sourcePort);
              }

            }
            else{
              //Send an ACK back to sender
              if (partnerPos > -1){
                int i;
                char buffer[200];

                //Clear buffer before using
                memset (buffer, 0, 200);

                //Add fields to buffer
                sprintf (buffer, "type:ACK version:%d send-path:%d TTL:%d  toPort:%d fromPort:%d seqNumber:%d  location:%d",
                  CURRENTVERSION, myPort, TTL, sourcePort, myPort, seqNum, myLoc);

                for (i = 0;i < Partners.maxHosts;i++){
                  //Declare variables
                  int portNumber;
                  char serverIP[20];

                  //Clear buffer before using
                  memset (serverIP, 0, 20); 
                  portNumber = Partners.hostInfo[i].portNumber;

                  // don't forward to myself
                  //Can't believe the solution to this issue was so simple
                  if (portNumber == myPort){
                    continue;
                  }

                  //Copy ipaddr to var
                  strcpy(serverIP, Partners.hostInfo[i].ipAddress);
                  
                  //Set server info
                  server_address.sin_family = AF_INET;
                  server_address.sin_port = htons(portNumber);
                  server_address.sin_addr.s_addr = inet_addr(serverIP);

                  //Send out info in buffer      
                  sendData(buffer, socketDescriptor, server_address);
                }    
                
                //Check for duplicate packet
                if (Partners.hostInfo[partnerPos].sentAck[seqNum] == true){
                  printf ("Duplicate packet detected! Duplication ACK being sent for ");
                  printf ("seq# %d, fromPort %d\n", seqNum, sourcePort);

                  //Print out send path here
                  int i;
                  for(i = 0; i < numberOfTokens; i++){
                    if(strcmp(tokens[i].key, "send-path") == 0){
                      printf ("****************************************************\n");
                      printf("           Duplicate Packet Send-Path\n");
                      printf ("%20s %20s\n\n", tokens[i].key, tokens[i].value);
                      printf ("****************************************************\n");
                    }
                  }
                }

                //Send ACK again
                else{
                  printf ("Sending an ack for seqNum %d partner # %d, fromPort %d\n",
                  seqNum, partnerPos, sourcePort);
                  Partners.hostInfo[partnerPos].sentAck[seqNum] = true;
                  printTokens (tokens, numberOfTokens, myLoc);
                }
              }
            } // end of else

            //Handle the move command
            int moveCmd = findIntToken(tokens, numberOfTokens, "move");
            //was originally
            //char *moveCmd

            //Move command not found
            if (moveCmd == -1){
              printf("Move command was not found\n");
            }

            //Move command found! Take action!
            else{
              #ifdef LAB7_TESTING
              printf("I found the token for move\n");
              printf("Move command is %d\n", moveCmd);
              printf("My current location is %d\n", myLoc);
              #endif

              //Update my location to what is in the move command
              myLoc = moveCmd;

              #ifdef LAB7_TESTING
              printf("Updated location is now %d\n", myLoc);
              #endif

              #ifdef LAB8_TESTING
              int i;
              for(i = 0; i < STOREDMESSAGETOTAL; i++){
                printf("This is message %d:\n", i);
                printf(storedBuffer[i].message);
                printf("\n");
              }
              #endif

              //I need to update the location in the stored messages before I resend them.
              //Tokenize
              
              int i;
              int numberOfMessageTokens;
              numberOfMessageTokens = findTokens(storedBuffer[0].message, tokens);

              #ifdef LAB8_TESTING
              for(i = 0; i < numberOfMessageTokens; i++){
                printf ("%20s %20s\n", tokens[i].key, tokens[i].value);
              }
              #endif

              //Update location in stored message
              for(i = 0; i < numberOfMessageTokens; i++){
                if(strcmp(tokens[i].key, "location") == 0){
                  char convertedMoveCmd[10];
                  sprintf(convertedMoveCmd, "%d", moveCmd);
                  //memcpy(tokens[i].value, &moveCmd, sizeof(moveCmd));
                  strcpy(tokens[i].value, convertedMoveCmd);
                  printf("updated location in stored struct is %s\n", tokens[i].value);
                }
              }

              #ifdef LAB8_TESTING
              printf("Updated tokens:\n");
              for(i = 0; i < numberOfMessageTokens; i++){
                printf ("%20s %20s\n", tokens[i].key, tokens[i].value);
              }
              #endif

              //Put tokenized messages back in the buffer
              char tempBuffer[100];
              //char overallBuffer[500];
              memset(tempBuffer, 0, 100);
              //Reinput values into buffer
              for(i = 0; i < numberOfMessageTokens; i++){
                sprintf(tempBuffer, "%s:%s%s", tokens[i].key, tokens[i].value, " ");
                strcat(storedBuffer[0].message, tempBuffer);
              }
              
              #ifdef LAB8_TESTING
              printf("Updated stored message is %s\n", storedBuffer[0].message);
              #endif

              //Resend all messages after moving locations
              reSend(storedBuffer, socketDescriptor, Partners, myPort, myLoc);

              printf("Location moved! Resent all stored messages\n");

              continue;
            }
          }
        }
        //Forward message
        else {
          int ttl;

          //Find ttl token
          ttl = findIntToken (tokens, numberOfTokens, "TTL");

          //Check if TTL is 0
          if (ttl == 0){
            printf ("message received NOT for me and msg out of lives!.\n");
            continue;
          }
          //Do optimization here
          //Actually, this was done in clientSend()

          //Do the message storage here before forwarding
          //I may need to do it in the forward send function

          //Forward messages
          forwardSend(buffer2Send, socketDescriptor, Partners, tokens,numberOfTokens, myLoc, myPort, storedBuffer);
    
        }
      }
    }  
  }
  return 0;
}
							       
int mainClient(int sd, struct _partners *Partners, int myPort, int *myLoc){
  //Used to check for errors
  int returnCode;

  //Clear partners struct
  memset (Partners, 0, sizeof(struct _partners));
  Partners->maxHosts = 0;
  
  //Read contents of config file
  returnCode = readConfigFile(Partners, myPort, myLoc);

  //Print out prot # and location
  printf ("my port is %d\n,myLoc is %d\n", myPort, *myLoc);

  //Print everything in partners struct
  printPartners (Partners);

  return 0; 
}

void clientSend(char *buffer, int sd, struct _partners Partners, int myPort){
  //Declare variables
  int i;
  struct sockaddr_in server_address; 

  #ifdef LAB6_TESTING
  printf ("I am sending '%s'", buffer);
  printf ("the length of the string is %lu bytes\n", strlen(buffer));
  #endif

  // I want to send this to each partner. Probably don't need the command line port/IP
  for (i = 0;i < Partners.maxHosts;i++){
    //Declare more variables
    int portNumber;
    char serverIP[20];

    //Clear variable before using
    memset (serverIP, 0, 20); 

    //Copy port # to variable
    portNumber = Partners.hostInfo[i].portNumber;

    //Copy ipaddr to variable
    strcpy(serverIP, Partners.hostInfo[i].ipAddress);
    
    //Setup server addr info
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);

    if(portNumber == myPort){
      //Don't send packet to myself
      continue;
    }
    else{
      //Sent message info to other servers      
      sendData(buffer, sd, server_address);
    }
  }    
} 
 
/******************************************************************/
/* this function actually does the sending of the data            */
/******************************************************************/
int sendData(char *buffer, int socketDescriptor, struct sockaddr_in server_address){
  //User to check for errors
  int returnCode = 0;
  
  //Send message to server
  returnCode = sendto(socketDescriptor, buffer, strlen(buffer), 0,
	      (struct sockaddr *) &server_address, sizeof(server_address));

  //Check to see if the entire message was sent to the server
  if (returnCode < strlen(buffer)){
    perror ("Unable to send message (sendData sendto())");
    exit(1);
  }

  return (0); 
}


/******************************************************************/
/* this function will ask the user for the name of the input file */
/* it will then open that file and pass pack the file descriptor  */
/******************************************************************/
FILE * openFile (){
  FILE * fptr = NULL; 
  char fileName [100]; // this will be given by user
  while (1){
    memset (fileName, 0,100); // always blank the buffer
    printf ("What is the name of the file with messages in it? ");
    char *ptr = fgets(fileName, sizeof(fileName), stdin);
    if (ptr == NULL){
      perror ("fgets");
      exit (1);
    }

    ptr = rtrim(ptr);
    if (ptr == NULL){ // error occured                                                                  
      printf ("you didn't enter anything, try again.\n");
    }
    else{
      fptr = fopen (fileName, "r");
      if (fptr == NULL){
        printf ("error opening the file, try again\n");
        continue; //bad bad bad
      }
      return fptr;
      break; // stop looping
    }
  } // end of the forever loop!
}// end of the function

/* this trims characters from a string */
char *rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}

/***********************************************************************/
/* this function reads in the config file. The filename is config.file */
/* it reads this data into a structure called partners.. we can change */
/* this structure so that it matches our needs                         */
/***********************************************************************/
int readConfigFile(struct _partners *Partners, int myPortNumber, int *myLoc){
  // open the file
  FILE *configFile = fopen("config.file", "r"); // hardcoded name
  char *line = NULL;  // weird how getline() works?
  ssize_t returnCode = 0;
  size_t bytesRead = 0;
  char ipaddr[25]; // assume IPv4
  char *ptr = &ipaddr[0]; 
  int port, location; 

  // did we open it?
  *myLoc = 0; // have to find me in the file
  if (configFile == NULL){
    printf ("couldn't open the config.file\nMake sure that file exists\n");
    exit (1);
  }

  // logic here; read a line, then loop to tokenize it
  // if i ever change the format of the config file, i would have to
  // redo this section of code.
  // you read the line here first, because it is possible there is
  // nothing in the file!!
  
  returnCode = getline(&line, &bytesRead, configFile); // prime the pump
  while (returnCode >= 0 ){
    //Tokenize the file
    ptr = strtok(line, " "); 

    //Isolate port #
    port = atoi(strtok(NULL, " "));

    //Check if IP addr is valid
    if (1 == validIP(ptr)){
      printf ("Invalid IP address. Skip this line\n");
      printf ("%s\n", line);
      exit(1);
    }
    location = atoi(strtok(NULL, "\n"));
    if (myPortNumber == port){
      *myLoc = location;
    }
    Partners->hostInfo[Partners->maxHosts].portNumber = port;
    Partners->hostInfo[Partners->maxHosts].location = location;
    strcpy (Partners->hostInfo[Partners->maxHosts].ipAddress, ptr);
    
    //Add additional information for lab 6
    Partners->hostInfo[Partners->maxHosts].currentSeqNumber = 0;
    int i;
    for (i=0; i<MAXSEQNUMBER;i++){
      Partners->hostInfo[Partners->maxHosts].hasAcked[i] = false;     
      Partners->hostInfo[Partners->maxHosts].sentAck[i] = false;     
    }
    Partners->hostInfo[Partners->maxHosts].location = location;
    Partners->maxHosts ++;
    returnCode = getline(&line, &bytesRead, configFile); // read the next line

    #ifdef LAB6_TESTING
    printf ("rc is %d\n", (int)rc);
    #endif
  }
  return 0;
}


/* in this function i can print out all my partners. I use this */
/* mainly to test that i am reading them in correctly.          */

void printPartners (struct _partners *Partners){
  //Declare iterator
  int i = 0;

  /* the partners structure tells me how many partners/hosts i have */
  while (i<Partners->maxHosts){ // do it for each entry. 
    printf ("ipaddress '%s', port '%d', location '%d', host '%d' \n",
	    Partners->hostInfo[i].ipAddress, //IP address
	    Partners->hostInfo[i].portNumber, //port Number
	    Partners->hostInfo[i].location, //location
	   i);
    i++;  // increment i
  }
  
}
int validIP(char *anIPAddress){
  //Declare struct variable inaddr to check address
  struct sockaddr_in inaddr;

  //Check if IP Address is valid
  if (!inet_pton(AF_INET, anIPAddress, &inaddr)){
    //Exit if IP address is not valid
    printf ("Error, bad IP address\n");
    exit (1); 
  }

  return 0;
}

/******************************************************************/
/* this function will create a socket and fill in the address of  */
/*  the server                                                    */
/******************************************************************/
void makeSimpleSocket(int *sd){

  /* first create a socket */
  *sd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */

}

/******************************************************************/
/* this function will create a socket and fill in the address of  */
/*  the server                                                    */
/******************************************************************/
 void makeSocket(int *socketDescriptor, char *argv[], struct sockaddr_in *server_address, int* myPort){

  //int i; // loop variable
  int portNumber; // get this from command line
  int returnCode; // always need to check return codes!
  
  //Create a socket
  *socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */

  //Check if there is an error
  if (*socketDescriptor == -1){ /* means some kind of error occured */
    perror ("socket");
    exit(1); /* just leave if wrong number entered */
  }

  //Check if port # is a number
  validNum(argv[1]);

  //Assign value from argv to variable
  portNumber = strtol(argv[1], NULL, 10); /* many ways to do this */
  *myPort = portNumber; // pass it back

  //Check if port number is valid
  checkPortNum(portNumber);

  //Fill in address data
  server_address->sin_family = AF_INET; /* use AF_INET addresses */
  server_address->sin_port = htons(portNumber); /* convert port number */
  server_address->sin_addr.s_addr = INADDR_ANY; /* any adapter */
  
  //Bind to address
  returnCode = bind (*socketDescriptor, (struct sockaddr *)server_address, sizeof(struct sockaddr ));
  if (returnCode < 0){
    perror("bind");
    exit (1);
  }

}
/*******************************************************************/
/* this function reads a line of data from the file                */
/*******************************************************************/

void receiveMessage (char *buffer,int socketDescriptor, struct sockaddr_in *from_address){
  int returnCode = 0;
  int messageLength;
  int flags = 0; // using 0 since I don't need anything special 
  
  /* this next variable is MANDATORY and must be set on linux systems */
  socklen_t fromLength = sizeof (struct sockaddr); // used in recvfrom
  
  //Clear buffer
  memset (buffer, 0, 1000); 
  
  //Receive message from client and assign to RC to check status
  returnCode = recvfrom(socketDescriptor, buffer, 1000, flags, (struct sockaddr *)from_address, &fromLength);
  
  //Assign message length
  messageLength = returnCode;

  //Affix null char to the end of the message
  buffer[messageLength] = '\0';

  /* check for any possible errors */
  if (returnCode <=0){
    perror ("recvfrom");
    printf ("leaving, due to socket error on recvfrom\n");
    exit (1);
  }

} 

/*******************************************************************/
/* this function will find all the tokens:value pairs              */
/*******************************************************************/

int findTokens(char *buffer , struct _tokens *tokens){
  
  int counter= 0; // how many tokens did i find!
  char * ptr;

  /* recall the protocol says the key:value are separated by ' ' */
  /* so i will tokenize first on the ' '                         */
  
  ptr = strtok (buffer," "); 
  while (ptr != NULL) // keep going until no more ' ' 
    {
      memset (tokens[counter].key, 0, 100);
      memset (tokens[counter].value, 0, 100);

      int i = 0; // KMT would be laughing
      int flag = 0;
      int k = 0; 
      for (i=0;i<strlen(ptr);i++){
        if (flag ==0){ // doing the key portion
          if (ptr[i] != ':')
            tokens[counter].key[i] = ptr[i];
          else // must be a ':'
            flag = 1;
        }
        else{ // doing the value portion
          if (ptr[i] == '^'){ // undoing the cleansing of msg portion
            ptr[i] = ' ';
          }
          tokens[counter].value[k] = ptr[i];
          k++;
        }
      }
      
      ptr = strtok (NULL, " ");
      counter ++;
    }
  return counter;
}

/*******************************************************************/
/* this function will find the msg (if it exists) and encode it    */
/* so it won't tokenize wrong                                      */
/*******************************************************************/
void cleanse (char * buffer){
  char *startPtr;
  int count = 0;
  int i;
  startPtr = strstr (buffer, "\"");

  /* replace the ' ' in the string with & so tokenize will work */
  if (startPtr != NULL){
    for(i=0; i<strlen(startPtr) && count < 2 ;i++){
      if (startPtr[i] == '"'){
	count ++;
      }
      else if (startPtr[i] == ' '){
	startPtr[i] = '^';
      }
    }
  }
}
 
/*******************************************************************/
/* this function will print out all the tokens                     */
/*******************************************************************/
void printTokens (struct _tokens *tokens, int numberOfTokens, int myLoc){
  int i; // loop index
  
  //Print out tokens
  printf ("****************************************************\n");
  printf ("%20s %20s\n", "Name", "Value");

  //Loop through tokens struct
  for (i=0;i<numberOfTokens;i++){
    printf ("%20s %20s\n", tokens[i].key, tokens[i].value);
  }
  //Add my location
  printf ("%20s %20d\n", "myLocation", myLoc);
  printf ("****************************************************\n");

}

/*******************************************************************/
/* this function will find and return int token value              */
/*******************************************************************/
//Perhaps the most useful function Dr. Ogle gave
int findIntToken (struct _tokens *tokens, int numberOfTokens, char *keyWanted){
  int i; // loop index

  for (i=0;i<numberOfTokens;i++){
    if (!strcmp(tokens [i].key, keyWanted))
      return (atoi(tokens[i].value)); // means i found the port
  }
  return -1; 

}

//Return element number of a particular token
int findTokenElementNum(struct _tokens *tokens, int numberOfTokens, char *keyWanted){
  int i; // loop index

  #ifdef LAB5_TESTING
  printf("Number of Tokens is %d\n", numberOfTokens);
  #endif

  for (i=0;i<numberOfTokens;i++){
    //printf("Tokens[%d.key is %d\n", i, tokens[i].key);
    if (!strcmp(tokens [i].key, keyWanted))
      return i; 
  }
  return -1; 
}

/*******************************************************************/
/* this function will find and return int token value              */
/*******************************************************************/
char * findCharToken (struct _tokens *tokens, int numberOfTokens, char *keyWanted){
  int i; // loop index

  for (i=0;i<numberOfTokens;i++){
    if (!strcmp(tokens [i].key, keyWanted))
      return (tokens[i].value); // means i found the port
  }
  return NULL; 

}

int isWithinRange(int rowSize, int columnSize, int otherLocation, int myLocation){
  // Declare variables
  int loc1X = -1, loc1Y = -1, loc2X = -1, loc2Y = -1, distance = -1;

  // Calculate the X and Y coordinates of location 1
  loc1X = (myLocation - 1) / columnSize + 1; 
  loc1Y = (myLocation - 1) % columnSize + 1;

  // Calculate the X and Y coordinates of the partner location
  loc2X = (otherLocation - 1) / columnSize + 1;
  loc2Y = (otherLocation - 1) % columnSize + 1;

  // Calculate distance between locations using the distance formula
  distance = trunc(sqrt(pow((loc2X - loc1X), 2) + pow((loc2Y - loc1Y), 2)));

  return distance;
}

//Broken down version of isWithinRange
//Works with Dr. Ogle's code, so let's use this for now
int findXYCoordinates(int squareNumber, int *row, int *column){
  *row = (squareNumber-1) / COLUMNS +1;
  *column = (squareNumber-1)%COLUMNS+1;
  #ifdef LAB5_TESTING 
  printf ("your row/column is %d/%d\n",*row, *column);
  #endif
  if (*row > ROWS || *column > COLUMNS){
    printf ("your squareNumber is from outside the boundary\n");
    return (-1);
  }
  return (1); 
}

int findDistance(int x, int y, int myX, int myY){
  int distance;
  distance = trunc(sqrt(pow((x-myX),2) + pow((y-myY),2)));
  return distance;
}

//Function updated to only update ttl, not return it.
//Resolves error where "checkTTL doesn't return anything"
//Decrement TTL
//Somehow, this doesn't work with the send-path
void updateTTL(struct _tokens *tokens, int numberOfTokens, char *buffer){
  int i, ttl = 0;
  for(i = 0;i < numberOfTokens;i++){
    //Find TTL token
    if(strcmp(tokens[i].key, "TTL") == 0){
      //Update ttl value based on what's in the tokens struct
      ttl = atoi(tokens[i].value);
      //Decrement
      ttl--;

      //I'm getting into an infinite loop. Need to fix
      if(ttl <= 0){
        sprintf(tokens[i].value,"%d", ttl);
        return;
      }

      //Add ttl back to tokens struct
      sprintf(tokens[i].value,"%d", ttl);
    }
  }
  return;
}

//Return TTL value
int returnTTL(struct _tokens *tokens, int numberOfTokens){
  int i, ttl = 0;
  for(i = 0;i < numberOfTokens;i++){
    //Find TTL token
    if(strcmp(tokens[i].key, "TTL") == 0){
      //Update ttl value based on what's in the tokens struct
      ttl = atoi(tokens[i].value);
    }
  }
  return ttl;
}

//Removing for now
//So much stress
/*
int recreateBuffer(char *buffer, int numberOfTokens, struct _tokens *tokens, int myLocation, int myPort, struct _partners *Partners){
  //Clear buffer
  memset(buffer, 0, 1000);

  int i;
  for(i = 0; i < numberOfTokens; i++){

    //Compare token with "senderLocation"
    if(strcmp(tokens[i].key, "senderLocation") == 0){
      #ifdef LAB5_TESTING
      printf("Found sender location\n");
      #endif

      //Update the location in tokens struct
      updateLocation(Partners, tokens, i, myPort);
    }

    //Look for "sendPath" in tokens
    if(strcmp(tokens[i].key, "sendPath") == 0){
      #ifdef LAB6_TESTING
      printf("Found send path\n");
      #endif

      char currentPathToken[100];

      memset(currentPathToken, 0, 100);

      #ifdef LAB6_TESTING
      printf("value of tokens[%d].value is %s\n", i, tokens[i].value);
      //copy token into temp buffer
      strcpy(currentPathToken, tokens[i].value);
      //sprintf(currentPathToken, "%s", currentPathToken);
      printf("current path token is %s\n", currentPathToken);
      #endif

      //Update the send path in the tokens
      updateSendPath(Partners, myPort, numberOfTokens, tokens);

    }
    
    //sprintf(buffer, "%s:%s%s", tokens[i].key, tokens[i].value, " "); //This is constantly overwritting the buffer
    //Read tokens to buffer in the order of key, value, and space
    strcat(buffer, tokens[i].key);
    strcat(buffer, ":");
    strcat(buffer, tokens[i].value);
    strcat(buffer, " ");

    #ifdef LAB5_TESTING
    printf("Buffer inside of loop is %s\n", buffer);
    #endif
  }
  #ifdef LAB5_TESTING
  printf("Buffer outside of loop is %s\n", buffer);
  #endif

  return 0;
}
*/

//This function works, although I can't seem to place a comma
//between the various port numbers without creating an issue
/*
int updateSendPath(struct _partners *Partners, int myPort, int numberOfTokens, struct _tokens *tokens) {
    #ifdef LAB5_TESTING
    printf("In fxn 'updateSendPath,' my port # is %d\n", myPort);
    #endif

    // Convert myPort to string
    char portNum[6];
    sprintf(portNum, "%d", myPort);

    // Find the index of the "sendPath" token in the tokens array
    int sendPathIndex = findTokenElementNum(tokens, numberOfTokens, "sendPath");
    if (sendPathIndex == -1) {
        // If the "sendPath" token was not found, return an error code
        return -1;
    }

    // Check if current send path already includes current port number
    char* currentPathToken = tokens[sendPathIndex].value;
    if (strstr(currentPathToken, portNum) != NULL) {
        // If current send path already includes current port number, exit function
        return 0;
    }

    // Append my port number to the existing send path
    //strcat(currentPathToken, ", ", portNum);
    strcat(currentPathToken, portNum);
    //strcat(tokens[sendPathIndex].value, ", ");
    //strcat(tokens[sendPathIndex].value, portNum);
    //sprintf(currentPathToken, "%s, %d", currentPathToken, myPort);
    //sprintf(currentPathToken, "%s, %d", currentPathToken, myPort);

    return 0;
}
*/

//Couldn't get this function to work properly
/*
void sendACK(int fromPort, int toPort, struct sockaddr_in *from_address, struct _partners *Partners, int socketDescriptor){
  char buffer[1000];
  memset(buffer, 0, 1000);

  // Add the ACK message to the buffer
  sprintf(buffer, "type:ACK fromPort:%d toPort:%d", fromPort, toPort);

  // Send the ACK to each partner
  clientSend(buffer, socketDescriptor, *Partners);

  // Update the sentAck value for the corresponding host
  for (int i = 0; i < Partners->maxHosts; i++) {
    if (Partners->hostInfo[i].portNumber == fromPort) {
      int seqNum = Partners->hostInfo[i].currentSeqNumber;
      Partners->hostInfo[i].sentAck[seqNum] = true;
      printf("Sent ACK\n");
      break;
    }
  }
}
*/

//Using this in place of the updateTTL(), recreateBuffer(), and clientSend() 
void forwardSend(char *buffer, int sd, struct _partners Partners, struct _tokens *tokens, int numberOfTokens,int myLoc, int myPort, struct _messages *storedBuffer){
  //Declare variables
  int i;
  struct sockaddr_in server_address; 

  #ifdef LAB5_TESTING
  printf ("I am forwarding\n");
  #endif

  char tempBuffer[100];
  //Clear buffer
  memset(buffer, 0, 500); 

  //Decrement the TTL and add it back to the tokens struct
  //Yikes, what an issue this causes
  //updateTTL(tokens, numberOfTokens, buffer);
  
  //Decrement ttl
  for (i=0;i<numberOfTokens;i++){
    //handle the TTL field first
    if (strcmp(tokens[i].key, "TTL") == 0){
      int ttl=0;
      ttl = atoi(tokens[i].value);
      ttl = ttl -1;
      if (ttl <=0){
        printf ("message has no more time!\n");
        return;
      }
      sprintf (tokens[i].value,"%d", ttl);
    }
    
    //Handle changing the location 
    if (strcmp(tokens[i].key, "location") == 0){
      sprintf (tokens[i].value, "%d", myLoc);
    }
    //Send-path
    if (strcmp(tokens[i].key, "send-path") == 0){
      //printf("send-path is as follows:\n");
      //printf(tokens[i].value);
      sprintf (tokens[i].value, "%s,%d", tokens[i].value, myPort);
    }
    
    //Reinput values into buffer
    sprintf (tempBuffer, "%s:%s%s", tokens[i].key, tokens[i].value, " ");
    strcat(buffer, tempBuffer);
    memset(tempBuffer, 0, 100);
  }
  
  int isDup = -1;
  //isDup = isDuplicate(buffer, Partners, tokens, storedBuffer);

  //Store buffer into a messages struct
  if(isDup != 1){
    printf("Storing message\n");
    memset(storedBuffer[messagesIndex].message, 0, 200);
    strcpy(storedBuffer[messagesIndex].message, buffer);
    #ifdef LAB8_TESTING
    printf("Stored message is %s\n", storedBuffer->message);
    #endif

    //Assign hops remainig value
    storedBuffer[messagesIndex].hopsRemaining = 5;
    #ifdef LAB8_TESTING
    printf("Hops remaining is %d\n", storedBuffer->hopsRemaining);
    #endif

    //Iterate index
    messagesIndex = messagesIndex + 1 % STOREDMESSAGETOTAL;
  }
  

  // get rid of the last ' ' in the message
  //If only I knew this 3 labs ago
  buffer[strlen(buffer)-1] = 0;

  #ifdef LAB6_TESTING
  printf("Forwarding message '%s'\n", buffer);
  #endif

  // I want to send this to each partner. Probably don't need the command line port/IP
  for (i=0;i<Partners.maxHosts;i++){
    int portNumber;
    char numPort[20];
    char serverIP[20];
    char *ptr;
    char sendPath[100];

    //Clear buffer
    memset (serverIP, 0, 20);
    memset (numPort, 0, 20);

    //Copy port #
    portNumber = Partners.hostInfo[i].portNumber;

    //Copy sendPath
    if (strcmp(tokens[i].key, "send-path") == 0)
      strcpy(sendPath, tokens[i].value);

    //Copy int to num
    sprintf(numPort, "%d", portNumber);
    #ifdef LAB7_TESTING
    printf("Converted port is %s\n", numPort);
    printf("Original port is %d\n", portNumber);
    #endif

    //Determine if port # is found in send-path
    ptr = strstr(sendPath, numPort);

    //Clever code - don't forward to myself
    if (portNumber == myPort){
      continue;
    }
    //Copy ipaddr
    strcpy(serverIP, Partners.hostInfo[i].ipAddress);

    //Assign server data
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = inet_addr(serverIP);  

    //Send out messages if they're not in the send-path 
    if(ptr != NULL){
      #ifdef LAB7_TESTING
      printf("Skipping the ports in the sendpath\n");
      sendPathCount++;
      #endif

      continue;
    }   
    else{
      #ifdef LAB7_TESTING
      printf("Forwarding out data since port is not in sendpath\n");
      #endif

      sendData(buffer, sd, server_address);
    } 
  }
  #ifdef LAB7_TESTING
  printf("Total ports skipped are %d\n", sendPathCount);   
  #endif 
} 

int findSeqNumber(int toPort, struct _partners *Partners){
  //Declare iterator
  int i;

  //Go through partners to find the seq #
  for (i=0;i<Partners->maxHosts;i++){
    //Found port match
    if (Partners->hostInfo[i].portNumber == toPort){
      //Increment
      Partners->hostInfo[i].currentSeqNumber ++;

      //Return updated seq #
      return Partners->hostInfo[i].currentSeqNumber;
    }
  }
  return (-1);
}

int findPartner(int sourcePort, struct _partners Partners){
  //Iterator
  int i;

  //Go through partners
  for(i=0; i<Partners.maxHosts; i++){
    //Port match
    if (Partners.hostInfo[i].portNumber == sourcePort){
      //Return index
      return i;
    }
  }
  return -1;
}

int validNum(char *aNumber){
  //Iterator
  int i;
  
  //Go through array, which is a number
  for (i=0;i<strlen(aNumber); i++){
    //Check if the value is a digit
    if (!isdigit(aNumber[i])){
      printf ("This isn't a number!\n");
      exit(1);
    }
  }
  return 0;
}

//Check if the port # is within the valid range
void checkPortNum(int portNumber){
  //If port number isn't valid, exit function
  if ((portNumber > 65535) || (portNumber < 0)){
    printf ("You entered an invalid socket number!\n");
    exit(1);
  }
}

void reSend(struct _messages *storedBuffer, int sd, struct _partners Partners, int myPort, int myLoc){
  int i;
  for(i = 0; i < STOREDMESSAGETOTAL; i++){
    if(storedBuffer[i].hopsRemaining == 0){
      printf("Hops remaining equals zero for message %d. Leaving fxn\n", i);
      return;
    }
    else{
      char buffer[200];
      
      //Causing a seg fault
      //printf("Move command is %c\n", moveCmd);

      //Copy data from messages struct to buffer
      strcpy(buffer, storedBuffer->message);

      #ifdef LAB8_TESTING
      //Test that it worked
      printf("Copied buffer in resend fxn is %s\n", buffer);

      printf("Hops begin at %d\n", storedBuffer->hopsRemaining);
      #endif

      //Decrement TTL in struct
      storedBuffer->hopsRemaining--;

      #ifdef LAB8_TESTING
      //Test that decrement was successful
      printf("Decremented hops is now %d\n", storedBuffer->hopsRemaining);
      #endif

      //Send out
      clientSend(buffer, sd, Partners, myPort);
    }
  }
}

int isDuplicate(char message, struct _partners Partners, struct _tokens *tokens, struct _messages *storedBuffer){
    // Extract the toPort, fromPort, and sequence number from the message
    int seqNum, toPort, fromPort;
    int numberOfTokens;
    numberOfTokens = findTokens(&message, tokens);

    #ifdef LAB8_TESTING
    //Print tokens tokens to make sure it worked
    printf("Printing tokens in isDuplicate fxn\n");
    int i;
    for (i=0;i<numberOfTokens;i++){
      printf ("%20s %20s\n", tokens[i].key, tokens[i].value);
    }
    #endif

    int i;
    for (i=0;i<numberOfTokens;i++){
      if(strcmp(tokens[i].key, "toPort") == 0){
        toPort = atoi(tokens[i].value);
      }
      if(strcmp(tokens[i].key, "fromPort") == 0){
        fromPort = atoi(tokens[i].value);
      }
      if(strcmp(tokens[i].key, "seqNumber") == 0){
        seqNum = atoi(tokens[i].value);
      }
    }
    //Print out values
    #ifdef LAB8_TESTING
    printf("seqNum is %d\n", seqNum);
    printf("toPort is %d\n", toPort);
    printf("fromPort is %d\n", fromPort);
    #endif

    int j;
    int dupFlag = -1;
    for(i = 0; i < STOREDMESSAGETOTAL; i++){
        char* storedMessage = storedBuffer[i].message;
        
        numberOfTokens = findTokens(storedMessage, tokens);

        for(j = 0; j < numberOfTokens; j++){
          if (toPort == atoi(tokens[i].key)){
            printf("Duplicate toPort found\n");
            dupFlag = 1;
          }
          if(fromPort == atoi(tokens[i].key)){
            printf("Duplicate fromPort found\n");
            dupFlag = 1;
          }
          if(seqNum == atoi(tokens[i].key)){
            printf("Duplicate seq num found\n");
            dupFlag = 1;
          }
        }
    }

    if(dupFlag == 1){
      printf("Duplicate present. Don't store\n");
      return 1;
    }
    
    return 0; // No duplicates found
}
