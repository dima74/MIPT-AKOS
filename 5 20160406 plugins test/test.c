#include <time.h>

int getTime()
{
	static int t = time(0);
	return t;
}
