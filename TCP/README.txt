	This project can be divided into two source code files. One is called "tcpserver.c", which is the running program of server side.And another one is called "tcpclient.c", which is the running program of client side.
	In the "tcpserver.c", the server uses socket(), bind(), listen() accept()functions to establish a TCP connection with client side. As the server is required to connect with multiple clients simutaneouly, a select() function is used to check multiple sockets. The select() is replaced at the while loop where it constantly check all the socket descriptors. The server and client send message to each other using send() function, and receive message from each other using recv(). 
	In the "tcpclient.c", the server uses socket(), connect() functions to establish a TCP connection with server side. In the while loop of the client program, scanf is used to read keyboard input which contains command: "JOIN" or "SEND", username of the connecting client, message. 
	A structure named "struct sbcp_message" has been made to implement the SBMP protocol for socket communication in this program.
	

	To run the progam, users are supposed to open several terminals. 
	
	Typing "./tcpserver.c [ipv4address] [portNmbr] [Maxclients]" with 3 arguments to run the server side program. 
	Typing "./tcpclient.c [ipv4address] [portNmbr]" with 2 arguments to run the client side program.
	A client setup the connection with server by inputting "join [usrname]" from keyboard, send message to other client by inputting "send [message]. If the client setup with a name that has been used by other client, the setup will be denied.  The server will communicate back to the client through commands fwd, ack and nak of sbcp protocol