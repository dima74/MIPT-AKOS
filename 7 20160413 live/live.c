#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

// В пустой (мёртвой) клетке, рядом с которой ровно три живые клетки, зарождается жизнь;
// Если у живой клетки есть две или три живые соседки, то эта клетка продолжает жить;
// D противном случае (если соседей меньше двух или больше трёх) клетка умирает («от одиночества» или «от перенаселённости»)
bool getNextState(bool **field, int h, int w, int i, int j)
{
	int count = 0;
	for (int di = -1; di <= 1; ++di)
		for (int dj = -1; dj <= 1; ++dj)
			if (di || dj)
			{
				int ni = (i + di + h) % h;
				int nj = (j + dj + w) % w;
				count += field[ni][nj];
			}
	return field[i][j] ? (2 <= count && count <= 3) : count == 3;
}

void step(bool **field, bool **next, int h, int w, int i0, int j0, int i1, int j1)
{
	for (int i = i0; i <= i1; ++i)
		for (int j = j0; j <= j1; ++j)
			next[i][j] = getNextState(field, h, w, i, j);
}

bool **malloc_field(int h, int w)
{
	bool **field = malloc(h * sizeof(char*));
	for (int i = 0; i < h; ++i)
		field[i] = malloc(w * sizeof(char));
	return field;
}

void free_field(bool **field, int h, int w)
{
	for (int i = 0; i < h; ++i)
		free(field[i]);
	free(field);
}

bool **generate_field(int h, int w)
{
	bool **field = malloc_field(h, w);
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			field[i][j] = rand() % 2;
	return field;
}

int number_threads = 4;
int number_steps = 10;
int h;
int w;
bool **curr;
bool **next;

pthread_mutex_t mutex_current = PTHREAD_MUTEX_INITIALIZER;

sem_t sem;
int my_getsem_value(sem_t *sem)
{
	int sem_val;
	sem_getvalue(sem, &sem_val);
	return sem_val;
}


int number_done;
pthread_mutex_t mutex_number_done = PTHREAD_MUTEX_INITIALIZER;

int current_step;
pthread_mutex_t mutex_current_step = PTHREAD_MUTEX_INITIALIZER;

void increment_current_step()
{
	pthread_mutex_lock(&mutex_current_step);
	++current_step;
	pthread_mutex_unlock(&mutex_current_step);
}

int get_current_step()
{
	int ret;
	pthread_mutex_lock(&mutex_current_step);
	ret = current_step;
	pthread_mutex_unlock(&mutex_current_step);
	return ret;
}

static void *thread_start(void *arg)
{
	int id = (size_t) arg;
	int i0 = h / number_threads * id;
	int j0 = 0;
	int i1 = i0 + h / number_threads - 1;
	int j1 = w - 1;
	//printf("%d (%2d %2d  %2d %2d)\n", id, i0, j0, i1, j1);
	
	for (int i = 0; i < number_steps; ++i)
	{
		//printf("%d %d %d\n", id, i, current_step);
		step(curr, next, h, w, i0, j0, i1, j1);
		printf("%d lock done\n", id);
		pthread_mutex_lock(&mutex_number_done);
		printf("%d get done\n", id);
		++number_done;
		if (number_done == number_threads)
		{
			printf("%d lock current\n", id);
			pthread_mutex_lock(&mutex_current);
			printf("%d get current\n", id);
			// swap(curr, next);
			bool **temp = curr;
			curr = next;
			next = curr;
			increment_current_step();
			pthread_mutex_unlock(&mutex_current);
			
			printf("\n");
			printf("Шаг %d\n", get_current_step());
			for (int id = 1; id < number_threads; ++id)
				sem_post(&sem);
			number_done = 0;
			pthread_mutex_unlock(&mutex_number_done);
		}
		else
		{
			pthread_mutex_unlock(&mutex_number_done);
			
			printf("%d lock sem  (%d)\n", id, my_getsem_value(&sem));
			sem_wait(&sem);
			printf("%d get sem  (%d)\n", id, my_getsem_value(&sem));
		}
	}
	return NULL;
}

void clear_screen()
{
	for (int i = 0; i < 25; ++i)
		printf("\n");
}

void show_field(bool **field, int h, int w)
{
	clear_screen();
	printf("Шаг %d:\n", get_current_step());
	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
			printf("%d ", field[i][j]);
		printf("\n");
	}
}

int main()
{
	srand(time(0));
	h = 20;
	w = 20;
	curr = generate_field(h, w);
	next = malloc_field(h, w);
	show_field(curr, h, w);
	
	sem_init(&sem, 0, 0);
	pthread_mutex_init(&mutex_number_done, NULL);
	
	pthread_t id; //unused
	for (size_t i = 0; i < number_threads; ++i)
		pthread_create(&id, NULL, &thread_start, (void*) i);
	
	char s[80];
	while (1)
	{
		gets(s);
		if (strcmp(s, "exit") == 0 || strcmp(s, "e") == 0)
			break;
		if (strcmp(s, "show") == 0 || strcmp(s, "s") == 0)
		{
			bool **copy = malloc_field(h, w);
			pthread_mutex_lock(&mutex_current);
			for (int i = 0; i < h; ++i)
				for (int j = 0; j < w; ++j)
					copy[i][j] = curr[i][j];
			pthread_mutex_unlock(&mutex_current);
			
			show_field(copy, h, w);
			free_field(copy, h, w);
		}
	}
	
	free_field(curr, h, w);
	free_field(next, h, w);
	sem_destroy(&sem);
	return 0;
}
