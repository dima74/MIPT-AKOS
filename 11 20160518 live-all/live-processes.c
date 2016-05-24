#define pshared 1

#include "live-threads-processors-common.h"
#include "test.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_NAME "/shm_live"

void *my_malloc(size_t size)
{
	int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0777);
	if (fd == -1)
		perror("shm_open"), exit(1);
	
	if (ftruncate(fd, size) == -1)
		perror("ftruncate"), exit(1);
	
	char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
		perror("mmap"), exit(1);
	return data;
}

void my_free(void *data, size_t size)
{
	check(munmap(data, size));
	check(shm_unlink(SHM_NAME));
}

void run_workers(struct thread_info *infos, int number_threads)
{
	pid_t *pids = malloc(number_threads * sizeof(pid_t));
	for (int i = 0; i < number_threads; ++i)
		if ((pids[i] = fork()) == 0)
		{
			worker((struct worker_info *) &infos[i]);
			exit(0);
		}
	
	for (int i = 0; i < number_threads; ++i)
	{
		if (debug)
			printf("%d waitpid %d\n", i, pids[i]);
		waitpid(pids[i], NULL, 0);
	}
	free(pids);
}

int main()
{
	//complex_test();
	test(2, 1, 1);
	//run_on_seed(8, 8, 100, 8, 0);
}
