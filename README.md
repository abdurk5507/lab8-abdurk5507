***READ ME***

*******************************************************
*  **Name      :**  Abdur Khan
*  **Student ID:**  105071067
*  **Class     :**  CSCI 3762           
*  **HW#       :**  Lab 8         
*  **Due Date  :**  05/03/2023
*******************************************************

This program has one file, drone8.c. This file serves the purpose of both a client and a server. This program will send messages across the network and receive and print those messages if it meets three conditions:

- (1): The port number is destined for my program
- (2): The location is within 2 sqaures
- (3): The TTL is greater than 0

This program will also send and receive ACKs, provide a send-path of the message, and increment with sequence numbers. The move command has also been implemented along with optimization.

**Status of Program**<br>
This program works in most scenarios. The move command works as described, but it's not clear if the optimization is working as it should.

My program, however, seems to print the intiialy "enter the string you'd like to send" message many times. I am not certain as to why this occurs.

**Files Needed to Run Program**
- drone8.c - this is the server and client file.
- config.file - this file provides the IP address, port numbers, and locations to send messages to. 
- makefile - file that allows the use of the make utility. 

**How to Execute**
1. Clone the git repository on your machine.
2. Run the following command on your CLI: "make"
3. Execute the code by doing the following: "./drone8 <PORT NUMBER> <ROW> <COLUMN>
 	- Be sure to enter a port number within the valid range where it says "PORT NUMBER" above. Also enter the desired row and column size.
	- Also be sure to create multiple drones in order to confirm that the messages are received and forwarded. The same command above can be run using an alternative port number.
4. Enter the number of desired rows and columns
5. Enter the message
6. Await messages to be received by other drone. Messages destined for the port # specified, within the distance of 2, and with a TTL greater than 0 will be printed.
7. Run the following command to remove the executable files: "make clean"


