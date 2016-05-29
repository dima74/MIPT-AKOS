#define pshared 0

#include "live-threads-processors-common.h"

void *my_malloc(size_t size)
{
	return malloc(size);
}

void my_free(void *data, size_t size)
{
	free(data);
}

void *thread_start(void *arg)
{
	worker((struct worker_info *) arg);
	return NULL;
}

void run_workers(struct thread_info *infos, int number_threads)
{
	pthread_t *ids = malloc(number_threads * sizeof(pthread_t));
	for (int id = 0; id < number_threads; ++id)
		pthread_create(&ids[id], NULL, &thread_start, (void*) &infos[id]);
	
	
	
	for (int id = 0; id < number_threads; ++id)
		pthread_join(ids[id], NULL);
	free(ids);
}
