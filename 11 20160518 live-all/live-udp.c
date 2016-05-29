#include "abstract.h"
//#include "main-default.h"
#include "test.h"
#include "live-udp-base.h"

#include <sys/types.h>
#include <sys/wait.h>

bool check_for_initial_ack(enum PacketType type)
{
	bool is_type_initial = type == STRUCT_WORKER_INFO || type == PART_OF_INITIAL_FIELD;
	if (is_type_initial)
		mysendto_ack(-1, ACK_FOR_INITIAL_DATA);
	return is_type_initial;
}

void sendline(int step, int id, bool *line, int w, int addtoi)
{
	mysendto_field(id, (uint8_t *) &step, sizeof(int), PART_OF_LINE, &line, 1, w, addtoi);
}

void sendlines(int step, struct worker_info *info, bool *toup, bool *todown)
{
	if (step < -1)
		return;
	if (debug) printf("%d sendlines %d\n", info->id, step);
	int idup = (info->number_threads + info->id - 1) % info->number_threads;
	int iddown = (info->id + 1) % info->number_threads;
	sendline(step, idup, toup, info->w, 1);
	sendline(step, iddown, todown, info->w, 0);
	if (debug) printf("%d sendlines done %d\n", info->id, step);
}

//==========================================
int step_;
bool func_filter_by_type_and_additional_data(enum PacketType type, uint8_t *additional_data, int debug_id_from)
{
	if (check_for_initial_ack(type))
		return true;
	int step = *additional_data;
	if (step != step_)
		printf("!!! %d want %d get %d from %d\n", sockfd_id, step_, step, debug_id_from);
	return step != step_;
}

bool *prevtoup_;
bool *prevtodown_;
bool *toup_;
bool *todown_;
struct worker_info *info_;
void func_when_timeout()
{
	printf("%d want %d timeout\n", sockfd_id, step_);
	sendlines(step_, info_, toup_, todown_);
	sendlines(step_ - 1, info_, prevtoup_, prevtodown_);
}
//==========================================

void send_and_recv(bool *prevtoup, bool *prevtodown, bool *toup, bool *todown, bool *fromup, bool *fromdown, struct worker_info *info, int step)
{
	sendlines(step, info, toup, todown);
	bool *field[2] = {fromup, fromdown};
	if (debug) printf("%d %d myrecvfrom_field\n", info->id, step);
	step_ = step;
	prevtoup_ = prevtoup;
	prevtodown_ = prevtodown;
	toup_ = toup;
	todown_ = todown;
	info_ = info;
	myrecvfrom_field(PART_OF_LINE, field, 2, info->w, sizeof(int), NULL, func_filter_by_type_and_additional_data, func_when_timeout);
	if (debug) printf("%d %d myrecvfrom_field done\n", info->id, step);
	
	//print_field_named(info->id, "sendlines", field, 2, info->w);
}

void recvinfo(struct worker_info *info)
{
	while (1)
	{
		uint8_t data[MAX_PACKET_SIZE];
		int len;
		myrecvfrom(data, &len);
		if (data[0] != STRUCT_WORKER_INFO)
			continue;
		assert(len == 1 + sizeof(struct worker_info));
		memcpy(info, data + 1, sizeof(struct worker_info));
		break;
	}
}

void run_worker(int port)
{
	create_socket(port);
	int id = port - START_PORT;
	
	struct worker_info info;
	if (debug) printf("%d recvinfo\n", id);
	recvinfo(&info);
	if (debug) printf("%d recvinfo done\n", id);
	int parth = info.i1 - info.i0;
	int partw = info.w;
	info.field = malloc_field(parth + 2, partw);
	if (debug) printf("%d recvfield\n", id);
	myrecvfrom_field(PART_OF_INITIAL_FIELD, info.field, parth + 2, partw, 0, NULL, NULL, NULL);
	if (debug) printf("%d recvfield done\n", id);
	
	//print_field(info.field, parth + 2, partw);
	//print_field_named(info.id, "field0", info.field, parth + 2, partw);
	
	usleep(info.id * 1e5);
	mysendto_ack(-1, ACK_FOR_INITIAL_DATA);
	
	worker(&info);
	
	while (1)
	{
		if (debug) printf("%d sendfield\n", id);
		mysendto_field(-1, NULL, 0, PART_OF_RESULTING_FIELD, info.field + 1, parth, partw, info.i0);
		if (debug) printf("%d sendfield done, waiting for ack...\n", id);
		
		uint8_t data[MAX_PACKET_SIZE];
		int len;
		int id = myrecvfrom_withtimeout(data, &len, 500);
		if (check_for_initial_ack(*data) || (id >= 0 && *data == PART_OF_LINE))
			continue;
		if (id == -1)
			break;
		assert(id == -2);
	}
	
	free_field(info.field, parth + 2, partw);
	close(sockfd);
	printf("%d EXIT\n", id);
}

void send_initial_data(struct worker_info *infos, int number_threads, bool recv_ack[])
{
	for (int id = 0; id < number_threads; ++id)
		if (!recv_ack[id])
		{
			int size = 1 + sizeof(struct worker_info);
			uint8_t data[size];
			*data = STRUCT_WORKER_INFO;
			memcpy(data + 1, &infos[id], sizeof(struct worker_info));
			if (debug) printf("  send info to %d\n", id);
			mysendto(id, data, size);
			if (debug) printf("  send field to %d\n", id);
			mysendto_field(id, NULL, 0, PART_OF_INITIAL_FIELD, infos[id].field, infos[id].i1 - infos[id].i0 + 2, infos[id].w, 0);
			if (debug) printf("  send to %d done\n", id);
		}
}

//======================================================
// "Лямбда"

struct worker_info *infos_;
int number_threads_;
int left_recv_grids[100];
void init_func_try_send_ack(struct worker_info *infos, int number_threads)
{
	infos_ = infos;
	number_threads_ = number_threads;
	for (int id = 0; id < number_threads_; ++id)
	{
		int parth = infos_[id].i1 - infos_[id].i0;
		int partw = infos_[id].w;
		left_recv_grids[id] = parth * partw;
	}
}

void func_try_send_ack(int i0, int j0, int number_grids, int is_first_time)
{
	for (int id = 0; id < number_threads_; ++id)
		if (infos_[id].i0 <= i0 && i0 < infos_[id].i1)
		{
			if (is_first_time)
				left_recv_grids[id] -= number_grids;
			//printf("  func_try_send_ack %d %d %d %d %d\n", i0, j0, number_grids, is_first_time, left_recv_grids[id]);
			if (left_recv_grids[id] == 0)
			{
				//printf("  mysendto_ack %d\n", id);
				mysendto_ack(id, ACK_FOR_RESULTING_FIELD);
			}
			break;
		}
}

//======================================================

bool **run_with_generator(int h, int w, int number_steps, int number_threads, func_generator generator)
{
	if (debug)
	{
		printf("======================\n");
		printf("%d %d %d %d\n", h, w, number_steps, number_threads);
	}
	assert(h >= number_threads);
	
	pid_t pids[number_threads];
	for (int id = 0; id < number_threads; ++id)
	{
		check(pids[id] = fork());
		if (pids[id] == 0)
		{
			run_worker(START_PORT + id);
			exit(0);
		}
		if (debug) printf("%d -> %d\n", getpid(), pids[id]);
	}
	
	bool **field = malloc_field(h, w);
	generator(field, h, w);
	struct worker_info *infos = create_infos(field, h, w, number_steps, number_threads);
	//print_field(field, h, w);
	
	usleep(10000); // нужно, чтобы форкнутый процесс успел вызвать socket(), иначе первые несколько пакетов будут потеряны
	create_socket(MAIN_PORT);
	
	if (debug) printf("  sendinitial\n");
	int number_recv_acks = 0;
	bool recv_ack[number_threads];
	memset(recv_ack, 0, number_threads);
	send_initial_data(infos, number_threads, recv_ack);
	
	while (number_recv_acks < number_threads)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000 * 1000;
		int number;
		check(number = select(sockfd + 1, &fds, NULL, NULL, &timeout));
		
		if (number == 0)
			send_initial_data(infos, number_threads, recv_ack);
		else
		{
			uint8_t data[MAX_PACKET_SIZE];
			int len;
			int id = myrecvfrom(data, &len);
			if (data[0] != ACK_FOR_INITIAL_DATA)
				continue;
			assert(len == 1);
			if (!recv_ack[id])
				++number_recv_acks;
			recv_ack[id] = true;
		}
	}
	if (debug) printf("  sendinitial done\n");
	
	if (debug) printf("  recvresult\n");
	init_func_try_send_ack(infos, number_threads);
	myrecvfrom_field(PART_OF_RESULTING_FIELD, field, h, w, 0, func_try_send_ack, NULL, NULL);
	if (debug) printf("  recvresult done\n");
	
	if (debug) printf("  waitpid\n");
	for (int id = 0; id < number_threads; ++id)
		waitpid(pids[id], NULL, 0);
	if (debug) printf("  waitpid done\n");
	
	bool **field0 = simple_live_with_generator(h, w, number_steps, generator);
	//print_field(field0, h, w);
	//print_field(field, h, w);
	assert(is_field_equals(field, field0, h, w));
	free_field(field0, h, w);
	
	if (debug)
	{
		printf("Done!\n");
		printf("\n");
	}
	for (int id = 0; id < number_threads; ++id)
		free(infos[id].field);
	free(infos);
	close(sockfd);
	return field;
}

int main()
{
	//for (int seed = 0; seed < 10; ++seed)
	run_on_seed(10, 10, 5, 3, 0);
	printf("  EXIT\n");
}
