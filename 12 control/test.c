#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

long getmtime()
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

int main(int argc, char **argv)
{
	printf("start %s %d\n", argv[0], getmtime() - 1464189535549l);
	//sleep(1);
	return 3;
}
