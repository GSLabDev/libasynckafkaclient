//
// examples/fetch_cxx03.cpp
// ------------------------
//
// Copyright (c) 2015 Daniel Joos
//
// Distributed under MIT license. (See file LICENSE)
//
// ----------------------------------
//
// This example shows how to create a 'FetchRequest' to get messages for a
// specific Topic & partition. On success, all received messages will be print
// to stdout.
//

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <libkafka_asio/libkafka_asio.h>

#include <sstream>
#include <streambuf>
#include <fstream>

//#include <libkafka_asio/constants.h>
//#include <libkafka_asio/detail/request_write.h>
//#include <libkafka_asio/detail/response_read.h>
//#include <libkafka_asio/error.h>


//using libkafka_asio::Client;
using libkafka_asio::MetadataRequest;
using libkafka_asio::MetadataResponse;

int sockFd = -1;

int main(int argc, char **argv)
{

    struct sockaddr_in serv_addr;

    sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd < 0) {
        printf("socket creation failed\n");
        exit(1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9092);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("connect failed\n");
        exit(1);
    }

    MetadataRequest request;
    request.set_correlation_id(1234);
    request.AddTopicName("TEMNOS");

    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, "test-client", os);


    //printf("The generated request msg is: \n");
    //std::cout << os.str() << std::endl;

    int res = write(sockFd, os.str().c_str(), os.str().length());
    printf(" size = %ld, res = %d\n", os.str().length(), res);


    /* Handle Response */
    uint32_t size;

    res = read(sockFd, (char*)&size, sizeof(uint32_t));
    size = ntohl(size);
    printf("The received msg size = %d\n", size);
    char *data = new char [size];

    res = read(sockFd, data, size);
    printf("size = %d res = %d\n", size, res);

    std::string msg(data, size);
    std::stringstream  is(msg, std::ios_base::in);
    //std::cout << msg << std::endl;

    delete [] data;

    MetadataRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);

    if (ec) {
        printf("ec = %ld\n", ec);
    }

    const MetadataResponse::OptionalType& rsp = response.response();
    // Find the leader for topic 'mytopic' and partition 1
    MetadataResponse::Broker::OptionalType leader =
    rsp->PartitionLeader("TEMNOS", 0);

    if (!leader) {
        std::cerr << "No leader found!" << std::endl;
        return -1;
    }

    std::cout << "response correlation id: " << rsp->correlation_id() << std::endl;
    std::cout << "The leader details are: " << std::endl;
    std::cout << "Node id: " << leader->node_id << std::endl;
    std::cout << "Host: " << leader->host << std::endl;
    std::cout << "Port: " << leader->port << std::endl;

  return 0;
}
