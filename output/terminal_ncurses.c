#include <curses.h>
#include <locale.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>



int init_terminal_ncurses(int col, int bgcol) {

	initscr();
	curs_set(0);
	timeout(0);
	noecho();
	start_color();
	use_default_colors();
	init_pair(1, col, bgcol);
	if(bgcol != -1) {
		bkgd(COLOR_PAIR(1));
	}
	attron(COLOR_PAIR(1));
	return 0;
}


void get_terminal_dim_ncurses(int *w, int *h) {

	getmaxyx(stdscr,*h,*w);
	//better clear in case of resize
	clear(); 
}


int draw_terminal_ncurses(int v, //virtualconsole
						  int h, //height
						  int w, //width
						  int bars, //#bars
						  int bw, //bar width
						  int bs, //bar size
						  int rest, 
						  int f[200], 
						  int flastd[200]) {

	int i, n, x; //tmp index
	const wchar_t* bh[] = { L"\u2581",  //0 lower 1/8th block
						    L"\u2582",  //1 lower 1/4th block
						    L"\u2583",  //2 lower 3/8ths block
						    L"\u2584",  //3 lower 1/2 block
						    L"\u2585",  //4 lower 5/8ths blocks
						    L"\u2586",  //5 lower 3/4ths block
						    L"\u2587",  //6 lower 7/8ths block
						    L"\u2588"}; //7 full block

	// checking if terminal resize has occured
	if (v != 0) {
		if ( LINES != h || COLS != w) {
			return -1;
		}
	}

	h--;

	for (i = 0; i <  bars; i++) {

		if(f[i] > flastd[i]){//if higher than previous
			if (v == 0) {
				for (n = flastd[i] / 8; n < f[i] / 8; n++) {
					for (x = 0; x < bw; x++) {
						//int mvprintw(int y, int x, char *fmt, ...);
						mvprintw((h - n), i*bw + x + i*bs + rest, "%d",8);
					}
				}		
			} else {
				for (n = flastd[i] / 8; n < f[i] / 8; n++) {
					for (x = 0; x < bw; x++) {
						//int mvaddwstr(int y, int x, const wchar_t *wstr);
						mvaddwstr((h - n), i*bw + x + i*bs + rest, bh[7]);
					}
				}		
			}
			if (f[i] % 8 != 0) {
				if (v == 0) {
					for (x = 0; x < bw; x++) {
						//int mvprintw(int y, int x, char *fmt, ...);
						mvprintw( (h - n), i*bw + x + i*bs + rest, "%d",(f[i] % 8) );
					}
				} else {
					for (x = 0; x < bw; x++) {
						//int mvaddwstr(int y, int x, const wchar_t *wstr);
						mvaddwstr( (h - n), i*bw + x + i*bs + rest, bh[(f[i] % 8) - 1]);
					}
				}

			}
		} else if(f[i] < flastd[i]) {//if lower than previous
			for (n = f[i] / 8; n < flastd[i]/8 + 1; n++) {
				for (x = 0; x < bw; x++) {
					//int mvaddstr(int y, int x, const char *str); (no wide char for space of course)
					mvaddstr( (h - n), i*bw + x + i*bs + rest, " ");
				}
			}		
			if (f[i] % 8 != 0) {
				if (v == 0) {
					for (x = 0; x < bw; x++) {
						//int mvprintw(int y, int x, char *fmt, ...);
						mvprintw((h - f[i] / 8), i*bw + x + i*bs + rest, "%d",(f[i] % 8) );
					}
				} else {
					for (x = 0; x < bw; x++) {
						//int mvaddwstr(int y, int x, const wchar_t *wstr);
						mvaddwstr((h - f[i] / 8), i*bw + x + i*bs + rest, bh[(f[i] % 8) - 1]);
					}
				}		
			}
		}
		flastd[i] = f[i]; //memory for falloff func
	}

	refresh();
	return 0;
}


// general: cleanup
void cleanup_terminal_ncurses(void)
{
	echo();
	system("setfont  >/dev/null 2>&1");
	system("setfont /usr/share/consolefonts/Lat2-Fixed16.psf.gz  >/dev/null 2>&1");
	system("setterm -blank 10");
	endwin();
	system("clear");
}

