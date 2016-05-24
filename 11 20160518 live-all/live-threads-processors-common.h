#ifndef LIVE_THREADS_PROCESSORS_COMMON
#define LIVE_THREADS_PROCESSORS_COMMON

#include "base.h"
#include "abstract.h"

#include <semaphore.h>
#include <assert.h>

struct thread_info
{
	struct worker_info base;
	int curr_step;
	sem_t sem_something_happen;
	sem_t mutex;
	bool *fromup;
	bool *fromdown;
	bool fromupfill;
	bool fromdownfill;
};

void *my_malloc(size_t size);
void my_free(void *data, size_t size);
void run_workers(struct thread_info *infos, int number_threads);

bool try_send(struct thread_info *info, struct thread_info *info_another, bool *linefrom, bool toup)
{
	if (debug) printf("send %d (%d) %s to %d (%d) \n", info->base.id, info->curr_step, toup ? "up" : "down", info_another->base.id, info_another->curr_step);
	sem_wait(&info_another->mutex);
	if (debug) printf("\twait mutex done %d (%d) %s to %d (%d) \n", info->base.id, info->curr_step, toup ? "up" : "down", info_another->base.id, info_another->curr_step);
	bool *lineto = toup ? info_another->fromdown : info_another->fromup;
	assert(info_another->curr_step <= info->curr_step);
	bool success = info_another->curr_step == info->curr_step && lineto != NULL;
	if (success)
	{
		if (debug) printf("\t%d to %d into %p: %d\n", info->base.id, info_another->base.id, lineto, linefrom[0]);
		for (int j = 0; j < info->base.w; ++j)
			lineto[j] = linefrom[j];
		toup ? (info_another->fromdownfill = true) : (info_another->fromupfill = true);
	}
	if (debug) printf("\tsend %d (%d) %s to %d (%d)  \t%s\n", info->base.id, info->curr_step, toup ? "up" : "down", info_another->base.id, info_another->curr_step, success ? "ok" : "fail");
	sem_post(&info_another->mutex);
	sem_post(&info_another->sem_something_happen);
	return success;
}

void send_and_recv(bool *toup, bool *todown, bool *fromup, bool *fromdown, struct worker_info *base_info)
{
	//usleep(base_info->id * 1e5);
	int id = base_info->id;
	int number_threads = base_info->number_threads;
	struct thread_info *info = (struct thread_info *) base_info;
	
	int idup = (number_threads + id - 1) % number_threads;
	int iddown = (id + 1) % number_threads;
	struct thread_info *infoup = info + (idup - id);
	struct thread_info *infodown = info + (iddown - id);
	
	if (debug) printf("%d wait mutex\n", id);
	check(sem_wait(&info->mutex));
	info->fromup = fromup;
	info->fromdown = fromdown;
	info->fromupfill = false;
	info->fromdownfill = false;
	if (debug) printf("\t%d post mutex\n", id);
	check(sem_post(&info->mutex));
	
	check(sem_post(&infoup->sem_something_happen));
	check(sem_post(&infodown->sem_something_happen));
	
	if (debug) printf("%d wait happen0 %p\n", id, &info->sem_something_happen);
	check(sem_wait(&info->sem_something_happen));
	if (debug) printf("\t%d wait happen0 done\n", id);
	
	bool sendup = 0;
	bool senddown = 0;
	while (!sendup || !senddown)
	{
		sendup = sendup || try_send(info, infoup, toup, true);
		senddown = senddown || try_send(info, infodown, todown, false);
		if (!sendup || !senddown)
		{
			if (debug) printf("%d wait happen1 %d %d\n", id, sendup, senddown);
			check(sem_wait(&info->sem_something_happen));
			if (debug) printf("\t%d wait happen1 done %d %d\n", id, sendup, senddown);
		}
	}
	
	while (1)
	{
		check(sem_wait(&info->mutex));
		bool fill_done = info->fromdownfill && info->fromupfill;
		check(sem_post(&info->mutex));
		if (fill_done)
			break;
		
		if (debug) printf("%d wait happen2\n", id);
		check(sem_wait(&info->sem_something_happen));
		if (debug) printf("\t%d wait happen2 done\n", id);
	}
	
	check(sem_wait(&info->mutex));
	++info->curr_step;
	info->fromup = info->fromdown = NULL;
	check(sem_post(&info->mutex));
	if (debug) printf("\t\t%d done\n", id);
	if (debug) if (id == 0) printf("\n\n\n");
}

typedef void (*func_generator) (bool **, int, int);
bool **run_with_generator(int h, int w, int number_steps, int number_threads, func_generator generator)
{
	if (debug)
	{
		printf("======================\n");
		printf("%d %d %d %d\n", h, w, number_steps, number_threads);
	}
	assert(h >= number_threads);
	
	int size_field = h * sizeof(bool *);										// field
	int size_field_data = h * w * sizeof(bool);									// field[i]
	int size_infos = number_threads * sizeof(struct thread_info);				// infos
	int size_infos_fields = (h + number_threads * 2) * sizeof(bool *);		// infos[id].field
	//int size_infos_lines_data = number_threads * 2 * (w + 2) * sizeof(bool *);	// infos[id].fromup + infos[id].fromdown
	size_field_data = (size_field_data + 7) / 8 * 8;
	
	int size = size_field + size_field_data + size_infos + size_infos_fields;
	void *start_field = my_malloc(size);
	void *start_field_data = start_field + size_field;
	void *start_infos = start_field_data + size_field_data;
	void *start_infos_fields = start_infos + size_infos;
	
	bool **field = start_field;
	for (int i = 0; i < h; ++i)
		field[i] = start_field_data + i * w * sizeof(bool);
	generator(field, h, w);
	
	struct thread_info *infos = start_infos;
	struct worker_info *base_infos = create_infos(field, h, w, number_steps, number_threads);
	for (int id = 0; id < number_threads; ++id)
	{
		infos[id].base = base_infos[id];
		infos[id].curr_step = 0;
		infos[id].fromup = infos[id].fromdown = NULL;
		
		infos[id].base.field = start_infos_fields + (base_infos[id].i0 + id * 2) * sizeof(bool *);
		int parth = base_infos[id].i1 - base_infos[id].i0;
		for (int i = 0; i <= parth + 1; ++i)
			infos[id].base.field[i] = base_infos[id].field[i];
		free(base_infos[id].field);
	}
	free(base_infos);
	
	for (int id = 0; id < number_threads; ++id)
	{
		check(sem_init(&infos[id].sem_something_happen, pshared, 0));
		check(sem_init(&infos[id].mutex, pshared, 1));
	}
	
	run_workers(infos, number_threads);
	
	for (int id = 0; id < number_threads; ++id)
	{
		sem_destroy(&infos[id].sem_something_happen);
		sem_destroy(&infos[id].mutex);
	}
	
	if (debug)
	{
		printf("Done!\n");
		printf("\n");
	}
	bool **ret = malloc_field(h, w);
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			ret[i][j] = field[i][j];
	my_free(start_field, size);
	return ret;
}

#endif
