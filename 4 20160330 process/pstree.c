#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

struct proc
{
	bool exists;
	int pid;
	char name[256];
	char state;
	int ppid;
};

struct proc *procs;
struct proc createProc(FILE* stat)
{
	struct proc ret;
	fscanf(stat, "%d %s %c %d", &ret.pid, ret.name, &ret.state, &ret.ppid);
	ret.exists = 1;
	procs[ret.pid] = ret;
	return ret;
}

void add(const char *path)
{
	char stat_path[256];
	strcpy(stat_path, path);
	strcat(stat_path, "/stat");
	FILE* stat = fopen(stat_path, "r");
	struct proc curr = createProc(stat);
	//printf("%d %s %c %d\n", curr.pid, curr.name, curr.state, curr.ppid);
	fclose(stat);
}

bool isNumber(const char *s)
{
	for (int i = 0; s[i] != 0; ++i)
		if (s[i] < '0' || s[i] > '9')
			return 0;
	return 1;
}

int numberProcess;
int getNumberProcess()
{
	int number = 0;
	DIR *dir = opendir("/proc");
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
		if (entry->d_name[0] != '.' && isNumber(entry->d_name))
		{
			int index = atoi(entry->d_name);
			if (index >= number)
				number = index + 1;
		}
	closedir(dir);
	return number;
}

void write_proc(int pid, int spaces)
{
	for (int i = 0; i < spaces; ++i)
		printf(" ");
	printf("%d %s\n", pid, procs[pid].name);
	// n^2
	for (int i = 0; i < numberProcess; ++i)
		if (procs[i].exists && procs[i].ppid == pid)
			write_proc(procs[i].pid, spaces + 2);
}

int main()
{
	numberProcess = getNumberProcess();
	procs = malloc(numberProcess * sizeof(struct proc));
	for (int i = 0; i < numberProcess; ++i)
		procs[i].exists = 0;
	
	DIR *dir = opendir("/proc");
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
		if (entry->d_name[0] != '.' && isNumber(entry->d_name))
		{
			char full_path[256] = "/proc/";
			strcat(full_path, entry->d_name);
				add(full_path);
		}
	closedir(dir);
	
	printf("Process withhout parent:\n");
	for (int i = 0; i < numberProcess; ++i)
		if (procs[i].exists && procs[i].ppid == 0)
			printf("%d %s\n", i, procs[i].name);
	printf("------------------------\n\n");
	
	write_proc(0, 0);
	free(procs);
	return 0;
}
