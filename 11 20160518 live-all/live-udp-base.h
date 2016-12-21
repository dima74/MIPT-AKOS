#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>

#define MAIN_PORT 7090
#define START_PORT (MAIN_PORT + 1)
#define MAX_PACKET_SIZE 500
#define DEFAULT_TIMEOUT 1

enum PacketType
{
	STRUCT_WORKER_INFO,
	PART_OF_INITIAL_FIELD,
	ACK_FOR_INITIAL_DATA,
	PART_OF_LINE,
	UP_WORKER_DONE,
	DOWN_WORKER_DONE,
	PART_OF_RESULTING_FIELD,
	ACK_FOR_RESULTING_FIELD
};

struct sockaddr_in get_sockaddr(int port)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	return addr;
}

int sockfd;
int sockfd_id;
void create_socket(int port)
{
	check(sockfd = socket(AF_INET, SOCK_DGRAM, 0));
	
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	check(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)));
	sockfd_id = port - START_PORT;
	
	// for debug
	//int option = 1;
	//setsockopt(sockfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &option, sizeof(option));
}

void mysendto(int id, uint8_t *data, int size)
{
	//usleep(rand() % 1000 * 10);
	
	assert(size <= MAX_PACKET_SIZE);
	int port = START_PORT + id;
	struct sockaddr_in addr_to = get_sockaddr(port);
	
	char sdata[size * 9 + 1];
	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < 8; ++j)
			sdata[i * 9 + j] = '0' + ((data[i] & (1 << j)) ? 1 : 0);
		sdata[i * 9 + 8] = ' ';
	}
	sdata[size * 9] = 0;
	//sdata[0] = 0;
	
	if (netdebug) printf("%20d mysendto %d len %d: %s\n", sockfd_id, id, size, sdata);
	check(sendto(sockfd, data, size, 0, (struct sockaddr *) &addr_to, sizeof(struct sockaddr_in)));
	if (netdebug) printf("%25d mysendto %d done\n", sockfd_id, id);
}

int myrecvfrom_withtimeout(uint8_t *data, int *len, int microseconds)
{
	if (microseconds != 0)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		
		struct timeval timeout;
		timeout.tv_sec = microseconds / 1000000;
		timeout.tv_usec = microseconds;
		//printf("%d %ld select %d %d\n", sockfd_id, getmtime(), (int) timeout.tv_sec, (int) timeout.tv_usec);
		int rc = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
		//printf("%d %ld select done %d\n", sockfd_id, getmtime(), rc);
		if (rc == 0) // timeout
			return -2;
	}
	
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	if (netdebug) printf("%20d myrecvfrom\n", sockfd_id);
	check(*len = recvfrom(sockfd, data, MAX_PACKET_SIZE, 0, (struct sockaddr *) &addr, &addr_len));
	
	int size = *len;
	char sdata[size * 9 + 1];
	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < 8; ++j)
			sdata[i * 9 + j] = '0' + ((data[i] & (1 << j)) ? 1 : 0);
		sdata[i * 9 + 8] = ' ';
	}
	sdata[size * 9] = 0;
	//sdata[0] = 0;
	
	if (netdebug) printf("%25d myrecvfrom %d done len %d: %s\n", sockfd_id, ntohs(addr.sin_port) - START_PORT, *len, sdata);
	assert(addr_len == sizeof(struct sockaddr_in));
	return ntohs(addr.sin_port) - START_PORT;
}

int myrecvfrom(uint8_t *data, int *len)
{
	return myrecvfrom_withtimeout(data, len, 0);
}

void mysendto_ack(int id, enum PacketType ack_type)
{
	uint8_t data[1] = {ack_type};
	mysendto(id, data, 1);
}

void zip_field(uint8_t *data, int size, bool **field, int h, int w, int i0, int j0)
{
	uint8_t curr = 0;
	int curr_number = 0;
	for (int i = i0; i < h && size > 0; ++i)
		for (int j = i == i0 ? j0 : 0; j < w && size > 0; ++j)
		{
			--size;
			if (field[i][j])
				curr |= 1 << curr_number;
			if (++curr_number == 8 || size == 0)
			{
				*data++ = curr;
				curr = 0;
				curr_number = 0;
			}
		}
}

void unzip_field(uint8_t *data, int size, bool **field, int h, int w, int i0, int j0)
{
	int size0 = size;
	
	uint8_t curr = *data;
	int curr_number = 0;
	for (int i = i0; i < h && size > 0; ++i)
		for (int j = i == i0 ? j0 : 0; j < w && size > 0; ++j)
		{
			--size;
			field[i][j] = curr & (1 << curr_number);
			if (++curr_number == 8 || size == 0)
			{
				curr = *++data;
				curr_number = 0;
			}
		}
	
	for (int i = i0, csize = 0; i < h; ++i)
		for (int j = i == i0 ? j0 : 0; j < w; ++j)
			if (csize++ < size0 && field[i][j] && !field[i][j])
				printf("!!!!!!!!!!!!\n");
}

int min(int a, int b)
{
	return a < b ? a : b;
}

// всё поле, возможно в нескольких пакетах
void mysendto_field(int id, uint8_t *additional_data, int additional_size, enum PacketType type, bool **field, int h, int w, int addtoi)
{
	int all_data_size = (h * w + 7) / 8;
	int header_size = 1 + 3 * sizeof(int) + additional_size;
	int max_data_size = MAX_PACKET_SIZE - header_size;
	int number_packets = (all_data_size + max_data_size - 1) / max_data_size;
	for (int ipacket = 0; ipacket < number_packets; ++ipacket)
	{
		int packet_size = ipacket < number_packets - 1 ? MAX_PACKET_SIZE : (header_size + (all_data_size - ipacket * max_data_size));
		int data_size = packet_size - header_size;
		uint8_t data[packet_size];
		*data = type;
		int i0 = ipacket * max_data_size * 8 / w;
		int j0 = ipacket * max_data_size * 8 % w;
		int number_grids = min(data_size * 8, (h - i0 - 1) * w + (w - j0));
		int extra[3] = {i0 + addtoi, j0, number_grids};
		memcpy(data + 1, extra, sizeof(extra));
		memcpy(data + header_size - additional_size, additional_data, additional_size);
		zip_field(data + header_size, number_grids, field, h, w, i0, j0);
		//printf("\t%2d  %d %d %d %d  %d\n", sockfd_id, i0, j0, ipacket * max_data_size * 8, number_grids, *(data + header_size));
		mysendto(id, data, packet_size);
	}
}

typedef void (*Func_try_send_ack) (int i0, int j0, int number_grids, int is_first_time);
typedef bool (*Func_filter_by_type_and_additional_data) (enum PacketType type, uint8_t *additional_data, int debug_id_from);
typedef void (*Func_when_timeout) ();
typedef bool (*Func_check_break_recv) (enum PacketType type);
void myrecvfrom_field(enum PacketType type, bool **field, int h, int w, int additional_size, Func_try_send_ack func_try_send_ack, Func_filter_by_type_and_additional_data func_filter_by_type_and_additional_data, Func_when_timeout func_when_timeout, Func_check_break_recv func_check_break_recv)
{
	int header_size = 1 + 3 * sizeof(int) + additional_size;
	int recv_grids = 0;
	bool **start_grids = malloc_field(h, w);
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			start_grids[i][j] = 0;
	while (recv_grids < h * w)
	{
		uint8_t data[MAX_PACKET_SIZE];
		int len;
		int id = myrecvfrom_withtimeout(data, &len, func_when_timeout == NULL ? 0 : DEFAULT_TIMEOUT);
		if (debug) printf("\t%d myrecvfrom_field part from %d (%d/%d)\n", sockfd_id, id, recv_grids, h * w);
		if (id == -2)
		{
			func_when_timeout();
			continue;
		}
		if (func_check_break_recv != NULL && func_check_break_recv(*data))
			return;
		if ((func_filter_by_type_and_additional_data != NULL && func_filter_by_type_and_additional_data(*data, data + header_size - additional_size, id)) || *data != type)
			continue;
		int extra[3];
		memcpy(extra, data + 1, sizeof(extra));
		int i0 = extra[0];
		int j0 = extra[1];
		int number_grids = extra[2];
		if (!start_grids[i0][j0])
		{
			if (additional_size > 0)
				if (debug) printf("%d get %d from %d\n", sockfd_id, *(data + header_size - additional_size), id);
			recv_grids += number_grids;
			unzip_field(data + header_size, number_grids, field, h, w, i0, j0);
			//printf("%2d  %d %d %d %d %d/%d  %d\n", sockfd_id, i0, j0, start_grids[i0][j0], number_grids, recv_grids, h * w, *(data + header_size));
		}
		if (func_try_send_ack != NULL)
			func_try_send_ack(i0, j0, number_grids, !start_grids[i0][j0]);
		start_grids[i0][j0] = true;
	}
	free_field(start_grids, h, w);
}
