#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include "typeToId.hpp"

//Serialization
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>

#include <cereal/archives/binary.hpp>

template<typename T, typename V>
struct type_pair_tag {
};

using networkId_t = unsigned int;
namespace asio = boost::asio;

struct messageMetaData {
	networkId_t from;
	networkId_t to;

	template<typename Archive>
	void serialize(Archive &archive) const {
		archive(from, to);
	}
};

struct messageHeader {
	messageMetaData metaData;
	std::size_t length = 0;
	int type_id = -1;
	static const std::size_t headerSize;

	messageHeader() {
	}

	template<typename T, typename typeIdMap>
	messageHeader(std::size_t len, const messageMetaData &meta,
			type_pair_tag<T, typeIdMap>) :
			metaData(meta), length(len), type_id(typeIdMap::template toId<T>) {
	}

	template<typename Archive>
	void serialize(Archive &archive) const {
		archive(metaData, length, type_id);
	}
};

const std::size_t messageHeader::headerSize = sizeof(messageHeader);

using binaryData = std::string;

template<typename typeIdMap>
struct serializedMessage {

	messageHeader header;
	binaryData data;

	serializedMessage() {

	}

	template<typename T>
	serializedMessage(const T &v, const messageMetaData &meta) {
		static_assert(typeIdMap::template hasType<T>,"type T not in typeIdMap");
		std::stringstream dataStream;
		{
			cereal::BinaryOutputArchive outArchive(dataStream);
			outArchive(v);
		}
		data = dataStream.str();
		header = messageHeader(data.size(), meta,
				type_pair_tag<T, typeIdMap> { });
	}

	messageMetaData getReturnMetaData() const {
		auto ret = header.metaData;
		std::swap(ret.from, ret.to);
		return ret;
	}

	asio::mutable_buffer headerBuffer() {
		return asio::buffer(&header, messageHeader::headerSize);
	}

	asio::mutable_buffer bodyBuffer() {
		return asio::buffer(data.data(), data.size());
	}

	std::array<asio::mutable_buffer, 2> toBuffers() {
		return {headerBuffer(),bodyBuffer()};
	}

	std::size_t dataSize() {
		return header.length;
	}

	void reserveSpaceForData() {
		data = std::string(header.length, 0);
	}

	template<typename T>
	T getObject() {
		static_assert(typeIdMap::template hasType<T>,"type T not in typeIdMap");
		if (header.type_id != typeIdMap::template toId<T>) {
			throw std::runtime_error(
					"message::getObject(): typeId in header("
							+ std::to_string(header.type_id) + ") != T typId("
							+ std::to_string(typeIdMap::template toId<T>)
							+ ")");
		}

		T ret;
		std::stringstream ss(data);
		{
			cereal::BinaryInputArchive inArchive(ss);
			inArchive(ret);
		}
		return ret;
	}
};

