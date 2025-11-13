#pragma once
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <memory>
#include <type_traits>
#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include "threadQueue.hpp"
#include "messageTypes.hpp"
#include "../logging.hpp"
#include <unordered_map>

using boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace posix = boost::asio::posix;

struct session;

struct serverData {
	networkId_t serverId = dedicatedId::masterServer;
	std::unordered_map<networkId_t, std::shared_ptr<session>> sessions;
};

struct session: std::enable_shared_from_this<session> {

	static inline networkId_t nextId = dedicatedId::baseOffset;
	tcp::socket socket_;
	std::array<char, 1024> data;
	networkId_t id;
	serializedMessage<messageTypeMap> inputMessage;
	serverData &serverdata;
	std::string name = "";
	session(tcp::socket socket, serverData &server_) :
			socket_(std::move(socket)), id(nextId++), serverdata(server_) {
	}

	void start() {
		sendWelcomeMessage();
	}

	messageMetaData getMessageMetaData(networkId_t to) {
		return messageMetaData { serverdata.serverId, to };
	}
private:

	void sendWelcomeMessage();

	void do_read() {
		do_read_header();
	}
	void do_read_header();
	void do_read_body();

	void processInputMessage();
	void processSetSenderName();
	void processTextMessage();
	void processRequestClientList();
	void sendMessage();
	void do_echo_inputMessage();

	tcp::socket& socketFor(const serializedMessage<messageTypeMap> &msg) {
		return serverdata.sessions[msg.header.metaData.to]->socket_;
	}

	template<typename F>
	void sendSerialzedMessage(serializedMessage<messageTypeMap> &msg,
			F &&lambda) {
		asio::async_write(socketFor(msg), msg.toBuffers(), lambda);
	}

	void logMessage(size_t len) {
		std::cout << "id: " << id << ", read " << len << " bytes \""
				<< std::string(data.data(), len) << "\"\n";
	}
	clientList getClientList() {
		using userVec = clientList::userVec;
		userVec vec;
		for (auto it : serverdata.sessions) {
			vec.emplace_back(it.first, it.second->name);
		}
		std::sort(vec.begin(),vec.end());
		return clientList(vec);
	}

};

void session::sendWelcomeMessage() {
	auto self(shared_from_this());
	setReciverId msg(id);
	serializedMessage<messageTypeMap> ser(msg, getMessageMetaData(id));
	sendSerialzedMessage(ser, [this, self](const boostError &er, size_t len) {
		if (!er) {
			do_read();
		} else {
			throw prependError("session::sendWelcomeMessage()", er);
		}
	});
}

void session::do_read_header() {
	auto self(shared_from_this());
	asio::async_read(socket_, inputMessage.headerBuffer(),
			[this, self](const boostError &er, size_t len) {
				if (!er) {
					do_read_body();
				} else {
					if (er == asio::error::eof) {
						logError("connection closed on id ", id);
					} else {
						throw prependError("session::do_read_header()", er);
					}
				}
			});
}

void session::do_read_body() {
	auto self(shared_from_this());
	inputMessage.reserveSpaceForData();
	asio::async_read(socket_, inputMessage.bodyBuffer(),
			[this, self](const boostError &er, size_t len) {
				if (!er) {
					processInputMessage();
				} else {
					throw prependError("session::do_read_body()", er);
				}
			});

}

void session::processInputMessage() {
	switch (inputMessage.header.type_id) {
	case getTypeId<setSenderName> :
		processSetSenderName();
		break;
	case getTypeId<textMessage> :
		processTextMessage();
		break;
	case getTypeId<requestClientList> :
		processRequestClientList();
		break;
	default:
		throw std::runtime_error(
				"unknown type id: "
						+ std::to_string(inputMessage.header.type_id)
						+ ", socket id: " + std::to_string(id));
		break;
	}

}

void session::processSetSenderName() {
	auto msg = inputMessage.getObject<setSenderName>();
	name = msg.userName;
	logAction("recived new user name on id: ", id, ", name: ", msg.userName);
	do_read();
}
void session::processTextMessage() {
	auto msg = inputMessage.getObject<textMessage>();
	logAction("recived TextMessage on id: ", id," to ",inputMessage.header.metaData.to, ", msg: ", msg.str);
	if (inputMessage.header.metaData.to == serverdata.serverId) {
		do_echo_inputMessage();
	} else {
		sendMessage();
	}
}

void session::sendMessage() {
	auto self(shared_from_this());
	sendSerialzedMessage(inputMessage,
			[this, self](const boostError &er, size_t len) {
				if (!er) {
					do_read();
				} else {
					throw prependError("session::do_echo_inputMessage()", er);
				}
			});
}

void session::do_echo_inputMessage() {
	auto self(shared_from_this());
	auto &metaData = inputMessage.header.metaData;
	metaData.to = metaData.from;
	metaData.from = serverdata.serverId;
	sendSerialzedMessage(inputMessage,
			[this, self](const boostError &er, size_t len) {
				if (!er) {
					do_read();
				} else {
					throw prependError("session::do_echo_inputMessage()", er);
				}
			});
}

void session::processRequestClientList() {
	auto self(shared_from_this());
	auto metaData = inputMessage.getReturnMetaData();
	clientList list = getClientList();
	serializedMessage<messageTypeMap> ser(list, metaData);
	sendSerialzedMessage(ser, [this, self](const boostError &er, size_t len) {
		if (!er) {
			do_read();
		} else {
			throw prependError("session::processRequestClientList()", er);
		}
	});
}

struct server {
	asio::io_context &io_;
	tcp::acceptor acceptor_;
	serverData serverData_;
	server(asio::io_context &io, const tcp::endpoint &endpoint) :
			io_(io), acceptor_(io_, endpoint) {
		do_accept();
	}

	void do_accept() {
		acceptor_.async_accept(
				[this](const boostError &er, tcp::socket socket) {
					if (!er) {
						logAction("new Connection id = ", session::nextId);
						auto tmp = std::make_shared<session>(std::move(socket),
								serverData_);
						serverData_.sessions[tmp->id] = tmp;
						tmp->start();
						do_accept();
					} else {
						throw prependError("server::do_accept()", er);
						//logError("server::do_accept(): ", er.to_string());
					}

				});
	}
};

/*

 struct streamHeader {
 TidT Tid;
 size_t count;
 };

 struct byteStream {
 std::vector<uint8_t> data;
 auto toBuffer() const {
 return boost::asio::buffer(data);
 }
 };

 template<typename T>
 struct PODstream {
 static_assert(std::is_pod<T>::value,"T must be POD");
 std::vector<T> data;

 auto toBuffer() const {
 return boost::asio::buffer(data);
 }
 };

 struct tcp_connection: std::enable_shared_from_this<tcp_connection> {
 tcp::socket socket_;
 using shrdPtrT = std::shared_ptr<tcp_connection>;

 static shrdPtrT create(boost::asio::io_context &io) {
 return shrdPtrT(new tcp_connection(io));
 }

 tcp::socket& socket() {
 return socket_;
 }

 void write(const byteStream &data) {
 boost::asio::async_write(socket_, data.toBuffer(),
 std::bind(&tcp_connection::handle_write, shared_from_this(),
 boost::asio::placeholders::error,
 boost::asio::placeholders::bytes_transferred));
 }

 void read(byteStream &data) {

 }
 private:
 tcp_connection(boost::asio::io_context &io) :
 socket_(io) {
 }

 void handle_write(const boost::system::error_code &errCode, size_t size) {

 }

 };

 struct networkBase {
 boost::asio::io_context &io;
 networkBase(boost::asio::io_context &ioV) :
 io(ioV) {

 }
 };

 struct tcp_server: networkBase {
 tcp::acceptor acceptor_;
 tcp_server(boost::asio::io_context &ioV) :
 networkBase(ioV), acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
 }

 private:
 void start_accept() {
 tcp_connection::shrdPtrT new_connection = tcp_connection::create(io);
 acceptor_.async_accept(new_connection->socket(),
 std::bind(&tcp_server::handle_accept, this, new_connection,
 boost::asio::placeholders::error));
 }

 void handle_accept(tcp_connection::shrdPtrT new_connection,
 const boost::system::error_code &error) {
 if (!error) {

 }
 start_accept();
 }

 };

 struct tcp_client: networkBase {
 tcp_client(boost::asio::io_context &ioV) :
 networkBase(ioV) {
 start_connection();
 }

 private:
 void start_connection() {

 }

 };
 */
