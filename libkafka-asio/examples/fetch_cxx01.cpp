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


using libkafka_asio::Client;
using libkafka_asio::FetchRequest;
using libkafka_asio::FetchResponse;
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
    //std::cout << msg << std::endl;

    std::filebuf fb;
    fb.open ("test-1.txt", std::ios::out);
    std::ostream os(&fb);
    os << msg;

    fb.close();
    delete [] data;

    fb.open ("test-1.txt", std::ios::in);
    std::istream is(&fb);

    FetchRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);

    if (ec) {
        printf("ec = %d\n", ec);
    }
    fb.close();
    const FetchResponse::OptionalType& rsp = response.response();
    std::for_each(rsp->begin(), rsp->end(), &PrintMessage);
    printf("The next offset to read is: %ld\n", next_fetch_offset);
    fetching = false;
    system("rm -f test-1.txt");
    return 0;
}


int handle_write()
{

    if (!fetching) {
        std::filebuf fb;
        fb.open ("test.txt", std::ios::out);

        FetchRequest request;
        request.FetchTopic("TEMNOS", 0, next_fetch_offset);

        std::ostream os(&fb);
        libkafka_asio::detail::WriteRequest(request, "test-client", os);

        fb.close();

        fb.open("test.txt", std::ios::in | std::ios::binary);
        int size = fb.in_avail();
        std::cout << "size: " << size << std::endl;
        char *data = new char [size];
        fb.sgetn(data, size);


        printf("The generated request msg is: \n");
        std::cout << data << std::endl;

        int res = write(sockFd, data, size);
        printf(" size = %d, res = %d\n", size, res);
        fb.close();
        fetching = true;
        system("rm -f test.txt");
        delete [] data;
    }

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
