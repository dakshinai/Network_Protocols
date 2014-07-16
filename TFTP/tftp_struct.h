
struct tftp_rrq
{
	uint16_t opcode;
	char *filename;
	char *mode;
};

struct tftp_data
{
	uint16_t opcode;
	uint16_t blocknum;
	char *payload;
};

struct tftp_ack
{
	uint16_t opcode;
	uint16_t blocknum;
};

struct tftp_error
{
	uint16_t opcode;
	uint16_t errorcode;
	char* payload;
};	
