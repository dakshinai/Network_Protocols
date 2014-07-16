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
#include <iostream>

int main(int argc, char** argv) {
	using namespace
	std;

	int csd;

	char *temp;
	int clip = 0;

	struct sockaddr_in addr;
	struct hostent* hret;
	short port = 0;

	struct http_response resp;
	struct http_request req;

	char cmd[10], url[200];

	if ((csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("\nError: Cannot create client socket");
		exit(EXIT_FAILURE);
	}

	hret = gethostbyname(argv[1]);
	port = atoi(argv[2]);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	memcpy(&addr.sin_addr.s_addr, hret->h_addr, hret->h_length);

	while (1) {

		scanf("%s %s", cmd, url);
		strcpy(req.cmd, cmd);

		temp = strtok(url, "/");

		clip = 0;
		while (temp != NULL) {

			if (clip == 1) {
				strcpy(req.url, temp);
			} else if (clip == 2) {
				strcpy(req.page, temp);
			} else if (clip > 2)
				break;

			temp = strtok(NULL, "/");
			clip++;
		}

		if (clip > 2) {

			//std::cout<<req.cmd<<endl<<strlen(req.cmd)<<endl<<req.url<<endl<<strlen(req.url)<<endl<<req.page<<endl<<strlen(req.page)<<endl;

			switch (req.cmd[0]) {
			case 'g':
			case 'G':
				if (strcmp(req.cmd, "get") == 0
						|| strcmp(req.cmd, "GET") == 0) {
					if (connect(csd, (struct sockaddr*) &addr,
							sizeof(struct sockaddr)) == -1) {
						perror("\nError: Cannot connect to Server");
					} else {

						if (send(csd, &req, sizeof(req), 0) < 0)
							write(1,
									"\nAn exception occured while passing the request",
									sizeof("\nAn exception occured while passing the request"));
						else {
							write(1, "\nSuccess in send client",
									sizeof("\nSuccess in send client"));

							//memset(resp.payload,0,sizeof(resp.payload));

							if (recv(csd, &resp, sizeof(struct http_response),
									0) < 0)

								write(1,
										"\nAn exception occured while receiving the response",
										sizeof("An exception occured while receiving the response"));
							else {
								write(1, "\nSuccess in recv client",
										sizeof("\nSuccess in recv client"));

								std::cout<<endl;
								write(1, resp.payload,
										strlen(resp.payload) + 1);

							}

							//free(recv_buf);
						}
						//free(send_buf);

					} //end of connection
				} //end of get

				break;

			default:

				break;

			} //end of switch
		} //end of check input
	} //end of while
	return 0;
}
