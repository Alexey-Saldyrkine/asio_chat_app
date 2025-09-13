#include "client.hpp"
#include "server.hpp"

const char *host = "127.0.0.1";
const char *port = "3737";

int main(int argc, char *argv[]) {
	std::cout << "argc = " << argc << std::endl;
	if (argc <= 1) {
		try {
			asio::io_context io;
			asio::signal_set signals(io, SIGINT, SIGTERM);
			signals.async_wait([&io](auto, auto) {
				io.stop();
			});
			server serv(io, tcp::endpoint(tcp::v4(), 3737));
			io.run();
		} catch (std::exception &e) {
			std::cerr << "server Exception: " << e.what() << std::endl;
		}
	} else {
		std::string name(argv[1]);
		try {
			clientManager client(host, port, name);
			client.do_loop();
		} catch (std::exception &e) {
			std::cerr << "client Exception: " << e.what() << std::endl;
		}
	}
}
