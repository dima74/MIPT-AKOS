#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

bool isDirectorySlow(const char *path)
{
	DIR *dir = opendir(path);
	bool ret = dir != NULL;
	closedir(dir);
	return ret;
}

struct stat statbuf;
bool isDirectory(const char *path)
{
	return stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}

bool isSymlink(const char *path)
{
	return lstat(path, &statbuf) == 0 && S_ISLNK(statbuf.st_mode);
}

int number;
void tree(char *path, int spaces)
{
	int len = strlen(path);
	if (len > 1 && path[len - 1] == '/')
		path[len - 1] = 0;
	if (!isDirectory(path) || isSymlink(path))
		return;
	
	++number;
	for (int i = 0; i < spaces; ++i)
		printf(" ");
	printf("%s\n", path);
	
	DIR *dir = opendir(path);
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
		if (entry->d_name[0] != '.')
		{
			char full_path[256];
			strcpy(full_path, path);
			if (len > 1)
				strcat(full_path, "/");
			strcat(full_path, entry->d_name);
			tree(full_path, spaces + 2);
		}
	closedir(dir);
}

int main()
{
	tree("/home/dima/akos", 0);
	printf("%d\n", number);
	return 0;
}
