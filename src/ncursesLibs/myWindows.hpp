

#pragma once
#include <ncurses.h>
#include <ncurses/panel.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include "colorPairs.hpp"

void destroy_window(WINDOW *win) {
	if (win != nullptr) {
		delwin(win);
	}
}

void destroy_panel(PANEL *panel) {
	if (panel != nullptr) {
		del_panel(panel);
	}
}

struct screenArea {
	int y, x, lines, cols;

	bool isInArea(int y_, int x_) {
		return y <= y_ && y_ < y + lines && x <= x_ && x_ < x + cols;
	}
};

screenArea screenAreaFromWin(WINDOW *win) {
	screenArea ret;
	getmaxyx(win, ret.lines, ret.cols);
	getbegyx(win, ret.y, ret.x);
	return ret;
}

screenArea normalizeToStdScr(screenArea ret) {
	auto stdArea = screenAreaFromWin(stdscr);
	if (ret.x < 0)
		ret.x = 0;
	if (ret.y < 0)
		ret.y = 0;
	if (ret.x + ret.cols > stdArea.cols)
		ret.x = stdArea.cols - ret.cols;
	if (ret.y + ret.lines > stdArea.lines)
		ret.y = stdArea.lines - ret.lines;
	return ret;
}

screenArea insideBorderArea(screenArea area) {
	area.x++;
	area.y++;
	area.cols -= 2;
	area.lines -= 2;
	return area;
}

screenArea calcMoveArea(WINDOW *win, int ry, int rx, int ey, int ex) {
	auto prev = screenAreaFromWin(win);
	prev.x += ex - rx;
	prev.y += ey - ry;
	return normalizeToStdScr(prev);
}

bool isBottomRightEdge(WINDOW *win, int y, int x) {
	auto area = screenAreaFromWin(win);
	int py = area.lines;
	int px = area.cols;
	return py - 4 <= y && y <= py && px - 4 <= x && x <= px;
}

screenArea calcResizeArea(WINDOW *win, int ry, int rx, int ey, int ex) {
	auto area = screenAreaFromWin(win);
	area.cols += ex - rx;
	area.lines += ey - ry;
	return normalizeToStdScr(area);
}

struct basicWindow {
	screenArea area;
	int &lines = area.lines, &cols = area.cols, &y = area.y, &x = area.x; // no structual binding in member decleration
	WINDOW *win;

	basicWindow(screenArea area_) noexcept :
			area(area_), win(newwin(lines, cols, y, x)) {
	}

	basicWindow(const basicWindow &other) noexcept :
			area(other.area), win(dupwin(other.win)) {
	}
	basicWindow(basicWindow &&other) noexcept :
			area(other.area), win(other.win) {
		other.win = nullptr;
	}
	basicWindow& operator=(const basicWindow &other) noexcept {
		area = other.area;
		win = dupwin(other.win);
		return *this;
	}

	basicWindow& operator=(basicWindow &&other) noexcept {
		area = other.area;
		win = other.win;
		other.win = nullptr;
		return *this;
	}

	~basicWindow() {
		destroy_window(win);
	}
	operator WINDOW*() {
		return win;
	}
};

struct button_interface;

void setButtonInterfaceUsrPtr(PANEL *panel, const button_interface *ptr) {
	set_panel_userptr(panel, ptr);
}
button_interface* getButtonInterfaceUsrPtr(PANEL *panel) {
	return reinterpret_cast<button_interface*>(const_cast<void*>(panel_userptr(
			panel)));
}

struct screenBase {
	PANEL *panel;
	screenBase() :
			panel(nullptr) {
	}
	screenBase(PANEL *p) :
			panel(p) {
	}
	virtual ~screenBase() {
		//destroy_panel(panel);
	}
	virtual void setWindow(WINDOW *win) {
		replace_panel(panel, win);
	}
	virtual void resizeMove(const screenArea &area)=0;
	virtual void move(int y, int x)=0;
	virtual void hide() =0;
	virtual void show()=0;
	virtual void highlight()=0;
	virtual void lowlight() =0;
	virtual operator WINDOW*()=0;
};

struct screen: screenBase {
	basicWindow win;

	screen(screenArea area_, button_interface *buttonPtr = nullptr) :
			win(area_) {
		panel = new_panel(win.win);
		setButtonInterfaceUsrPtr(panel, buttonPtr);
		hide();
	}
	screen(const screen &other) :
			win(other.win) {
		panel = new_panel(win.win);
		setButtonInterfaceUsrPtr(panel, getButtonInterfaceUsrPtr(other.panel));
		hide();
	}
	screen(screen &&other) :
			screenBase(other.panel), win(other.win) {
		other.panel = nullptr;
	}
	screen& operator=(const screen &other) {
		win = other.win;
		panel = new_panel(win.win);
		setButtonInterfaceUsrPtr(panel, getButtonInterfaceUsrPtr(other.panel));
		hide();
		return *this;
	}
	screen& operator=(screen &&other) {
		win = std::move(other.win);
		panel = other.panel;
		other.panel = nullptr;
		return *this;
	}
	~screen() {
		destroy_panel(panel);
	}

	operator WINDOW*() override {
		return win.win;
	}

	void resizeMove(const screenArea &area) override {
		win = basicWindow(area);
		replace_panel(panel, win.win);
	}
	void move(int y, int x) override {
		move_panel(panel, y, x);
	}
	void hide() override {
		hide_panel(panel);
	}
	void show() override {
		show_panel(panel);
	}
	void highlight() override {

	}
	void lowlight() override {

	}
};

using screenPtr = std::unique_ptr<screenBase>;

struct button_interface {
	int prevX = 0, prevY = 0;
	size_t id = -1;
	bool holdMode = false;
	bool resizeMode = false;
	button_interface() {
	}
	virtual ~button_interface() {
	}
	void setId(size_t i) {
		id = i;
	}
	size_t getId() {
		return id;
	}
	bool processWindowMove(const MEVENT &event, screen *thisScreen) {
		WINDOW *win = thisScreen->win.win;

		if (holdMode) {
			holdMode = false;
			if ((event.bstate & BUTTON1_RELEASED)) {
				thisScreen->resizeMove(
						calcMoveArea(win, prevY, prevX, event.y, event.x));
				return true;
			}
		}
		if (event.bstate & BUTTON1_PRESSED) {
			if (!isBottomRightEdge(win, event.y, event.x)) {
				holdMode = true;
				prevX = event.x;
				prevY = event.y;
			}
		}
		return false;
	}

	bool processWindowResize(const MEVENT &event, screen *thisScreen) {
		WINDOW *win = thisScreen->win.win;
		if (resizeMode) {
			resizeMode = false;
			if ((event.bstate & BUTTON1_RELEASED)) {
				thisScreen->resizeMove(
						calcResizeArea(win, prevY, prevX, event.y, event.x));
				return true;
			}
		}
		if ((event.bstate & BUTTON1_PRESSED)) {
			if (isBottomRightEdge(win, event.y, event.x)) {
				resizeMode = true;
				prevX = event.x;
				prevY = event.y;
			}
		}
		return false;
	}

	virtual bool processKey(int, PANEL*&) =0;
	virtual bool processMouseEvent(const MEVENT&, PANEL*&) = 0;
	virtual void highlight()=0;
	virtual void lowlight()=0;
	virtual void tick() {
	}
	virtual void showWindow() {
	}
	virtual void hideWindow() {
	}

}
;

struct panelIterator {
	PANEL *panel;
	panelIterator(PANEL *p) :
			panel(p) {
	}
	PANEL* operator*() {
		return panel;
	}
	panelIterator& operator++() {
		panel = panel_below(panel);
		return *this;
	}
	panelIterator& operator--() {
		panel = panel_above(panel);
		return *this;
	}

	friend bool operator==(const panelIterator &a, const panelIterator &b) {
		return a.panel == b.panel;
	}
	friend bool operator!=(const panelIterator &a, const panelIterator &b) {
		return !(a == b);
	}

	static panelIterator begin() {
		return {panel_below(nullptr)};
	}
	static panelIterator end() {
		return {nullptr};
	}

};

struct panelRange {
	panelIterator begin() {
		return panelIterator::begin();
	}
	panelIterator end() {
		return panelIterator::end();
	}
};

struct buttonProcessDriver {
	PANEL *focusPanel = nullptr;
	bool holdingOnPanel = false;
	bool processInput(int c) {
		if (c == KEY_MOUSE) {
			return !processMouse();
		} else if (focusPanel != nullptr) {
			processKey(c);
			return false;
		} else {
			return true;
		}

	}

	void normalizeEvent(MEVENT &event, const screenArea &area) {
		event.x -= area.x;
		event.y -= area.y;
	}

	void unhighlight() {
		if (focusPanel != nullptr) {
			button_interface *ptr2 = getButtonInterfaceUsrPtr(focusPanel);
			if (ptr2 != nullptr) {
				ptr2->lowlight();
			}
		}
	}

	bool processMouse() {
		MEVENT event;
		if (getmouse(&event) == OK) {
			if (holdingOnPanel == true) {
				holdingOnPanel = false;
				if (focusPanel != nullptr
						&& (event.bstate & BUTTON1_RELEASED)) {
					button_interface *ptr = getButtonInterfaceUsrPtr(
							focusPanel);
					if (ptr != nullptr) {
						screenArea area;
						getbegyx(panel_window(focusPanel), area.y, area.x);
						getmaxyx(panel_window(focusPanel), area.lines,
								area.cols);
						normalizeEvent(event, area);
						ptr->processMouseEvent(event, focusPanel);
						return true;
					}
				}
			}
			for (auto *it : panelRange { }) {
				screenArea area;
				getbegyx(panel_window(it), area.y, area.x);
				getmaxyx(panel_window(it), area.lines, area.cols);
				if (area.isInArea(event.y, event.x)) {
					normalizeEvent(event, area);
					button_interface *ptr = getButtonInterfaceUsrPtr(it);
					ptr->highlight();
					unhighlight();
					focusPanel = it;
					if (ptr != nullptr) {
						if ((event.bstate & BUTTON1_PRESSED)) {
							holdingOnPanel = true;
						}
						ptr->processMouseEvent(event, focusPanel);
						return true;
					} else {
						return false;
					}
				}
			}
			unhighlight();
			focusPanel = nullptr;
		}
		return false;

	}
	bool processKey(int c) {
		if (focusPanel != nullptr) {
			return getButtonInterfaceUsrPtr(focusPanel)->processKey(c,
					focusPanel);
		} else {
			return false;
		}
	}
};

struct borderedScreen: button_interface, screen {
	screenPtr innerScreen;
	bool highlighted = false;
	borderedScreen(screenArea area, screenPtr &&inner_) :
			screen(area), innerScreen(std::move(inner_)) {
		setButtonInterfaceUsrPtr(panel, static_cast<button_interface*>(this));
		printBorder();
		resizeMoveInner(area);
	}
	~borderedScreen() {
	}

	void printBorder() {
		if (highlighted) {
			highlight();
		} else {
			lowlight();
		}
	}

	void resizeMoveInner(const screenArea &area) {
		innerScreen->resizeMove(insideBorderArea(area));
	}

	void resizeMove(const screenArea &area) override {
		screen::resizeMove(area);
		printBorder();
		resizeMoveInner(area);
	}
	void move(int y, int x) override {
		screen::move(y, x);
		innerScreen->move(y + 1, x + 1);
	}

	void hide() override {
		screen::hide();
		innerScreen->hide();
	}

	void show() override {
		screen::show();
		innerScreen->show();
	}

	void highlight() override {
		highlighted = true;
		box(screen::win.win, '#', '#');
	}

	void lowlight() override {
		highlighted = false;
		box(screen::win.win, '|', '-');
	}

	bool processKey(int, PANEL*&) override {
		return false;
	}

	bool processMouseEvent(const MEVENT&, PANEL *&focusPanel) {
		hide();
		show();
		focusPanel = innerScreen->panel;
		return true;
	}
};

struct windowManagerBase {
	bool isActiveFlag = true;
	buttonProcessDriver driver;
	int tickSpeed = 500;
	std::vector<std::unique_ptr<button_interface>> windows;
	void addWindow(std::unique_ptr<button_interface> ptr) {
		ptr->showWindow();
		ptr->setId(windows.size());
		windows.push_back(std::move(ptr));
	}

	void removeWindow(size_t i) {
		if (i < windows.size()) {
			windows[i].reset();
		}
	}
};


