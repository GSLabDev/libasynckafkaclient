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

#ifndef AsyncKakfa_KafkaClient_INCLUDED
#define AsyncKakfa_KafkaClient_INCLUDED

#include <unistd.h>

#include <vector>
#include <functional>

#include "KafkaEndpointMP.h"
#include "LibeventReactor.h"

namespace AsyncKakfa
{

class KafkaClient {
public:
    KafkaClient(KafkaPartitionType partitionType,
                  struct event_base* base,
                  const std::string topic,
                  const int32_t partition, // If -1, consume from all partitions OR consume only from specified partition (0....N)
                  const std::string broker,
                  const int16_t service,
                  KafkaClientConfig & config);

    virtual ~KafkaClient();
    void init();
    int handleFetchedData(std::vector<std::string> msgs);
    int handleProduceResponse(std::string topic, int32_t partition, int64_t offset);
    int handleMsgQueuedResponse();
    typedef std::function<int (std::vector<std::string> && msgs)> onFetchedDataCb;
    typedef std::function<int (int32_t errCode, std::string errStr)> onErrorCb;
    typedef std::function<int (std::string topic, int32_t partition, int64_t offset)> onProduceResponseCb;
    typedef std::function<int ()> onMsgQueuedCb;
    onFetchedDataCb onFetchedData;
    onErrorCb onError;
    onProduceResponseCb onProduceResponse;
    onMsgQueuedCb onMsgQueued;

protected:
    int32_t                 m_numPartitions;
    int32_t                 m_iPartition;
    std::string             m_strBroker;
    int16_t                 m_iService;
    std::string             m_strTopic;
    LibeventReactor       m_reactor;
    KafkaClientConfig &   m_config;
    KafkaPartitionType    m_partitionType;
    std::vector<std::shared_ptr<KafkaPartition>> m_partitions;
    std::vector<KafkaBroker> m_brokers;
};

class KafkaConsumer : public KafkaClient
{
public:
    KafkaConsumer(struct event_base* base,
                    const std::string topic,
                    const int32_t partition, // If -1, consume from all partitions OR consume only from specified partition (0....N)
                    const std::string broker,
                    const int16_t service,
                    KafkaClientConfig & config);

    virtual ~KafkaConsumer();
    int startFetch();
    void pauseFetch();
    void resumeFetch();
};

class KafkaProducer : public KafkaClient
{
public:
    KafkaProducer(struct event_base* base,
                    const std::string topic,
                    const int32_t partition, // If -1, consume from all partitions OR consume only from specified partition (0....N)
                    const std::string broker,
                    const int16_t service,
                    KafkaClientConfig & config);

    virtual ~KafkaProducer();
    int startProduce();
    int produce(std::string msg);
    int produce(std::vector<std::string> msgs);
};

} //namespace AsyncKakfa

#endif // AsyncKakfa_KafkaClient_INCLUDED
