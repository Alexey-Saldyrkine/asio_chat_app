#pragma once
#include <ncurses.h>

void initColors() {
	//short int DEFAULT_FORGROUND, DEFAULT_BACKGROUND;
	// 9 colors
	// 0 - default color
	// 1 - black
	// 2 - red
	// 3 - green
	// 4 - yellow
	// 5 - blue
	// 6 - magenta
	// 7 - cyan
	// 8 - white
	// pair = fg_color + bg_color*9;
	// bg def
	//short int colors[8] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
	//COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };
	for (short int fg = 0; fg <= 8; fg++) {
		for (short int bg = 0; bg <= 8; bg++) {

			short int fgColor = fg - 1;
			short int bgColor = bg - 1;
			init_pair(fg + 9 * bg, fgColor, bgColor);

		}
	}
}

void setFgSameAsBg(WINDOW *win) {
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int bgColor = curPair / 9;
	wattron(win, COLOR_PAIR(bgColor + bgColor*9));
}

void setBgSameAsFg(WINDOW *win) {
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int fgColor = curPair % 9;
	wattron(win, COLOR_PAIR(fgColor + fgColor*9));
}

void setFgColor(WINDOW *win, short int fg) {
	short int fgColor = (fg == 99 ? 0 : fg + 1);
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int bgColor = curPair / 9;
	//std::string str = std::to_string(fgColor) + std::to_string(bgColor);
	//wprintw(win, str.c_str());
	wattron(win, COLOR_PAIR(fgColor + bgColor*9));
}

void setBgColor(WINDOW *win, short int bg) {
	short int bgColor = (bg == 99 ? 0 : bg + 1);
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int fgColor = curPair % 9;
	//std::string str = std::to_string(fgColor) + std::to_string(bgColor);
	//wprintw(win, str.c_str());
	wattron(win, COLOR_PAIR(fgColor + bgColor*9));
}

void setFgColorBright(WINDOW *win, short int fg) {
	short int fgColor = (fg == 99 ? 0 : fg + 1);
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int bgColor = curPair / 9;
	//std::string str = std::to_string(fgColor) + std::to_string(bgColor);
	//wprintw(win, str.c_str());
	wattron(win, COLOR_PAIR(fgColor + bgColor*9) | A_BOLD);
}

void setBgColorBright(WINDOW *win, short int bg) {
	short int bgColor = (bg == 99 ? 0 : bg + 1);
	chtype cur = winch(win);
	short int curPair = cur & A_COLOR;
	short int fgColor = curPair % 9;
	//std::string str = std::to_string(fgColor) + std::to_string(bgColor);
	//wprintw(win, str.c_str());
	wattron(win, COLOR_PAIR(fgColor + bgColor*9) | A_BOLD);
}
