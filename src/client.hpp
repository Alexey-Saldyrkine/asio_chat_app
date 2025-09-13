#pragma once
#include "ncursesLibs/clientConsole.hpp"
#include "networkLibs/clientNetNetwork.hpp"
#include <thread>
#include <unordered_map>

struct networkClient {
	asio::io_context io;
	tcp::resolver resolver;
	threadQueue<serializedMessage<messageTypeMap>> in, out;
	clientNet network;
	asio::signal_set signals;
	std::thread io_run_thread;
	networkClient(const char *host, const char *port, const std::string &name) :
			resolver(io), network(io, in, out, resolver.resolve(host, port),
					name), signals(io, SIGINT, SIGTERM) {
		signals.async_wait([this](auto, auto) {
			io.stop();
		});
		io_run_thread = std::thread([this]() {
			io.run();
		});
	}

	void sendSerializedMessage(const serializedMessage<messageTypeMap> &ser) {
		out.push(ser);
	}

	messageMetaData getMessageMetaData(int to) {
		return network.getMessageMetaData(to);
	}
//	void do_loop() {
//		std::string str = "";
//		while (str != "q") {
//			while (!in.empty()) {
//				serializedMessage<messageTypeMap> serIn = in.pop();
//				textMessage msgIn = serIn.getObject<textMessage>();
//				std::cout << msgIn.str << std::endl;
//			}
//			std::cin >> str;
//			textMessage msg(str);
//			serializedMessage<messageTypeMap> ser(msg,
//					network.getMessageMetaData(dedicatedId::masterServer));
//			out.push(ser);
//		}
//	}

	~networkClient() {
		io.stop();
		io_run_thread.join();
	}
};

struct ClientListBox: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	networkClient &network;
	int selection = -1;
	int listCount;
	ClientListBox(const screenArea &area, networkClient &network_) :
			network(network_) {
		textscreen = std::make_unique<borderedScreen>(area,
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
		listCount = area.lines - 2;
		//WINDOW *win = *textscreen->innerScreen;

	}

	std::string idToName(int id) {
		return std::to_string(id);
	}

	networkId_t getSelectedId() const {
		auto &list = network.network.clientNames;
		auto it = list.begin();
		for (int i = 0; i < selection; i++)
			it++;
		return it->first;
	}

	void printClientList() {
		WINDOW *win = *textscreen->innerScreen;
		int n = getmaxy(win);
		unsigned int l = getmaxx(win);
		auto &list = network.network.clientNames;
		auto it = list.begin();
		for (int i = 0; i < n; i++) {
			if (it != list.end()) {
				std::string str = std::to_string(it->first) + ": " + it->second;
				if (str.size() > l) {
					str = str.substr(0, l);
				}
				if (i == selection) {
					setBgColor(win, COLOR_CYAN);
					mvwprintw(win, i, 0, str.c_str());
					setBgColor(win, COLOR_WHITE);
					setBgColor(win, 99);
				} else {
					mvwprintw(win, i, 0, str.c_str());
				}
				it++;
			} else {
				break;
			}
		}
	}

	void sendRequestToUpdateClientList() {
		requestClientList req;
		serializedMessage<messageTypeMap> ser(req,
				network.getMessageMetaData(dedicatedId::masterServer));
		network.sendSerializedMessage(ser);
	}
	int printCounter = 11;
	void print() {
		printCounter++;
		if (printCounter > 6) {
			printCounter = 0;
			sendRequestToUpdateClientList();
		}
		printClientList();
		//mvwprintw(*textscreen->innerScreen, 0, 0, str.c_str());
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		print();
		if (c == 'w' || c == KEY_UP) {
			selection--;
			selection = std::max(selection, 0);
		}
		if (c == 's' || c == KEY_DOWN) {
			selection++;
			selection = std::min(selection, listCount);
		}
		return true;
	}
	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();
		focusPanel = textscreen->innerScreen->panel;
		selection = event.y;

		//processWindowMove(event, textscreen.get());
		//processWindowResize(event, textscreen.get());
		//WINDOW *win = *textscreen->innerScreen;
		print();
		return true;
	}

	void tick() override {
		print();
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

struct textInputBox: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	std::string str;
	networkClient &network;
	const ClientListBox &clients;
	int to = dedicatedId::masterServer;
	textInputBox(const screenArea &area, networkClient &network_,
			const ClientListBox &clb) :
			network(network_), clients(clb) {
		textscreen = std::make_unique<borderedScreen>(area,
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
	}
	void print() {
		wclear(*textscreen->innerScreen);
		mvwprintw(*textscreen->innerScreen, 0, 0, str.c_str());
	}

	serializedMessage<messageTypeMap> serializeString(const std::string &str) {
		textMessage msg(str);
		return serializedMessage<messageTypeMap>(msg,
				network.getMessageMetaData(clients.getSelectedId()));
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		if (c == '\n' || c == KEY_ENTER) {
			auto ser = serializeString(str);
			network.sendSerializedMessage(ser);
			network.in.push(ser);
			str = "";
		} else if (c == KEY_BACKSPACE) {
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

		//processWindowMove(event, textscreen.get());
		//processWindowResize(event, textscreen.get());

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

struct textOutputBox: button_interface {
	std::unique_ptr<borderedScreen> textscreen;
	networkClient &network;
	int to = dedicatedId::masterServer;

	textOutputBox(const screenArea &area, networkClient &network_) :
			network(network_) {
		textscreen = std::make_unique<borderedScreen>(area,
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
		WINDOW *win = *textscreen->innerScreen;
		idlok(win, true);
		scrollok(win, true);
	}

	std::string idToName(int id) {
		auto &map = network.network.clientNames;
		if (map.find(id) != map.end()) {
			return map[id];
		} else {
			return std::to_string(id);
		}
	}

	std::string textMessageToOutput(const textMessage &msg,
			const messageMetaData &data) {
		std::string ret = "";
		ret += "\nfrom " + idToName(data.from) + ": " + msg.str;
		return ret;
	}

	void printAllFromQueue() {
		WINDOW *win = *textscreen->innerScreen;
		while (!network.in.empty()) {
			serializedMessage<messageTypeMap> ser = network.in.pop();
			textMessage msg = ser.getObject<textMessage>();
			wprintw(win, textMessageToOutput(msg, ser.header.metaData).c_str());
		}
	}
	void print() {
		printAllFromQueue();
		//mvwprintw(*textscreen->innerScreen, 0, 0, str.c_str());
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		print();
		return true;
	}
	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();

		//processWindowMove(event, textscreen.get());
		//processWindowResize(event, textscreen.get());
		WINDOW *win = *textscreen->innerScreen;
		idlok(win, true);
		scrollok(win, true);
		print();
		return true;
	}

	void tick() override {
		print();
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

struct comWindow: button_interface {
	std::unique_ptr<borderedScreen> BorderScreen;
	std::unique_ptr<textInputBox> input;
	std::unique_ptr<textOutputBox> output;
	std::unique_ptr<ClientListBox> selectionList;
	networkClient &network;
	std::array<screenArea, 3> areas;
	std::array<screenArea, 3> getScreenAreas(const screenArea &area) {
		// textOutput |textInput
		//			  |--------
		//			  |slection
		int leftCol = area.cols / 2;
		int topRightLines = area.lines / 2;
		screenArea left { area.y, area.x, area.lines, leftCol };
		screenArea topRight { area.y, area.x + leftCol, topRightLines, area.cols
				- leftCol };
		screenArea bottomRight { area.y + topRightLines, area.x + leftCol,
				area.lines - topRightLines, area.cols - leftCol };
		return {left,topRight,bottomRight};
	}

	comWindow(const screenArea &area, networkClient &network_) :
			network(network_) {
		areas = getScreenAreas(area);
		BorderScreen = std::make_unique<borderedScreen>(area,
				std::make_unique<screen>(screenArea { 0, 0, 0, 0 },
						static_cast<button_interface*>(this)));
		output = std::make_unique<textOutputBox>(areas[0], network);
		selectionList = std::make_unique<ClientListBox>(areas[2], network);
		input = std::make_unique<textInputBox>(areas[1], network,
				*selectionList);

	}

	void print() {
		input->print();
		output->print();
		selectionList->print();
	}

	bool processKey(int c, PANEL *&focusPanel) override {
		print();
		return true;
	}
	bool processMouseEvent(const MEVENT &event, PANEL *&focusPanel) override {
		highlight();
		hideWindow();
		showWindow();
		if (areas[0].isInArea(event.y, event.x)) {

		}

		//processWindowMove(event, textscreen.get());
		//processWindowResize(event, textscreen.get());
		//WINDOW *win = *textscreen->innerScreen;
		print();
		return true;
	}

	void tick() override {
		input->tick();
		output->tick();
		selectionList->tick();
		print();
	}

	void highlight() override {
		BorderScreen->highlight();
		//input->highlight();
		//output->highlight();
		//selectionList->highlight();
	}
	void lowlight() override {
		BorderScreen->lowlight();
		//input->lowlight();
		//output->lowlight();
		//selectionList->lowlight();
	}
	void showWindow() override {
		BorderScreen->showWindow();
		input->showWindow();
		output->showWindow();
		selectionList->showWindow();
	}
	void hideWindow() override {
		BorderScreen->hideWindow();
		input->hideWindow();
		output->hideWindow();
		selectionList->hideWindow();
	}

};

struct clientManager: windowManager {
	networkClient network;
	clientManager(const char *host, const char *port, const std::string &name) :
			network(host, port, name) {
		/*addWindow(
		 std::make_unique<textInputBox>(screenArea { 0, 15, 10, 30 },
		 network));
		 addWindow(
		 std::make_unique<textOutputBox>(screenArea { 10, 15, 10, 30 },
		 network));
		 addWindow(
		 std::make_unique<ClientListBox>(screenArea { 10, 55, 10, 20 },
		 network));*/
		auto area = getTerminalArea();
		area.y += 3;
		area.lines -= 3;
		addWindow(std::make_unique<comWindow>(area, network));
	}
	void do_loop() {
		while (isActiveFlag) {
			tick();
		}
	}
};
