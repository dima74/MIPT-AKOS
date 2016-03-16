#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int spaces = 0;
int number = 0;
void f()
{
	sleep(1);
	if (number == 2)
	{
		//printf("exit %d\n", getpid());
		sleep(10000);
		exit(0);
	}
	++spaces;
	int p = fork();
	if (p > 0)
	{
		--spaces;
		++number;
		for (int i = 0; i < spaces * 2; ++i)
			printf(" ");
		printf("fork %d -> %d\n", getpid(), p);
	}
	else if (p == 0)
	{
		//printf("start %d %d\n", number, getpid());
		number = 0;
	}
	f();
}

int main()
{
	f();
	return 0;
}
