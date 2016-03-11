#!/usr/local/bin/python

# Barbara Hazlett
# CS 372 
# 3/3/2015
# Program #2 - 2 connection client-server network application

# This program is the file transfer client. It takes a server host and port, a command (list or get),
# data port and filename (if applicable) from the user and initiates a connection on the server port.
# It then receives the directory or a file from the server based on the command entered via the dataport.  

# Help sources:
# http://stackoverflow.com/a/2560472/1400198 for initiate contact function
# https://github.com/KamalChaya/ftp-with-socket-programming was used for creating
# the socket and program structure and especially code for getting the file
# also leveraged python client-server program from 'Computer Networking, A Top-Down Approach'
# by Kurose and Ross (section 2.7)

import socket
import sys
import errno
import os
import time
import getopt

MAX_ATTEMPTS = 5

#****************************************************************************************
# ** Function: createSocket
# ** Description: Returns a TCP socket or error message if cannot be created.
# ** Input Parameters: none
# ** Output:  Returns a TCP socket.
#****************************************************************************************
def createSocket():
	try:
		return socket.socket(socket.AF_INET, socket.SOCK_STREAM) #Create the socket
	except socket.error:
		print "Error in socket creation"
		sys.exit(1)
		
#****************************************************************************************
# ** Function: initiate_contact
# ** Description: Attempts to establish a TCP data connection on servNum.  Tries five
# ** times and gives error if can't connect.
# ** Input Parameters: servSocket, hostName, servNum
# ** Output:  TCP data connection or error 
#****************************************************************************************
def initiateContact(servSocket, hostName, servNum):
	for attempt in range(MAX_ATTEMPTS):
		try:
			servSocket.connect((hostName, int(servNum)))
			print ('client: connected to a control connection over port ' + servNum)
		except EnvironmentError as exc:
			if exc.errno == errno.ECONNREFUSED:
				print 'Connection failure; trying again in 1 second...'
				time.sleep(1)
			else:
				raise
		else:
			break
	else:
		raise RuntimeError("Max number of unsuccessful attempts reached")
	
#****************************************************************************************
# ** Function: makeRequest
# ** Description: sends command line info to server, gets response and calls functions to 
# ** list directory or get the requested file
# ** Input Parameters: servSocket, cmd, dataPort
# ** Output:  none
#****************************************************************************************	
def makeRequest(servSocket, cmd, fileName, dataPort):    
    #send the command that the user passed in 	
	servSocket.send(dataPort + " " + cmd + " " + socket.gethostname() + " " + fileName)	
	response = servSocket.recv(100)
	print response
	
	if 'valid' in response:
		dataSocket = createSocket() #Create data socket
		if 'list' == cmd:
			#get the directory from the server and display on screen
			receiveList(servSocket, dataSocket, dataPort)
            
        if 'get' == cmd:
            #get file from the server
            receiveFile(servSocket, dataSocket, fileName, dataPort)
	else:
		dataSocket.close()
        servSocket.close()
        return

#****************************************************************************************
# ** Function: receiveList
# ** Description: receives and prints the directory sent by the server to screen
# ** Input Parameters: servSocket, dataSocket, dataPort
# ** Output: none
#****************************************************************************************		
def receiveList(servSocket, dataSocket, dataPort):
	dataSocket.bind(('',int(dataPort)))
	dataSocket.listen(1)
	#servSocket.send("valid cmd received")
	
	connectionSocket, addr = dataSocket.accept()
	received = connectionSocket.recv(500)
	print received
	
	connectionSocket.close()
	dataSocket.close() #is this needed?
	servSocket.close()  
   		
#****************************************************************************************
# ** Function: receiveFile
# ** Description: receives the file sent by the server
# ** Input Parameters: servSocket, dataSocket, fileName, dataPort
# ** Output: none
#****************************************************************************************
def receiveFile(servSocket, dataSocket, fileName, dataPort): 
	dataSocket.bind(('',int(dataPort)))
	dataSocket.listen(1)
	#servSocket.send("valid cmd received")
            
	connectionSocket, addr = dataSocket.accept()
	received = connectionSocket.recv(1024)
	if received == 'Error: FILE NOT FOUND':
		print received + '\n'
		sys.exit(1) #Kernel will close sockets for us
                
	if received == 'small':
		connectionSocket.send("transfer")
		small = connectionSocket.recv(1500)
		print 'File transfer complete.\n'
				
	if os.path.isfile(fileName):
		response = raw_input(fileName + " exists.  Do you want to overwrite? (y/n) ")
		if response != 'y':
			print 'File will not be saved\n'
			connectionSocket.close()
			servSocket.close()
			return
					
	with open(fileName, 'w') as out:
		out.write(small)
				
	print 'The file has been saved\n'
	#return
	connectionSocket.close()
	dataSocket.close() #is this needed?
	servSocket.close()  
			
def main():
	#get hostname, dataport, controlport, command and filename (if applicable)
	fileName = "bad";
	cmd = "bad";
	if len(sys.argv) == 5:
		servName = sys.argv[1]
		servPort = sys.argv[2]		
		dataPort = sys.argv[4]
		if sys.argv[3] == '-l':
			cmd = "list"
	elif len(sys.argv) == 6:
		servName = sys.argv[1]
		servPort = sys.argv[2]		
		dataPort = sys.argv[5]
		if sys.argv[3] == '-g':
			cmd = "get"
			fileName = sys.argv[4]
	else:
		print 'usage: python ftclient.py [server-host][server-port] [-l or -g <filename> for list or get] [data-port] \n'
		sys.exit(1)	
	     
	#add extension if user only input flip1, flip2, etc
	if '.engr.oregonstate.edu' not in servName:
		servName = servName + '.engr.oregonstate.edu'
    			
	#create connection socket
	servSocket = createSocket()
	
	#connect and initiate contact
	initiateContact(servSocket, servName, servPort)
	
	#send command to server
	makeRequest(servSocket, cmd, fileName, dataPort)		
	
if __name__ == "__main__":
    main()	


	
	

	