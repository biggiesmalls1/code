/** matrix.c
 *  by joey
**/

/** TODO:
 *  Altering happens too fast wrt sliding.  Run them in parallel.
 *  Wider screen => more parallelisation.
 *  Some kind of timing so that it doesn't go too fast on new machines.
 *  Make the white things more like original (eg. changing symbol, sometimes static, ...).
 *  In XMatrix, columns do not slide at different speeds!
 *  Putty compatability?
 *  Turn cursor off.
 *  Exit cleanly on keypress.
**/

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>

void writeToPoint(int x,int y) {
	mvaddch(y,x,'.');
}

void doSummat(void writeFn(int,int)) {
	// ...
	writeFn(12,16);
	// ...
}

void cls() {
	for (int x=0;x<COLS;x++) {
		for (int y=0;y<LINES;y++) {
			mvaddch(y,x,32);
		}
	}
}

#define mbit char

void main() {

	mbit** thematrix;
	
	mbit* palette = "^+ouq/\\*}0&$@#";
	int paletteSize = strlen(palette);
	// mbit* altPalette = "%@#";
	// int altPaletteSize = strlen(altPalette);
	
	srand(time(NULL));
	// Needed
	initscr(); cbreak(); noecho();
	// Optional
	nonl(); intrflush(stdscr,FALSE); keypad(stdscr,FALSE);
	printf("%i x %i\n",COLS,LINES);

	if (has_colors()) {
		start_color();
		init_pair(0,COLOR_BLACK,COLOR_BLACK);
		init_pair(1,COLOR_RED,COLOR_BLACK);
		init_pair(2,COLOR_GREEN,COLOR_BLACK);
		init_pair(3,COLOR_YELLOW,COLOR_BLACK);
		init_pair(4,COLOR_BLUE,COLOR_BLACK);
		init_pair(5,COLOR_MAGENTA,COLOR_BLACK);
		init_pair(6,COLOR_CYAN,COLOR_BLACK);
		init_pair(7,COLOR_WHITE,COLOR_BLACK);
	}

	cls();

	thematrix = new mbit*[COLS];
	for (int x=0;x<COLS;x++) {
		thematrix[x] = new mbit[LINES];
	}

	move(0,0);
	for (int x=0;x<COLS;x++) {
		for (int y=0;y<LINES;y++) {
			thematrix[x][y] = ' ';
			move(y,x);
			addch(' ');
		}
	}

	// base it on area instead of width
	int AVSLIDELEN = ( LINES>30 ? 2 : 1 );
	int AVALTLET = ( LINES>30 ? 4 : 2 );

	while (true) {

		int action = rand() % 100;

		if (action < 10) {

			// Alter: add some new symbols and a white "active" symbol (goes too fast atm)

			int x = rand() % COLS;
			int y = rand() % (int)((double)LINES * 4);
			if (y>=LINES)
				y=0;

			attrset(COLOR_PAIR(2));
			do {

				move(y,x);
				attrset(COLOR_PAIR(7) | A_BOLD);
				addch('%');

				wrefresh(stdscr);

				if (y>0) {
					mbit symbol = palette[ rand() % paletteSize ];
					move(y-1,x);
					attrset(COLOR_PAIR(2));
					addch(symbol);
					thematrix[x][y-1] = symbol;

				}

				wrefresh(stdscr);

				y++;

			} while ( (rand() % AVALTLET) != 0 && y < LINES );

			if ( y<LINES && (rand() % 2)==0 ) {
				move(y,x);
				attrset(COLOR_PAIR(7) | A_BOLD);
				addch('%');
				thematrix[x][y] = '%';
			}

		} else if (action < 100) {

			// Slide: slide a whole column down n spaces

			// while ( (rand() % 10) != 0 ) {

				int x = rand() % COLS;

				attrset(COLOR_PAIR(2));
				do {

					for (int y=LINES-1;y>0;y--) {
						mbit src = thematrix[x][y-1];
						thematrix[x][y] = src;
						move(y,x);
						if (src == '%') {
							if ( (rand() % 6*AVSLIDELEN) == 0 ) {
								thematrix[x][y] = palette[ rand() % paletteSize ];
							} else {
								attrset(COLOR_PAIR(7) | A_BOLD );
								addch(thematrix[x][y]);
								attrset(COLOR_PAIR(2));
							}
						} else {
							addch(thematrix[x][y]);
						}
					}
					thematrix[x][0]=' ';
					move(0,x);
					addch(' ');

					wrefresh(stdscr);

				} while ( (rand() % AVSLIDELEN) != 0 );

			// }

		}

		wrefresh(stdscr);

	}

	cls();

	// Needed
	endwin();

}
