#include <stdio.h>

struct S
{
	/*
	int 	var_i;
	long 	var_l;
	short 	var_s;
	char 	var_c;
	//*/
	
	int 	var_i0;
	int 	var_i1;
	int 	var_i2;
	int 	var_i3;
	int 	var_i4;
	int 	var_i5;
	int 	var_i6;
	int 	var_i7;
	
	int 	var_i0c;
	int 	var_i1c;
	int 	var_i2c;
	int 	var_i3c;
	int 	var_i4c;
	int 	var_i5c;
	int 	var_i6c;
	int 	var_i7c;
	int 	var_i8c;
};

struct S f()
{
	struct S s;
	s.var_i0 = 0;
	s.var_i1 = 1;
	s.var_i2 = 2;
	s.var_i3 = 3;
	s.var_i4 = 4;
	s.var_i5 = 5;
	s.var_i6 = 6;
	s.var_i7 = 7;
	return s;
}

void f2()
{}

int f3()
{
	return 1;
}

int main()
{
	f2();
	f3();
	struct S s = f();
	printf("%d %d %d %d %d %d %d %d\n", s.var_i0, s.var_i1, s.var_i2, s.var_i3, s.var_i4, s.var_i5, s.var_i6, s.var_i7);
	return 0;
}
