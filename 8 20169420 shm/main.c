#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
	Разделяемая память сохраняется после завершения программы.
*/
int main()
{
	bool recreate = 0;
	
	int fd = shm_open("/shm_test3", O_RDWR | O_CREAT, 0777);
	if (fd == -1)
		perror("shm_open"), exit(1);
	
	int size = sizeof(char[80]);
	if (recreate && ftruncate(fd, size) == -1)
		perror("ftruncate"), exit(1);
	
	char *s = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (s == MAP_FAILED)
		perror("mmap"), exit(1);
	
	if (recreate)
		strcpy(s, "Hello!");
	if (!recreate)
		printf("%s\n", s);
	return 0;
}
