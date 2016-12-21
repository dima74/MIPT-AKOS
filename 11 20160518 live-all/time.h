#include "test.h"

#include <stdio.h>

bool **run_with_generator(int h, int w, int number_steps, int number_threads, func_generator generator);

double gettime()
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_sec + tp.tv_nsec / (double)  1e9;
}

double measure_on_seed(int h, int w, int number_steps, int number_threads, int seed)
{
	double time_start = gettime();
	run_on_seed(h, w, number_steps, number_threads, seed);
	double time_end = gettime();
	return time_end - time_start;
}

double measure_on_seed_best(int h, int w, int number_steps, int number_threads, int seed, int number_measures)
{
	printf("\n");
	double time_best = 1e9;
	while (number_measures--)
	{
		double time_curr = measure_on_seed(h, w, number_steps, number_threads, seed);
		printf("%d %f\n", number_threads, time_curr);
		if (time_curr < time_best)
			time_best = time_curr;
	}
	return time_best;
}

const int max_number_threads = 8;
const int number_measures = 3;
void measure_hwnsp(int h, int w, int number_steps, int seed, const char *path)
{
	printf("measure_hwnsp()\n");
	double times[max_number_threads + 1];
	for (int number_threads = 1; number_threads <= max_number_threads; ++number_threads)
		times[number_threads] = measure_on_seed_best(h, w, number_steps, number_threads, seed, number_measures);
	FILE *file = fopen(path, "w");
	fprintf(file, "x y\n");
	for (int number_threads = 1; number_threads <= max_number_threads; ++number_threads)
		fprintf(file, "%d %f\n", number_threads, times[number_threads]);
	fclose(file);
}

void measure_hwns(int h, int w, int number_steps, int seed)
{
	char path[128];
	sprintf(path, "%dx%d_%d.txt", h, w, number_steps);
	measure_hwnsp(h, w, number_steps, seed, path);
}

void measure()
{
	printf("measure()\n");
	int seed = time(0);
	measure_hwns(1200, 100, 100, seed);
}
