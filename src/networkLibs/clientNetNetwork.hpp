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

using boost::asio::ip::tcp;
namespace asio = boost::asio;
namespace posix = boost::asio::posix;

struct clientNet {
	using tq = threadQueue<serializedMessage<messageTypeMap>>;

	asio::io_context &io_;
	tcp::socket socket_;
	serializedMessage<messageTypeMap> inputMessage;
	tq &messagesIn, &messagesOut;
	std::string name_;
	networkId_t id = -1;
	std::unordered_map<networkId_t, std::string> clientNames;
	clientNet(asio::io_context &io, tq &in, tq &out,
			const tcp::resolver::results_type &endpoint, std::string name) :
			io_(io), socket_(io_), messagesIn(in), messagesOut(out), name_(name) {
		do_connect(endpoint);
	}

	messageMetaData getMessageMetaData(networkId_t to =
			dedicatedId::masterServer) {
		return messageMetaData { id, to };
	}
private:
	void do_connect(const tcp::resolver::results_type &endpoint);
	void close();

	void do_write_name_to_server();
	void do_readStdIn();
	void do_write();

	void do_read() {
		do_read_header();
	}
	void do_read_header();
	void do_read_body();

	void processInputMessage();
	void processSetReciverId();
	void processTextMessage();
	void processClientList();

};
void clientNet::close() {
	socket_.close();
}

void clientNet::do_connect(const tcp::resolver::results_type &endpoint) {
	asio::async_connect(socket_, endpoint,
			[this](const boostError &er, tcp::endpoint) {
				if (!er) {
					//logAction("connected to server");
					do_write_name_to_server();
				} else {
					throw prependError("client::do_connect()", er);
					//logError("client::do_connect(): ", er.to_string());
					//close();

				}
			});
}

void clientNet::do_write_name_to_server() {
	setSenderName msg(name_);
	serializedMessage<messageTypeMap> serialized(msg, getMessageMetaData());
	asio::async_write(socket_, serialized.toBuffers(),
			[this](const boostError &er, size_t len) {
				if (!er) {
					do_read();
					do_readStdIn();
				} else {
					throw prependError("client::do_write_name_to_server()", er);
				}
			});
}

void clientNet::do_readStdIn() {
	if (!messagesOut.empty()) {
		do_write();
	} else {
		asio::post(io_, [this]() {
			do_readStdIn();
		});
	}
}

void clientNet::do_write() {
	serializedMessage<messageTypeMap> message = messagesOut.pop();
	asio::async_write(socket_, message.toBuffers(),
			[this](const boostError &er, size_t len) {
				if (!er) {
					//logAction("wrote ", len, " bytes to server");
					do_readStdIn();
				} else {
					throw prependError("client::do_write()", er);
					//logError("client::do_write(): ", er.what());
					//close();
				}
			});
}

void clientNet::do_read_header() {
	asio::async_read(socket_, inputMessage.headerBuffer(),
			[this](const boostError &er, size_t len) {
				if (!er) {
					do_read_body();
				} else {
					throw prependError("session::do_read_header()", er);
				}
			});
}

void clientNet::do_read_body() {
	inputMessage.reserveSpaceForData();
	asio::async_read(socket_, inputMessage.bodyBuffer(),
			[this](const boostError &er, size_t len) {
				if (!er) {
					processInputMessage();
				} else {
					throw prependError("session::do_read_body()", er);
				}
			});

}

void clientNet::processInputMessage() {
	switch (inputMessage.header.type_id) {
	case getTypeId<setReciverId> :
		processSetReciverId();
		break;
	case getTypeId<textMessage> :
		processTextMessage();
		break;
	case getTypeId<clientList> :
		processClientList();
		break;
	default:
		throw std::runtime_error(
				"unknown type id: "
						+ std::to_string(inputMessage.header.type_id)
						+ ", socket id: " + std::to_string(id));
		break;
	}

}

void clientNet::processSetReciverId() {
	setReciverId msg = inputMessage.getObject<setReciverId>();
	id = msg.id;
	//logAction("id set to ", id);
	textMessage reportMsg("set self network Id to " + std::to_string(msg.id));
	serializedMessage<messageTypeMap> ser(reportMsg,
			inputMessage.header.metaData);
	messagesIn.push(ser);
	do_read();
}

void clientNet::processTextMessage() {
	messagesIn.push(inputMessage);
	do_read();
}

void clientNet::processClientList() {
	clientList list = inputMessage.getObject<clientList>();
	for (auto &it : list.users) {
		clientNames[it.first] = it.second;
	}
	do_read();
}
