This project implements a trivial file transfer protocol over Connectionless UDP. It handles basic level of acknowledgement between every data transfer. It supports only RRQ, read request and each file requested by the client is sent in chunks of 512 bytes with header information.

It reports error code when file is not found at server executing folder. In the case of delayed acknowledgement > 2 secs the data is retransmitted.

The server is designed to handle multiple client requests, individually or concurrently by forking itself. It can handle any size of file. The implementation carries data in packet formats dictated by the tftp protocol.	

	To run the progam, users are supposed to open one terminal for the server. They can use any tftp client. On linux try the command below
	
	Typing "./tftpserver.c [ipv4address] [portNmbr]" with 2 arguments to run the server side program. 
	Typing "tftp [server's address] [server's port] -m octet -c get [server's filename] [new filename]" to run the client side program.
	