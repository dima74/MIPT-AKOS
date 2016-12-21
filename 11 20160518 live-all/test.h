#ifndef TEST_H
#define TEST_H

#include "abstract.h"

bool **run_with_generator(int h, int w, int number_steps, int number_threads, func_generator generator);

int seed_for_generator;
void generator_create_from_seed(bool **field, int h, int w)
{
	srand(seed_for_generator);
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			field[i][j] = rand() % 2;
}

bool **run_on_seed(int h, int w, int number_steps, int number_threads, int seed)
{
	seed_for_generator = seed;
	return run_with_generator(h, w, number_steps, number_threads, generator_create_from_seed);
}

bool **field_for_generator;
void generator_copy_from_field(bool **field, int h, int w)
{
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			field[i][j] = field_for_generator[i][j];
}

bool **run_on_field(int h, int w, int number_steps, int number_threads, bool **field)
{
	field_for_generator = field;
	return run_with_generator(h, w, number_steps, number_threads, generator_copy_from_field);
}

bool is_field_equals(bool **field0, bool **field1, int h, int w)
{
	for (int i = 0; i < h; ++i)
		for (int j = 0; j < w; ++j)
			if (field0[i][j] != field1[i][j])
				return false;
	return true;
}

bool get(bool **field, int i, int j, int h, int w)
{
	return field[(h + i) % h][(w + j) % w];
}

bool getNextStateSimple(bool **field, int i, int j, int h, int w)
{
	int count = 
		get(field, i - 1, j - 1, h, w) + 
		get(field, i - 1, j, h, w) + 
		get(field, i - 1, j + 1, h, w) + 
		get(field, i, j - 1, h, w) + 
		get(field, i, j + 1, h, w) + 
		get(field, i + 1, j - 1, h, w) + 
		get(field, i + 1, j, h, w) + 
		get(field, i + 1, j + 1, h, w);
	return field[i][j] ? (2 <= count && count <= 3) : count == 3;
}

bool **simple_live_with_generator(int h, int w, int number_steps, func_generator generator)
{
	bool **curr = malloc_field(h, w);
	bool **next = malloc_field(h, w);
	generator(curr, h, w);
	for (int step = 0; step < number_steps; ++step)
	{
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				next[i][j] = getNextStateSimple(curr, i, j, h, w);
		// swap(curr, next);
		bool **temp = curr;
		curr = next;
		next = temp;
	}
	free_field(next, h, w);
	return curr;
}

bool **simple_live_on_field(int h, int w, int number_steps, bool **field)
{
	field_for_generator = field;
	return simple_live_with_generator(h, w, number_steps, generator_copy_from_field);
}

bool **simple_live_on_seed(int h, int w, int number_steps, int seed)
{
	seed_for_generator = seed;
	return simple_live_with_generator(h, w, number_steps, generator_create_from_seed);
}

bool **generate_field(int h, int w, int seed)
{
	bool **field = malloc_field(h, w);
	seed_for_generator = seed;
	generator_create_from_seed(field, h, w);
	return field;
}

void test(int h, int w, int number_steps)
{
	printf("test %dx%d on %d steps\n", h, w, number_steps);
	int seed = rand();
	int max_number_threads = 8;
	if (max_number_threads > h)
		max_number_threads = h;
	
	bool **begin_field = generate_field(h, w, seed);
	bool **end_fields[max_number_threads + 1];
	end_fields[0] = simple_live_on_field(h, w, number_steps, begin_field);
	for (int number_threads = 1; number_threads <= max_number_threads; ++number_threads)
	{
		printf("\trun on %d threads\n", number_threads);
		end_fields[number_threads] = run_on_field(h, w, number_steps, number_threads, begin_field);
	}
	
	//if (0)
	if (debug)
	for (int number_threads = 0; number_threads <= max_number_threads; ++number_threads)
		print_field(end_fields[number_threads], h, w);
	
	for (int number_threads = 1; number_threads <= max_number_threads; ++number_threads)
		assert(is_field_equals(end_fields[0], end_fields[number_threads], h, w));
	
	free_field(begin_field, h, w);
	for (int number_threads = 0; number_threads <= max_number_threads; ++number_threads)
		free_field(end_fields[number_threads], h, w);
}

void complex_test()
{
	srand(time(0));
	test(2, 1, 1);
	test(8, 1, 10);
	test(10, 10000, 10);
	test(10, 100, 1000);
	
	int number = 10;
	while (number--)
	{
		int h = 9 + rand() % 10;
		int w = 1 + rand() % 100;
		int number_operations = 1 + rand() % 1000;
		test(h, w, number_operations);
	}
}

#endif
