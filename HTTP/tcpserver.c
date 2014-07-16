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
#include "http_cache.c"

int main(int argc, char** argv) {

	using namespace
	std;
	struct queue *queue = CreateNewQueue(4);
	struct hash *hash = CreateHash(150);

	int max_sd;
	int i, csd;
	int csd_count = 0;
	int *csds;
	int max_clients = 0;
	fd_set readfds;
	int activity;
	int ssd;
	char *fullurl;

	struct http_response resp;
	struct http_request req;

	struct sockaddr_in addr;
	struct hostent* hret;
	short port = 0;

	struct sockaddr_in w_addr;
	struct hostent* w_hret;
	short w_port = 0;
	int w_csd;

	struct sockaddr_in wh_addr;
	struct hostent* wh_hret;
	short wh_port = 0;
	int wh_csd;

	char *get_c, *get_c2, *get_c3;

	int charindex;
	char get_request[500];
	char get_conditional_request[500];
	char head_buf[10000];
	char expirydatestr[30], modifydatestr[30];
	char etag[50];

	time_t emptytime;

	time_t et; //, lmt;
	struct tm timestruct;

	if ((ssd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("\nError: Cannot create socket");
		exit(EXIT_FAILURE);
	}

	hret = gethostbyname(argv[1]);
	port = atoi(argv[2]);
	max_clients = atoi(argv[3]);

	csds = (int*) malloc(max_clients * sizeof(int)); //memory allocated to csds array

	memset(csds, 0, max_clients * sizeof(int));  //initialize csds array

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

			csd = -1;

			if ((csd = accept(ssd, 0, 0)) == -1) {
				perror("\nError: Accepting connections");

			} else {
				write(1, "\nA connection received",
						sizeof("\nA connection received"));

				csds[csd_count] = csd;
				csd_count++;

				//memset(req.cmd, 0, sizeof(req.cmd));
				//memset(req.url, 0, sizeof(req.url));
				//memset(req.page, 0, sizeof(req.page));

				if (recv(csd, &req, sizeof(req), 0) < 0)

					write(1,
							"\nAn exception occured while receiving the request",
							sizeof("\nAn exception occured while receiving the request"));
				else {
					write(1, "\nSuccess in recv server",
							sizeof("\nSuccess in recv server"));

					fullurl = (char*) malloc(
							sizeof("http://") + sizeof(req.url) + sizeof("/")
									+ sizeof(req.page));

					strcat(fullurl, "http://");
					strcat(fullurl, req.url);
					strcat(fullurl, "/");
					strcat(fullurl, req.page);

					if (!IsPageInCache(queue, hash, fullurl)) {

						if ((w_csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
							perror("\nError: Cannot create proxy socket");
							continue;
						}

						w_hret = gethostbyname(req.url);
						w_port = 80;
						w_addr.sin_family = AF_INET;
						w_addr.sin_port = htons(w_port);

						memcpy(&w_addr.sin_addr.s_addr, w_hret->h_addr, w_hret->h_length);

						if (connect(w_csd, (struct sockaddr*) &w_addr,
								sizeof(struct sockaddr)) == -1) {
							perror("\nError: Cannot connect to Web server");
							continue;
						}

						memset(get_request, 0, sizeof(get_request));

						//GET /index.html HTTP/1.0 \n \r HOST: 165.91.215.188
						strcat(get_request, "GET /");
						strcat(get_request, req.page);
						strcat(get_request, " HTTP/1.0\n\nHOST:");
						strcat(get_request, req.url);
						//strcat(get_request,"\r \n \r \n");

						std
						::cout<<endl<<get_request<<endl;

						//send req to web server
						if (send(w_csd, get_request, sizeof(get_request), 0)
								< 0)
							write(1,
									"\nAn exception occured while passing the request to web server",
									sizeof("\nAn exception occured while passing the request to web server"));
						else {
							write(1, "\nSuccess in send proxy",
									sizeof("\nSuccess in send proxy"));

							memset(resp.payload, 0, sizeof(resp.payload));
							//get response
							if (recv(w_csd, resp.payload, sizeof(resp.payload),
									0) < 0)

								write(1,
										"\nAn exception occured while receiving the response from web server",
										sizeof("An exception occured while receiving the response from web server"));
							else {
								write(1, "\nSuccess in recv proxy",
										sizeof("\nSuccess in recv proxy"));

								//write(1, resp.payload, strlen(resp.payload) + 1);

								get_c = strstr(resp.payload, "200 OK");

								//std::cout<<endl<<get_c<<endl;

								if (get_c) {
									//if 200ok update content, expiry and return

									std
									::cout<<endl<<"Page fetched from web server"<<endl;

									get_c = strstr(resp.payload, "Expires: ");
									get_c = get_c + 9;

									get_c2 = strstr(resp.payload, "ETag: ");
									get_c2 = get_c2 + 6;

									get_c3 = strstr(resp.payload,
											"Last-Modified: ");
									get_c3 = get_c3 + 15;

									charindex = 0;
									while (get_c && *get_c != '\n') {
										expirydatestr[charindex] = *get_c;
										charindex++;
										get_c++;
									}

									if (expirydatestr[charindex - 1] != '\0') {
										charindex++;
										expirydatestr[charindex - 1] = '\0';
									}
									//std::cout<<endl<<expirydatestr<<endl;

									strptime(expirydatestr,
											"%a, %d %b %Y %H:%M:%S %z",
											&timestruct);
									et = mktime(&timestruct);

									charindex = 0;
									while (get_c2 && *get_c2 != '\n') {
										etag[charindex] = *get_c2;
										charindex++;
										get_c2++;
									}

									if (etag[charindex - 1] != '\0') {
										charindex++;
										etag[charindex - 1] = '\0';
									}
									//std::cout<<endl<<expirydatestr<<endl;

									charindex = 0;
									while (get_c3 && *get_c3 != '\n') {
										modifydatestr[charindex] = *get_c3;
										charindex++;
										get_c3++;
									}

									if (modifydatestr[charindex - 1] != '\0') {
										charindex++;
										modifydatestr[charindex - 1] = '\0';
									}

									/*strptime(modifydatestr,
									 "%a, %d %b %Y %H:%M:%S %z",
									 &timestruct);
									 lmt = mktime(&timestruct);*/

									ReferencePage(queue, hash, fullurl,
											resp.payload, 0, et, etag,
											modifydatestr);

									std
									::cout<<"\nCache entries:\n";
									std
									::cout<<"URL:"<<hash->urls[queue->front->pageno-1]<<endl;
									std
									::cout<<"Contents:"<<queue->front->contents<<endl;
									std
									::cout<<"Timestamp:"<<ctime(&queue->front->timestamp)<<endl;
									std
									::cout<<"Expiry Timestamp:"<<ctime(&queue->front->expirystamp)<<endl;
									//std::cout<<"Modified Timestamp:"<<ctime(&queue->front->modifiedstamp)<<endl;
									std
									::cout<<"ETag:"<<queue->front->etag<<endl;
									std
								::cout<<"Modify Date String:"<<queue->front->modifystampstr<<endl;

							}
							/*else
							 ReferencePage(queue, hash, fullurl,
							 resp.payload, 0, emptytime);*/

							//and send to client
							if (send(csd, &resp, sizeof(struct http_response),
									0) < 0) {

								write(1,
										"\nAn exception occured while passing the response to client",
										sizeof("\nAn exception occured while passing the response to client"));
							} else {

								write(1, "\nSuccess in send client",
										sizeof("\nSuccess in send client"));

							}

							//refer page in cache along with new content

							std
						::cout<<endl<<"Data was fetched from web server"<<endl;

					}
					//success in get response from web server

				}					//success in send to web server

			}					//page not in cache

			else {

				if ((wh_csd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					perror("\nError: Cannot create proxy socket");
					continue;					//to be changed
				}

				wh_hret = gethostbyname(req.url);
				wh_port = 80;
				wh_addr.sin_family = AF_INET;
				wh_addr.sin_port = htons(wh_port);

				memcpy(&wh_addr.sin_addr.s_addr, wh_hret->h_addr, wh_hret->h_length);

				if (connect(wh_csd, (struct sockaddr*) &wh_addr,
						sizeof(struct sockaddr)) == -1) {
					perror("\nError: Cannot connect to Web server");
					continue;					//to be changed
				}

				memset(get_conditional_request, 0,
						sizeof(get_conditional_request));

				ReferencePage(queue, hash, fullurl, resp.payload, 0, emptytime,
						etag, modifydatestr);

				//GET /index.html HTTP/1.0 \n \r HOST: 165.91.215.188
				strcat(get_conditional_request, "GET /");
				strcat(get_conditional_request, req.page);
				strcat(get_conditional_request, " HTTP/1.0\r\nHOST:");
				strcat(get_conditional_request, req.url);
				strcat(get_conditional_request, "\nIf-Modified-Since: ");
				//strcat(get_conditional_request, ctime(&queue->front->timestamp));
				strcat(get_conditional_request, queue->front->modifystampstr);
				//modifiedstampstr
				strcat(get_conditional_request, "If-None-Match: ");
				strcat(get_conditional_request, queue->front->etag);
				strcat(get_conditional_request, "\r\n\r\n");

				//strcat(get_request,"\r \n \r \n");

				std
				::cout<<endl<<get_conditional_request<<endl;

				//send req to web server
				//do a conditional get request
				if (send(wh_csd, get_conditional_request,
						sizeof(get_conditional_request), 0) < 0)
					write(1,
							"\nAn exception occured while passing the conditional get request to web server",
							sizeof("\nAn exception occured while passing the conditional get request to web server"));
				else {
					write(1, "\nSuccess in conditional get send proxy",
							sizeof("\nSuccess in conditional get send proxy"));

					memset(head_buf, 0, sizeof(head_buf));

					if (recv(wh_csd, head_buf, sizeof(head_buf), 0) < 0)

						write(1,
								"\nAn exception occured while receiving the conditional get response from web server",
								sizeof("An exception occured while receiving the conditional get response from web server"));
					else {
						write(1, "\nSuccess in conditional get recv proxy",
								sizeof("\nSuccess in conditional get recv proxy"));

						std
						::cout<<endl<<head_buf<<endl;
						memset(resp.payload, 0, sizeof(resp.payload));

						get_c = strstr(head_buf, "200 OK");

						//std::cout<<endl<<get_c<<endl;

						std
						::cout<<endl<<"Page to be fetched from cache"<<endl;

						if (get_c) {
							//if 200ok update content, expiry and return

							std
							::cout<<endl<<"Page modified in web server, updating cache.."<<endl;

							get_c = strstr(head_buf, "Expires: ");
							get_c = get_c + 9;

							get_c2 = strstr(head_buf, "ETag: ");
							get_c2 = get_c2 + 6;

							get_c3 = strstr(head_buf, "Last-Modified: ");
							get_c3 = get_c3 + 15;
							charindex = 0;
							while (get_c && *get_c != '\n') {
								expirydatestr[charindex] = *get_c;
								charindex++;
								get_c++;
							}

							if (expirydatestr[charindex - 1] != '\0') {
								charindex++;
								expirydatestr[charindex - 1] = '\0';
							}
							//std::cout<<endl<<expirydatestr<<endl;

							strptime(expirydatestr, "%a, %d %b %Y %H:%M:%S %z",
									&timestruct);
							et = mktime(&timestruct);

							charindex = 0;
							while (get_c2 && *get_c2 != '\n') {
								etag[charindex] = *get_c2;
								charindex++;
								get_c2++;
							}

							if (etag[charindex - 1] != '\0') {
								charindex++;
								etag[charindex - 1] = '\0';
							}

							charindex = 0;
							while (get_c3 && *get_c3 != '\n') {
								modifydatestr[charindex] = *get_c3;
								charindex++;
								get_c3++;
							}

							if (modifydatestr[charindex - 1] != '\0') {
								charindex++;
								modifydatestr[charindex - 1] = '\0';
							}

							//std::cout<<endl<<expirydatestr<<endl;

							/*strptime(modifydatestr, "%a, %d %b %Y %H:%M:%S %z",
							 &timestruct);
							 lmt = mktime(&timestruct);*/

							strcpy(resp.payload, head_buf);

							ReferencePage(queue, hash, fullurl, resp.payload, 1,
									et, etag, modifydatestr);

							std
							::cout<<"\nCache entries:\n";
							std
							::cout<<"URL:"<<hash->urls[queue->front->pageno-1]<<endl;
							std
							::cout<<"Contents:"<<queue->front->contents<<endl;
							std
							::cout<<"Timestamp:"<<ctime(&queue->front->timestamp)<<endl;
							std
							::cout<<"Expiry Timestamp:"<<ctime(&queue->front->expirystamp)<<endl;
							//std::cout<<"Modified Timestamp:"<<ctime(&queue->front->modifiedstamp)<<endl;
							std
							::cout<<"ETag:"<<queue->front->etag<<endl;
							std
						::cout<<"Modify Date String:"<<queue->front->modifystampstr<<endl;

					}			//successful conditional get- page modified

					else {
						//if not 200ok
						//compare expiry date,
						//if expiry modified update expiry date and return
						//if not expiry modified return

						//here default update of expiry date is the same

						/*get_c = strstr(head_buf, "Expires: ");
						 get_c = get_c + 9;

						 charindex = 0;
						 while (get_c && *get_c != '\n') {
						 expirydatestr[charindex] = *get_c;
						 charindex++;
						 get_c++;
						 }

						 if (expirydatestr[charindex - 1] != '\0') {
						 charindex++;
						 expirydatestr[charindex - 1] = '\0';
						 }
						 //std::cout<<endl<<expirydatestr<<endl;

						 strptime(expirydatestr, "%a, %d %b %Y %H:%M:%S %z",
						 &timestruct);
						 et = mktime(&timestruct);

						 strcpy(resp.payload, queue->front->contents);

						 ReferencePage(queue, hash, fullurl, resp.payload, 1,
						 et);*/

						std
						::cout<<endl<<"Page not modified in web server, fetching from cache.."<<endl;

						strcpy(resp.payload, queue->front->contents);

						std
						::cout<<"\nCache entries:\n";
						std
						::cout<<"URL:"<<hash->urls[queue->front->pageno-1]<<endl;
						std
						::cout<<"Contents:"<<queue->front->contents<<endl;
						std
						::cout<<"Timestamp:"<<ctime(&queue->front->timestamp)<<endl;
						std
						::cout<<"Expiry Timestamp:"<<ctime(&queue->front->expirystamp)<<endl;

						//std::cout<<"Modified Timestamp:"<<ctime(&queue->front->modifiedstamp)<<endl;
						std
					::cout<<"ETag:"<<queue->front->etag<<endl;
				}

				if (send(csd, &resp, sizeof(struct http_response), 0) < 0) {

					write(1,
							"\nAn exception occured while passing the response to client",
							sizeof("\nAn exception occured while passing the response to client"));
				} else {

					write(1, "\nSuccess in send client",
							sizeof("\nSuccess in send client"));

				}

				std
			::cout<<endl<<"Data was fetched from cache"<<endl;
		}
		//success in conditional get
	}
	//end of conditional get
}
//end in cache

}
//success in recv request from client
} //success in connection
} //end of server check

//check client changes here

} //end of while

return 0;
}
