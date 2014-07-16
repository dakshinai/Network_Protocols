
struct http_response {

	char payload[25000];
};

struct http_request {

	char cmd[10];
	char url[200];
	char page[200];

};

