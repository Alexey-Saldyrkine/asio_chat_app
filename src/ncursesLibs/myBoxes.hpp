#pragma once
#include "myWindows.hpp"

struct colorShowcaseBox: button_interface {
	std::unique_ptr<borderedScreen> textscreen;

	colorShowcaseBox(int y, int x) {
		textscreen = std::make_unique<borderedScreen>(
				screenArea { y, x, 2 + 9 * 2 + 1, 2 + 9 * 2 },
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
		print();
	}
	void print() {
		wclear(*textscreen->innerScreen);
		for (int i = 0; i <= 8; i++) {
			for (int j = 0; j <= 8; j++) {
				wattron(*textscreen->innerScreen, COLOR_PAIR(j+i*9));
				std::string str = std::to_string(j + i * 9);
				if (str.size() < 2)
					str = " " + str;
				wprintw(*textscreen->innerScreen, str.c_str());
			}
		}
		wattrset(*textscreen->innerScreen, A_NORMAL);
		wprintw(*textscreen->innerScreen, "------------------");
		for (int i = 0; i <= 8; i++) {
			for (int j = 0; j <= 8; j++) {
				wattron(*textscreen->innerScreen, COLOR_PAIR(j+i*9) | A_BOLD);
				std::string str = std::to_string(j + i * 9);
				if (str.size() < 2)
					str = " " + str;
				wprintw(*textscreen->innerScreen, str.c_str());
			}
		}
	}
	bool processKey(int c, PANEL *&focusPanel) override {
		return false;
	}
	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();
		processWindowMove(event, textscreen.get());

		print();
		return true;
	}

	void highlight() override {
		textscreen->highlight();
	}
	void lowlight() override {
		textscreen->lowlight();
	}
	void showWindow() override {
		textscreen->show();
	}
	void hideWindow() override {
		textscreen->hide();
	}
};

struct textBox: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	std::string str;
	textBox(const screenArea &area) {
		textscreen = std::make_unique<borderedScreen>(area,
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
	}
	void print() {
		wclear(*textscreen->innerScreen);
		mvwprintw(*textscreen->innerScreen, 0, 0, str.c_str());
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		if (c == KEY_BACKSPACE) {
			if (!str.empty())
				str.pop_back();
		} else {
			str += static_cast<char>(c);
		}
		print();
		return true;
	}
	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();

		processWindowMove(event, textscreen.get());
		processWindowResize(event, textscreen.get());

		print();
		return true;
	}

	void highlight() override {
		textscreen->highlight();
	}
	void lowlight() override {
		textscreen->lowlight();
	}
	void showWindow() override {
		textscreen->show();
	}
	void hideWindow() override {
		textscreen->hide();
	}

};

struct controlWindow: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	windowManagerBase *manager;
	static constexpr int buttonCount = 3;
	static constexpr int width = 1;
	static constexpr int windowLength = 2 + 6;
	static constexpr int windowWidth = 2 + width;
	controlWindow(int y, int x, windowManagerBase *wins) {
		manager = wins;
		textscreen = std::make_unique<borderedScreen>(screenArea { y, x,
				windowWidth, windowLength },
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
		print();
	}
	void print() {
		std::string str { " exit " };
		wprintw(*textscreen->innerScreen, str.c_str());
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		return false;
	}

	void doAction(int i) {
		if (i < 6) {
			manager->isActiveFlag = false;
		} else if (i > 6) {

		}
	}

	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();
		processWindowMove(event, textscreen.get());

		if (event.bstate & BUTTON1_CLICKED) {
			doAction(event.x);
		}

		print();
		return true;
	}

	void highlight() override {
		textscreen->highlight();
	}
	void lowlight() override {
		textscreen->lowlight();
	}
	void showWindow() override {
		textscreen->show();
	}
	void hideWindow() override {
		textscreen->hide();
	}
};

struct windowSession {
	windowSession() {
		initscr();
		start_color();
		use_default_colors();
		initColors();
		cbreak();
		noecho();
		keypad(stdscr, TRUE);
		curs_set(0);
		mousemask(ALL_MOUSE_EVENTS, nullptr);

	}

	screenArea getTerminalArea() {
		screenArea area;
		area.x = 0;
		area.y = 0;
		area.cols = COLS;
		area.lines = LINES;
		return area;
	}

	~windowSession() {
		endwin();
	}
};

struct windowManager: windowManagerBase, windowSession {

	windowManager() {
		addWindow(std::make_unique<controlWindow>(0, 0, this));
	}

	bool isActive() {
		return isActiveFlag;
	}

	void tickWindows() {
		for (auto &win : windows) {
			win->tick();
		}
	}

	void tick() {
		timeout(tickSpeed);
		int c = getch();
		processInput(c);
		tickWindows();
		update_panels();
		doupdate();
	}

	void processInputForThis(int c) {
		if (c == 'q') {
			isActiveFlag = false;
		}
	}

	void processInput(int c) {
		if (c != ERR) {
			if (driver.processInput(c)) {
				processInputForThis(c);
			}
		}
	}

};

