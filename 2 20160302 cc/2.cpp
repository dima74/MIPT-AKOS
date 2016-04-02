#include <bits/stdc++.h>
using namespace std;

class C
{
public:
	C()
	{
		printf("Конструктор\n");
	}
	
	~C()
	{
		printf("Деструктор\n");
	}
};

int main()
{
	{
		C c;
		while (1)
		{
			//return 0;
			goto label;
		}
	}
	printf("...\n");
	label:
		printf("метка\n");
	return 0;
}
