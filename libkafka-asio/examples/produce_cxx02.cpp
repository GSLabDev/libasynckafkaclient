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
using libkafka_asio::ProduceRequest;
using libkafka_asio::ProduceResponse;
using libkafka_asio::MessageAndOffset;

int sockFd = -1;
int64_t next_fetch_offset = 1;
bool fetching = false;

std::string BytesToString(const libkafka_asio::Bytes& bytes)
{
  if (!bytes || bytes->empty())
  {
    return "";
  }
  return std::string((const char*) &(*bytes)[0], bytes->size());
};

void PrintMessage(const MessageAndOffset& message)
{
    next_fetch_offset = message.offset() + 1;
  std::cout << BytesToString(message.value()) << std::endl;
}

int handle_read()
{
    uint32_t size;
    int res;

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

    ProduceRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);

    if (ec) {
        printf("ec = %ld\n", ec);
        printf("There is error in produce request. The error code is: %ld\n", ec);
    } else {
        printf("message produced on broker successfully\n");
    }

    return 0;
}


int handle_write()
{

    ProduceRequest request;
    request.AddValue("***************************", "TEMNOS", 0);
    for (int i = 0; i < 100; ++i) {

        request.AddValue("Hello World", "TEMNOS", 0);
    }

    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, "test-client", os);


    printf("The generated request msg is: \n");
    //std::cout << os.str() << std::endl;

    int res = write(sockFd, os.str().c_str(), os.str().length());
    printf(" size = %ld, res = %d\n", os.str().length(), res);

    return 0;

}

int main(int argc, char **argv)
{

    struct sockaddr_in serv_addr;

    int epollFd = -1;
    struct epoll_event ev, events[20];

    epollFd = epoll_create(10);
    if (epollFd == -1) {
        printf("epoll_create() failed\n");
        exit(1);
    }

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

    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sockFd;

    int res = epoll_ctl(epollFd, EPOLL_CTL_ADD, sockFd, &ev);
    if (res == -1) {
        printf("epoll_ctl failed \n");
        exit(1);
    }

    int nfds = 0;
    for (;;) {
        nfds = epoll_wait(epollFd, events, 20, -1);
        if (nfds == -1) {
            printf("epoll_wait failed\n");
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sockFd) {
                if (events[i].events & EPOLLOUT) {
                    handle_write();
                    sleep(2);
                }

                if (events[i].events & EPOLLIN) {
                    handle_read();
                }
            }
        }
    }
  // Create a 'Fetch' request and try to get data for partition 0 of topic
  // 'mytopic', starting with offset 1

  // Send the prepared fetch request.
  // The client will attempt to automatically connect to the broker, specified
  // in the configuration.

  // Let's go!
  return 0;
}
