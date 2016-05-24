#ifndef BASE_H
#define BASE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

bool debug = 0;
#define check(expr)			\
{							\
	if (expr == -1)			\
	{						\
		perror(#expr);		\
		exit(1);			\
	}						\
}

struct worker_info
{
	int id;
	//int h;
	int w;
	int i0;
	int i1;
	int number_steps;
	int number_threads;
	bool **field; // размера (i1 - i0 + 2) * w, часть в общем поле --- [i0, i1) * [0, w)
};

void print_field(bool **field, int h, int w)
{
	for (int i = 0; i < h; ++i)
	{
		printf("%p: ", field[i]);
		for (int j = 0; j < w; ++j)
			printf("%d", field[i][j]);
		printf("\n");
	}
	printf("\n");
}

#include <unistd.h>
void print_field_named(int id, const char *name, bool **field, int h, int w)
{
	usleep(id * 1e5); 
	printf("%d %s:\n", id, name);
	print_field(field, h, w);
}

#endif
