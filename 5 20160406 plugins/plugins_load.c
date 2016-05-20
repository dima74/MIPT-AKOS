#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

typedef int (*func) (int, int);
typedef const char *(*funcGetString) ();

int main()
{
	DIR *dir = opendir("plugins");
	struct dirent *entry;
	void *handles[100];
	size_t number = 0;
	for (size_t i = 0; (entry = readdir(dir)) != NULL; ++i)
		if (entry->d_name[0] != '.')
		{
			char path[256] = "plugins/";
			strcat(path, entry->d_name);
			void *handle = dlopen(path, RTLD_NOW);
			handles[number++] = handle;
		}
	closedir(dir);
	
	printf("Выберете плагин:\n", number);
	for (size_t i = 0; i < number; ++i)
	{
		funcGetString name = dlsym(handles[i], "getName");
		printf("%d: %s\n", i +1, name());
	}
	
	size_t choose;
	scanf("%zu", &choose);
	void *handle = handles[choose - 1];
	funcGetString description = dlsym(handle, "getDescription");
	func f = dlsym(handle, "func");
	
	printf("Введите два числа:\n");
	int a, b;
	scanf("%d%d", &a, &b);
	printf("Результат (%s):\n", description());
	printf("%d\n", f(a, b));
	
	for (size_t i = 0; i < number; ++i)
		dlclose(handles[i]);
	return 0;
}
