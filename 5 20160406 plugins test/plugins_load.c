#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

typedef int (*func) ();

int main()
{
	void *handle = dlopen("./test.so", RTLD_LAZY);
	if (handle == NULL)
		perror("dlopen"), exit(1);
	func getTime = dlsym(handle, "getTime");
	if (getTime == NULL)
		perror("dlsym"), exit(1);
	printf("%d", getTime());
	return 0;
}
