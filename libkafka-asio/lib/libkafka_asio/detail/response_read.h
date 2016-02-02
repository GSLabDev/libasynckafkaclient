//
// detail/response_read.h
// ----------------------
//
// Copyright (c) 2015 Daniel Joos
//
// Distributed under MIT license. (See file LICENSE)
//

#ifndef RESPONSE_READ_H_TODO
#define RESPONSE_READ_H_TODO

#include <iostream>
#include <boost/system/error_code.hpp>
#include <libkafka_asio/primitives.h>
#include <libkafka_asio/response.h>
#include <libkafka_asio/message.h>

namespace libkafka_asio
{

//
// Forward declarations
//

class MutableMetadataResponse;

class MutableProduceResponse;

class MutableFetchResponse;

class MutableOffsetResponse;

class MutableConsumerMetadataResponse;

class MutableOffsetCommitResponse;

class MutableOffsetFetchResponse;

namespace detail
{

Int8 ReadInt8(std::stringstream& is);

Int16 ReadInt16(std::stringstream& is);

Int32 ReadInt32(std::stringstream& is);

Int64 ReadInt64(std::stringstream& is);

String ReadString(std::stringstream& is);

void ReadBytes(std::stringstream& is, Bytes& bytes);

void ReadMessage(std::stringstream& is, Message& message);

void ReadMessageSet(std::stringstream& is, MessageSet& message_set, size_t size);

template<typename TMutableResponse>
void ReadResponse(std::stringstream& is,
                  TMutableResponse& response,
                  boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableMetadataResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableProduceResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableFetchResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableOffsetResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableConsumerMetadataResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableOffsetCommitResponse& response,
                         boost::system::error_code& ec);

void ReadResponseMessage(std::stringstream& is,
                         MutableOffsetFetchResponse& response,
                         boost::system::error_code& ec);

}  // namespace detail
}  // namespace libkafka_asio

#include <libkafka_asio/detail/impl/response_read.h>

#endif  // RESPONSE_READ_H_TODO
