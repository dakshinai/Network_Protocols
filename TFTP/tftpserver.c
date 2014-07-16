#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include "tftp_struct.h"

void sigchld_handler(int s) {
	while (wait( NULL) > 0)
		; /* wait for any child to finish */
}

int recvtimeout(int s, int n_bytes, char *r_buf, char *s_buf, int len,
		int timeout, struct sockaddr *from, socklen_t *fromlen) {
	fd_set fds;
	int n;
	struct timeval tv;
	FD_ZERO(&fds);

	// set up the file descriptor set
	FD_SET(s, &fds);

	// set up the struct timeval for the timeout
	tv.tv_sec = timeout;                         //wait for timeout seconds 
	tv.tv_usec = 0;

	// wait until timeout or data received
	n = select(s + 1, &fds, NULL, NULL, &tv);
	if (n == 0) // timeout!
			{
		sendto(s, s_buf, n_bytes + 4, 0, from, *fromlen);
		n = recvtimeout(s, n_bytes, r_buf, s_buf, 4, 2, from, fromlen); //wait for 2 seconds
		return (n);
	}
	if (n == -1) {
		// error occurred
		perror("Error");
	}

	// data must be here, so do a normal recv()
	return recvfrom(s, r_buf, 4, 0, from, fromlen);
}

int main(int argc, char** argv) {

	static int iport;
	std
	::cout<<"Begin.\n";
	int sfd;
	struct sockaddr_in server_address;
	struct hostent* hret;
	short port;

	struct sockaddr_in client_address;
	uint16_t opcode, blocknum, acknum, ackopcode, rrqopcode;
	socklen_t client_len;

	struct tftp_rrq rrq_msg;
	struct tftp_data data_msg;
	struct tftp_ack ack_msg;
	struct tftp_error error_msg;
	struct sigaction sa; /* deal with signals from dying children! */

	char recv_buf[516] = { };
	int n_bytes = 0;
	char send_buf[516] = { };
	char recv_cmd[4] = { };
	char rrq_filename[50] = { };

	int n = 0;
	int fd;
	int pid;
	using namespace
	std;

	//initialization of structure
	rrq_msg.filename = (char*) malloc(50);
	rrq_msg.mode = (char*) malloc(10);
	data_msg.payload = (char*) malloc(512);
	error_msg.payload = (char*) malloc(50);

	if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Cannot create socket");
		exit(EXIT_FAILURE);
	}

	server_address.sin_family = AF_INET;
	port = atoi(argv[2]);
	iport = port;
	server_address.sin_port = htons(port);
	hret = gethostbyname(argv[1]);
	memcpy(&server_address.sin_addr.s_addr, hret->h_addr, hret->h_length);

	if (bind(sfd, (struct sockaddr*) &server_address, sizeof(struct sockaddr))
			== -1) {
		perror("Cannot bind socket in parent");
		exit(EXIT_FAILURE);
	}
	std
	::cout<<"Bind.\n";

	/* Signal handler stuff */
	sa.sa_handler = sigchld_handler; /* reap all dead processes */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction( SIGCHLD, &sa, NULL) == -1) {
		perror("Server sigaction");
		exit(1);
	}

	while (1) {                                              //the outer while
		if ((n_bytes = recvfrom(sfd, recv_buf, 516, 0,
				(struct sockaddr *) &client_address, &client_len)) == -1) {
			perror("recvfrom");
			exit(0);
		} else {

			pid = fork();                //fork a children process to handle it.
										 //Once it returns in the child process with return value ’0′ (parent's pid)
			//and then it returns in the parent process with child’s PID as return value.

			if (pid == -1) {
				//failure
				continue;
			} else if (pid > 0) {
				//parent process
				continue;
			} else {

				std
				::cout<<"receiving from client\n";
				close(sfd);

				if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
					perror("Cannot create socket");
					exit(EXIT_FAILURE);
				}
				iport++;

				server_address.sin_family = AF_INET;
				server_address.sin_port = htons(iport);
				hret = gethostbyname(argv[1]);
				memcpy(&server_address.sin_addr.s_addr, hret->h_addr, hret->h_length);

				if (bind(sfd, (struct sockaddr*) &server_address,
						sizeof(struct sockaddr)) == -1) {
					perror("Cannot bind socket in child");
					exit(EXIT_FAILURE);
				}

				strcpy(rrq_msg.filename, recv_buf + 2);

				if (recv_buf[1] == 1) {
					std
					::cout<<"It's a read request for pid "<<pid<<endl;
					std
					::cout << "file is "<<(rrq_msg.filename)<< endl;
					if ((fd = open(rrq_msg.filename, O_RDONLY)) == 0) //recv_buf+2
							{
						perror("open");
						exit(0);
					} else if (fd == -1)                  //file not found error
							{
						std
						::cout<<"fd="<<fd<<endl;
						error_msg.opcode = 5;
						error_msg.errorcode = 1;
						error_msg.opcode = htons(error_msg.opcode);
						error_msg.errorcode = htons(error_msg.errorcode);
						strcpy(error_msg.payload, "File not found");

						memcpy(send_buf, &error_msg, 2 * sizeof(uint16_t));
						memcpy(send_buf + 2 * sizeof(uint16_t),
								error_msg.payload, sizeof("File not found"));
						//std::cout<<send_buf+4<<endl;//static_cast<unsigned int>(send_buf[4])
						error_msg.opcode = ntohs(error_msg.opcode);
						error_msg.errorcode = ntohs(error_msg.errorcode);
						n = sendto(sfd, send_buf,
								sizeof("File not found") + 2 * sizeof(uint16_t),
								0, (struct sockaddr*) &client_address,
								sizeof(struct sockaddr));
					} else {
						std
						::cout<<"fd="<<fd<<endl;

						blocknum = 1;
						opcode = 3;
						while (1) {
							std
							::cout<<endl;

							n_bytes = read(fd, data_msg.payload, 512);

							data_msg.opcode = htons(opcode);
							data_msg.blocknum = htons(blocknum);

							memset(send_buf, 0, 516);

							memcpy(send_buf, &data_msg, 2 * sizeof(uint16_t));
							memcpy(send_buf + 2 * sizeof(uint16_t),
									data_msg.payload, n_bytes);
							data_msg.opcode = ntohs(opcode);
							data_msg.blocknum = ntohs(blocknum);

							if (n_bytes > 0) {
								n = sendto(sfd, send_buf,
										n_bytes + 2 * sizeof(uint16_t), 0,
										(struct sockaddr*) &client_address,
										sizeof(struct sockaddr));
								std
								::cout<<"n="<<n<<endl;      //if(blocknum>31999)
								std
								::cout<<"blocknum "<<blocknum<<" sends "<<n<<" bytes"<<endl;
								//wait for 2 seconds
								recvtimeout(sfd, n_bytes, recv_cmd, send_buf, 4,
										2, (struct sockaddr *) &client_address,
										&client_len);
								memcpy(&acknum, recv_cmd + 2, sizeof(uint16_t));
								memcpy(&ackopcode, recv_cmd, sizeof(uint16_t));

								acknum = ntohs(acknum);
								ackopcode = ntohs(ackopcode);

								ack_msg.opcode = ackopcode;
								ack_msg.blocknum = acknum;

								cout << "Ack code is " << ack_msg.opcode
										<< endl;

								if (ack_msg.blocknum == blocknum)
									blocknum++;
								//opcode = 3;

							} else if (n_bytes == 0) {
								n = sendto(sfd, send_buf,
										n_bytes + 2 * sizeof(uint16_t), 0,
										(struct sockaddr*) &client_address,
										sizeof(struct sockaddr));
								//if(blocknum>31999)
								std
								::cout<<"blocknum "<<blocknum<<" sends "<<n<<" bytes"<<endl;
								std
								::cout<<"End of transfer"<<endl;
								break;
							} else {
								perror("Fail to send the block");
								exit(1);
							}
							std
						::cout<<"next blocknum="<<blocknum<<endl;
					}
					//end of inner while

					close(fd);
				}
				//end of of valid fd

				close(sfd);
				exit(0); //end of child

			} //end of check
		} //end of pid switch
	} //end of main if

} //end of outer while
return 0;
}
