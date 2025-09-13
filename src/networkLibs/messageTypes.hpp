#pragma once
#include "serializeMessages.hpp"

#define SERIALIZE(args) \
template<typename Archive>\
void serialize(Archive &archive){\
	archive(args);\
}

enum dedicatedId : networkId_t {
	masterServer = 0, baseOffset = 1000,
};

struct setReciverId {
	networkId_t id = -1;
	setReciverId() {

	}
	setReciverId(networkId_t id_) :
			id(id_) {
	}

	SERIALIZE(id)
};

struct setSenderName {
	std::string userName;
	setSenderName() {

	}
	setSenderName(const std::string &str) :
			userName(str) {
	}

	SERIALIZE(userName)
};

struct textMessage {
	std::string str;
	textMessage() {

	}
	textMessage(const std::string &str_) :
			str(str_) {
	}

	SERIALIZE(str)
};

struct clientList {
	using userVec = std::vector<std::pair<networkId_t,std::string>>;
	userVec users;

	clientList() {

	}

	clientList(const userVec &other) {
		users = other;
	}

	SERIALIZE(users)

};

struct requestClientList {
	requestClientList() {
	}

	template<typename Archive>
	void serialize(Archive &archive) {
	}
};

using messageTypeMap = TypeIdMap<setReciverId,setSenderName,textMessage,clientList,requestClientList>;

template<typename T>
constexpr int getTypeId = messageTypeMap::template toId<T>;

