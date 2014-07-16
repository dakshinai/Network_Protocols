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

	//counter variables
	int max_sd;
	int i, csd;
	int result;

	struct sbcp_message *msg, *msg2;
	char *current_user;
	int deny_flag = 0;

	char *buf;
	//storage
	int csd_count = 0, username_count = 0;
	int *csds;
	int max_clients = 0;

	char **username;

	int ssd;
	struct sockaddr_in addr;
	struct hostent* hret;
	short port = 0;

	fd_set readfds;
	int activity;

	if ((ssd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("\nError: Cannot create socket");
		exit(EXIT_FAILURE);
	}

	hret = gethostbyname(argv[1]);
	port = atoi(argv[2]);
	max_clients = atoi(argv[3]);

	csds = malloc(max_clients * sizeof(int));   //memory allocated to csds array
	memset(csds, 0, max_clients * sizeof(int));  //initialize csds array

	username = (char**) malloc(sizeof(char*) * max_clients);
	for (i = 0; i < max_clients; i++)
		username[i] = malloc(100);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);

	if (bind(ssd, (struct sockaddr*) &addr, sizeof(struct sockaddr)) == -1) {
		perror("\nError: Cannot bind socket");
		exit(EXIT_FAILURE);
	}
	if (listen(ssd, 10) == -1) {
		perror("\nError: In setting socket to listen state");
		exit(EXIT_FAILURE);
	}

	buf = (char*) malloc(sizeof(char) * 5000);

	while (1) {

		FD_ZERO(&readfds);
		FD_SET(ssd, &readfds);
		max_sd = ssd;

		for (i = 0; i < csd_count; i++) {
			csd = csds[i];
			if (csd > 0)
				FD_SET(csd, &readfds);

			if (csd > max_sd)
				max_sd = csd;

		}

		activity = select(max_sd + 1, &readfds, 0, 0, 0);
		if (activity < 0 && (errno != EINTR))
			perror("\nError: No activity");

		//if there is a activity on ssd
		if (FD_ISSET(ssd,&readfds)) {

			csd = 0;
			deny_flag = 0;
			memset(current_user, 0, sizeof(current_user));

			if ((csd = accept(ssd, 0, 0)) == -1) {
				perror("\nError: Accepting connections");

			} else {
				write(1, "\nA connection received",
						sizeof("\nA connection received"));

				//msg = malloc(sizeof(struct sbcp_message));

				memset(buf, 0, sizeof(buf));

				//if (recv(csd, msg, 5000, 0) < 0)
				if (recv(csd, buf, 5000, 0) < 0)

					write(1,
							"\nAn exception occured while receiving the request",
							sizeof("\nAn exception occured while receiving the request"));
				else {
					write(1, "\nSuccess in recv server",
							sizeof("\nSuccess in recv server"));

					/*for (i = 0; i < username_count; i++) {
					 memset(buf, 0, sizeof(buf));
					 strcpy(buf, "\n");
					 strcat(buf, username[i]);
					 write(1, buf, sizeof(buf));
					 memset(buf, 0, sizeof(buf));
					 }*/
					//msg->type == 2 &&
					if (username_count < max_clients) {

						memset(current_user, 0, sizeof(current_user));

						//copy param 0-user name
						/*strncpy(current_user, msg->payload,
						 strlen(msg->payload));

						 memset(buf, 0, sizeof(buf));
						 strcpy(buf, current_user);
						 write(1, buf, sizeof(buf));
						 memset(buf, 0, sizeof(buf));*/

						strcpy(current_user, buf);

						for (i = 0; i < username_count; i++) { //verify whether the name have been used
							write(1, current_user, sizeof(current_user));
							write(1, username[i], sizeof(username[i]));
							if (strcmp(current_user, "") != 0
									&& strcmp(username[i], current_user) == 0) {

								deny_flag = 1; // show that the name have been used
								break;
							}
						}
					} else
						deny_flag = 1;

					//free(msg);

					memset(buf, 0, sizeof(buf));
					sprintf(buf, "\n%d", deny_flag);
					write(1, buf, sizeof(buf));
					memset(buf, 0, sizeof(buf));

					if (deny_flag == 0) {

						csds[csd_count] = csd;
						strncpy(username[username_count], current_user,
								strlen(current_user));

						csd_count++;
						username_count++;

						msg2 = malloc(sizeof(struct sbcp_message));

						msg2->type = 7;    //ack

						memset(msg2->payload, 0, sizeof(msg2->payload));
						strncpy(msg2->payload,
								"\nClient successfully connected\0",
								strlen("\nClient successfully connected\0")
										+ 1);

						if (send(csd, msg2, 50000, 0) < 0)
							//if (write(csd, msg2, 50000) < 0)
							write(1,
									"\nAn exception occured while passing the response-2",
									sizeof("\nAn exception occured while passing the response-2"));
						else
							write(1, "\nSuccess in send server2",
									sizeof("\nSuccess in send server2"));

						free(msg2);

						//send online message to all active clients

					} else {

						//free(msg);
						msg2 = malloc(sizeof(struct sbcp_message));

						msg2->type = 5;    //nak
						memset(msg2->payload, 0, sizeof(msg2->payload));
						strncpy(msg2->payload, "\nUsername is already in use\n",
								strlen("\nUsername is already in use\n") + 1);

						if (send(csd, msg2, sizeof(msg2), 0) < 0)
							write(1,
									"\nAn exception occured while passing the response",
									sizeof("\nAn exception occured while passing the response"));
						else
							write(1, "\nSuccess in send server",
									sizeof("\nSuccess in send server"));

						free(msg2);

						close(csd);
					}
				}

			}
		}

		//for client side change

		for (i = 0; i < csd_count; i++) {
			csd = csds[i];
			if (FD_ISSET(csd,&readfds)) {

				//read message

				msg2 = malloc(sizeof(struct sbcp_message));

				result = recv(csd, msg2, 50000, 0);
				if (result < 0)
					//if (read(csd, msg2, 5000) < 0)
					write(1,
							"\nAn exception occured while sending the connecection to client",
							sizeof("\nAn exception occured while sending the connecection to client"));
				else if (result == 0) {
					//A change in the client descriptor but no change in data
					//This can imply a disconnection
					memset(buf, 0, sizeof(buf));
					sprintf(buf, "Client %d disconnected", i + 1);
					write(1, buf, sizeof(buf));
					memset(buf, 0, sizeof(buf));

					//remove from username and csd array
					//set count -- for username and csd array
					//send offline message to all clients active

				} else {
					write(1, "\nSuccess in contacting client",
							sizeof("\nSuccess in contacting client"));

					//send message to all clients except connected client

					for (i = 0; i < csd_count; i++) {
						csd = csds[i];

						msg = malloc(sizeof(struct sbcp_message));

						msg->type = 3;    //fwd message
						memset(msg->payload, 0, sizeof(msg->payload));
						strcpy(msg->payload, msg2->payload);

						if (send(csd, msg, sizeof(msg), 0) < 0)
							write(1,
									"\nAn exception occured while passing the response",
									sizeof("\nAn exception occured while passing the response"));
						else
							write(1, "\nSuccess in send server",
									sizeof("\nSuccess in send server"));

					}

					free(msg);

				}

				free(msg2);

			}

		}    //end of for

	}    //end while

	return 0;
}
