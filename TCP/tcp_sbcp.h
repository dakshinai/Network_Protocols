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

struct sbcp_message {
//join(2) or fwd(3) or send(4) or ack(7) or nak(5), or online(8) or offline(6) or idle(9)
	int type;
	char payload[5000];
};

