/*
 * libasynckafka - Apache Kafka C++ Async Client library
 *
 * Copyright (c) 2016, Ganesh Nikam, Great Software Laboratory Pvt Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <event2/event.h>

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>

#include "KafkaClient.h"

AsyncKakfa::KafkaProducer *pProducer = NULL;

const char* short_opts = "S:P:t:p:b:a:h";
const struct option long_opts[] = {
    { "server",     1, NULL, 'S' },
    { "port",       1, NULL, 'P' },
    { "topic",      1, NULL, 't' },
    { "partition",  1, NULL, 'p' },
    { "batch",      1, NULL, 'b' },
    { "acks",       1, NULL, 'a' },
    { "help",       1, NULL, 'h' },
    { NULL,         0, NULL, 0 }
};

static void usage(const char *short_opts) {
    printf("Usage: \n");

    if (strchr(short_opts, 'S')) {
        printf("-S <server ip>\n");
    }

    if (strchr(short_opts, 'P')) {
        printf("-P <port number>\n");
    }

    if (strchr(short_opts, 't')) {
        printf("-t <topic>\n");
    }

    if (strchr(short_opts, 'p')) {
        printf("-p <partition ID (-1 for all partitions)>\n");
    }

    if (strchr(short_opts, 'b')) {
        printf("-b <batch length (# of messages)>\n");
    }

    if (strchr(short_opts, 'a')) {
        printf("-a <# of required acks(1 = leader only, -1 = All ISR, N = N # of ISRs)>\n");
    }

    if (strchr(short_opts, 'h')) {
        printf("-h help\n");
    }
}

static int onProduceResponse(std::string topic, int32_t partition, int64_t offset)
{
    printf("Messages produced on server. The last offset is: %ld\n", offset);
    return 0;
}


int64_t numMesgs = 0;
bool run = true;
static int onMsgQueued() {

    std::stringstream ss;
    ++numMesgs;
    ss << "The mssage number is : " << numMesgs;

    pProducer->produce(ss.str());
    return 0;
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = static_cast<struct event_base *>(user_data);
    struct timeval delay = { 0, 0 };

    printf("Caught an interrupt signal; exiting cleanly in one second.\n");
    event_base_loopexit(base, &delay);
}

int main(int argc, char* argv[]) {
    int c;
    int i;

    std::string serverIp;
    int16_t port;
    std::string topicStr;
    int16_t partitionId = -1;
    AsyncKakfa::KafkaClientConfig kConfig;

    if (argc <= 1) {
        usage(short_opts);
        return -1;
    }

    while((c = getopt_long(argc, argv, short_opts, long_opts, &i)) != -1) {
        switch(c) {
            case 'S':
                serverIp = optarg;
                break;
            case 'P':
                port = atoi(optarg);
                break;
            case 't':
                topicStr = optarg;
                break;
            case 'p':
                partitionId = atoi(optarg);
                break;
            case 'b':
                kConfig.pBatchNumMsgs = atoi(optarg);
                break;
            case 'a':
                kConfig.pRequiredAcks = atoi(optarg);
                break;
            case 'h':
            default:
                usage(short_opts);
                return (1);
        }
    }

    struct event_base *base = NULL;
    struct event *signal_event;
    base =  event_base_new();
    if (!base) {
        printf("Memory allocation failed for event_base\n");
        return 1;
    }

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

    if (!signal_event || event_add(signal_event, NULL)<0) {
         printf("Could not create/add a signal event!\n");
        return 1;
    }

    pProducer = new AsyncKakfa::KafkaProducer(base,
                             topicStr,
                             partitionId,
                             serverIp,
                             port,
                             kConfig
                            );

    pProducer->onProduceResponse = &onProduceResponse;
    pProducer->onMsgQueued = &onMsgQueued;

    pProducer->startProduce();

    event_base_dispatch(base);

    printf("Message Produced = %ld\n\n", numMesgs);

    if (pProducer) {
        free(pProducer);
    }

    return 0;

}
