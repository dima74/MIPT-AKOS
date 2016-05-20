#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *itoa_(int x)
{
	int copyx = x;
	int numberDigits = 1;
	while (copyx > 9)
	{
		copyx /= 10;
		++numberDigits;
	}
	
	char *s = malloc(numberDigits + 1);
	for (int i = 0; i < numberDigits; ++i, x /= 10)
		s[numberDigits - i - 1] = '0' + x % 10;
	s[numberDigits] = 0;
	return s;
}

void puts_(const char *s)
{
	for (int i = 0; s[i] != 0; ++i)
		putc(s[i], stdout);
}

void printf_(const char *s, ...)
{
	va_list args;
	va_start(args, s);
	
	int len = strlen(s);
	for (int i = 0; i < len; ++i)
		if (s[i] == '%')
		{
			char next = s[i + 1];
			switch (next)
			{
				case 'd':;
					int x = va_arg(args, int);
					char *sx = itoa_(x);
					puts_(sx);
					free(sx);
					break;
				case 'c':;
					putc(va_arg(args, int), stdout);
					break;
				case 's':;
					puts_(va_arg(args, char*));
					break;
				case 'f':;
					// TODO
					break;
				default:
					--i;
					break;
			}
			++i;
		}
		else
			putc(s[i], stdout);
	va_end(args);
}

void printf2(const char *s, ...)
{
	va_list args;
	va_start(args, s);
	printf("%d\n", va_arg(args, int));
	//printf("%f\n", va_arg(args, double));
	va_arg(args, int);
	printf("%d\n", va_arg(args, int));
	printf("%d\n", va_arg(args, int));
	printf("%d\n", va_arg(args, int));
	va_end(args);
}

#include <stdint.h>
int main()
{
	printf("%zu\n", sizeof(double));
	printf2("%d %q %d %d %d %d", 77, 1234.5789, 1, 2, 3, 4);
	return 0;
}
