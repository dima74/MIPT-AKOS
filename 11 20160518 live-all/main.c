#include "base.h"
#include "live-threads.h"
#include "test.h"

#include <ncurses.h>
#include <locale.h>

void show_field(int step, bool **field, int h, int w)
{
	static chtype c0 = ' ' | COLOR_PAIR(1);
	static chtype c1 = ' ' | COLOR_PAIR(2);
	static bool help = 1;
	clear();
	move(0, 0);
	if (help)
	{
		printw("Нажмите любую клавишу для перехода к следующему поколению\n");
		help = 0;
	}
	else
		printw("Шаг %d:\n", step);
	for (int i = 0; i < h; ++i)
	{
		for (int j = 0; j < w; ++j)
			addch(field[i][j] ? c1 : c0);
		printw("\n");
	}
	refresh();
}

void visual()
{
	int ncurses = 0;
	if (ncurses)
	{
		initscr();
		keypad(stdscr, TRUE);
		start_color();
		init_pair(1, COLOR_BLACK, COLOR_RED);
		init_pair(2, COLOR_BLACK, COLOR_GREEN);
	}
	
	int h = 20;
	int w = 40;
	int nubmer_threads = 2;
	bool **field = run_on_seed(h, w, 1, nubmer_threads, 0);
	if (ncurses)
		show_field(0, field, h, w);
	for (int step = 0; step < 1e4; ++step)
	{
		//printf("%d\n", step);
		if (ncurses)
			getch();
		field = run_on_field(h, w, 1, nubmer_threads, field);
		if (ncurses)
			show_field(step + 1, field, h, w);
	}
	
	if (ncurses)
		endwin();
}

int main()
{
	setlocale(LC_ALL, "");
	//visual();
	complex_test();
	return 0;
}
