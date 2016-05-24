#ifndef ABSTRACT_H
#define ABSTRACT_H

#include "base.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

void send_and_recv(bool *toup, bool *todown, bool *fromup, bool *fromdown, struct worker_info *info);

// В пустой (мёртвой) клетке, рядом с которой ровно три живые клетки, зарождается жизнь;
// Если у живой клетки есть две или три живые соседки, то эта клетка продолжает жить;
// В противном случае (если соседей меньше двух или больше трёх) клетка умирает («от одиночества» или «от перенаселённости»)
bool getNextState(bool **field, int i, int j)
{
	int count = 
		field[i - 1][j - 1] + 
		field[i - 1][j] + 
		field[i - 1][j + 1] + 
		field[i][j - 1] + 
		field[i][j + 1] + 
		field[i + 1][j - 1] + 
		field[i + 1][j] + 
		field[i + 1][j + 1];
	return field[i][j] ? (2 <= count && count <= 3) : count == 3;
}

bool **malloc_field(int h, int w)
{
	bool **field = malloc(h * sizeof(bool*));
	for (int i = 0; i < h; ++i)
		field[i] = malloc(w * sizeof(bool));
	return field;
}

void free_field(bool **field, int h, int w)
{
	for (int i = 0; i < h; ++i)
		free(field[i]);
	free(field);
}

void set_left_and_right_columns(bool **field, int h, int w)
{
	for (int i = 0; i <= h + 1; ++i)
	{
		field[i][0] = field[i][w];
		field[i][w + 1] = field[i][1];
	}
}

void update_bounds(bool **curr, bool **next, struct worker_info *info)
{
	int parth = info->i1 - info->i0;
	int partw = info->w;
	if (info->number_threads == 1)
		for (int j = 1; j <= partw; ++j)
		{
			next[0][j] = next[parth][j];
			next[parth + 1][j] = next[1][j];
		}
	else
	{
		bool *toup = next[1] + 1;
		bool *todown = next[parth] + 1;
		bool *fromup = next[0] + 1;
		bool *fromdown = next[parth + 1] + 1;
		send_and_recv(toup, todown, fromup, fromdown, info);
	}
	set_left_and_right_columns(next, parth, partw);
}

void print(struct worker_info *info, bool **field)
{
	usleep(info->id * 1e5);
	int parth = info->i1 - info->i0;
	int partw = info->w;
	printf("id %d:\n", info->id);
	for (int i = 0; i <= parth + 1; ++i)
	{
		printf("%p: ", field[i]);
		for (int j = 0; j <= partw + 1; ++j)
			printf("%d", field[i][j]);
		printf("\n");
	}
	printf("\n");
}

void worker(struct worker_info *info)
{
	usleep(info->id * 1e3);
	int parth = info->i1 - info->i0;
	int partw = info->w;
	
	bool **curr = malloc_field(parth + 2, partw + 2);
	bool **next = malloc_field(parth + 2, partw + 2);
	
	for (int i = 0; i <= parth + 1; ++i)
		for (int j = 1; j <= partw; ++j)
			curr[i][j] = info->field[i][j - 1];
	set_left_and_right_columns(curr, parth, partw);
	
	for (int step = 0; step < info->number_steps; ++step)
	{
		//print_field_named(info->id, "curr", curr, parth + 2, partw + 2);
		for (int i = 1; i <= parth; ++i)
			for (int j = 1; j <= partw; ++j)
				next[i][j] = getNextState(curr, i, j);
		update_bounds(curr, next, info);
		// swap(curr, next);
		bool **temp = curr;
		curr = next;
		next = temp;
	}
	
	//print_field_named(info->id, "last-curr", curr, parth + 2, partw + 2);
	
	for (int i = 1; i <= parth; ++i)
		for (int j = 1; j <= partw; ++j)
			info->field[i][j - 1] = curr[i][j];
	free_field(curr, parth, partw);
	free_field(next, parth, partw);
}

struct worker_info *create_infos(bool **field, int h, int w, int number_steps, int number_threads)
{
	struct worker_info *infos = malloc(number_threads * sizeof(struct worker_info));
	for (int id = 0; id < number_threads; ++id)
	{
		struct worker_info info;
		info.id = id;
		//info.h = h;
		info.w = w;
		info.i0 = id * h / number_threads;
		info.i1 = (id + 1) * h / number_threads;
		info.number_steps = number_steps;
		info.number_threads = number_threads;
		
		int parth = info.i1 - info.i0;
		info.field = malloc((parth + 2) * sizeof(bool *));
		for (int i = 0; i <= parth + 1; ++i)
			info.field[i] = field[(h + info.i0 + i - 1) % h];
		
		infos[id] = info;
	}
	infos[number_threads - 1].i1 = h;
	
	// debug
	int sumh = 0;
	for (int i = 0; i < number_threads; ++i)
		sumh += infos[i].i1  - infos[i].i0;
	assert(sumh == h);
	
	return infos;
}

#endif
