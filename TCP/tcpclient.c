#include "tcp_sbcp.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>

int main(int argc, char** argv) {

	int csd;
	struct sockaddr_in addr;
	struct hostent* hret;
	short port = 0;
	int iResult = 0;

	//to modify
	char cmd1[10], cmd2[100];
	char *buf;

	struct sbcp_message *msg, *msg2;

	if ((csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("\nError: Cannot create client socket");
		exit(EXIT_FAILURE);
	}

	hret = gethostbyname(argv[1]);
	port = atoi(argv[2]);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);
	buf = (char*) malloc(sizeof(char) * 5000);

	while (1) {

		scanf("%s %s", cmd1, cmd2);
		switch (cmd1[0]) {
		case 'j':                                            //join JOIN command
		case 'J':
			if (strcmp(cmd1, "join") == 0 || strcmp(cmd1, "JOIN") == 0) {
				if (connect(csd, (struct sockaddr*) &addr,
						sizeof(struct sockaddr)) == -1) {
					perror("\nError: Cannot connect to Server");
				} else {
					//msg = malloc(sizeof(struct sbcp_message));
					//msg->type = 2;    //join

					//memset(msg->payload, 0, sizeof(msg->payload));

					//strncpy(msg->payload, cmd2, strlen(cmd2));

					//if (send(csd, msg, sizeof(msg), 0) < 0)
					if (send(csd, cmd2, sizeof(cmd2), 0) < 0)
						write(1,
								"\nAn exception occured while passing the request",
								sizeof("\nAn exception occured while passing the request"));
					else {
						write(1, "\nSuccess in send client",
								sizeof("\nSuccess in send client"));

						msg2 = malloc(sizeof(struct sbcp_message));

						if (recv(csd, msg2, 5000, 0) < 0)
							//if (read(csd, msg2, 5000) < 0)
							write(1,
									"\nAn exception occured while receiving the response",
									sizeof("An exception occured while receiving the response"));
						else {
							write(1, "\nSuccess in recv client",
									sizeof("\nSuccess in recv client"));

							memset(buf, 0, sizeof(buf));
							strcpy(buf, "\n");
							strcpy(buf, msg2->payload);
							write(1, buf, sizeof(buf));
							memset(buf, 0, sizeof(buf));
						}

						free(msg2);
					}

					//free(msg);

				}
				break;
				/*case 's':
				 case 'S':
				 if(strcmp(cmd1,"send")==0||strcmp(cmd1,"SEND")==0){
				 memset(buf, 0, sizeof(buf));
				 strcpy(buf,cmd2);
				 write(csd,buf,sizeof(buf));
				 }
				 break;*/
				default:

				msg2 = malloc(sizeof(struct sbcp_message));

				if (recv(csd, msg2, 5000, 0) < 0)
					//if (read(csd, msg2, 5000) < 0)
					write(1,
							"\nAn exception occured while receiving the response",
							sizeof("An exception occured while receiving the response"));
				else {
					write(1, "\nSuccess in recv client",
							sizeof("\nSuccess in recv client"));

					memset(buf, 0, sizeof(buf));
					strcpy(buf, "\n");
					strcpy(buf, msg2->payload);
					write(1, buf, sizeof(buf));
					memset(buf, 0, sizeof(buf));
				}

				free(msg2);
				break;
			}
		}
	}
	return 0;
}
