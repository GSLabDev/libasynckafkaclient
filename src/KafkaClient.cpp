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

#include <sys/stat.h>

#include <string>
#include <iostream>
#include <libkafka_asio/libkafka_asio.h>
#include <libkafka_asio/constants.h>

#include <memory>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>

#include "KafkaClient.h"

using libkafka_asio::MetadataRequest;
using libkafka_asio::MetadataResponse;

namespace AsyncKakfa
{

KafkaClient::KafkaClient(KafkaPartitionType partitionType,
                    struct event_base* base,
                    const std::string topic,
                    const int32_t partition, // If -1, consume from all partitions OR consume only from specified partition (0....N)
                    const std::string broker,
                    const int16_t service,
                    KafkaClientConfig & config):
                m_partitionType(partitionType),
                m_strTopic(topic),
                m_iPartition(partition),
                m_strBroker(broker),
                m_iService(service),
                m_config(config)
{
    m_reactor.init(base);
    init();
}

KafkaClient::~KafkaClient()
{
    // decide what to clean here
}

void
KafkaClient::init()
{
    int32_t sockFd = AsyncKakfa::utils::connectServer(m_strBroker, m_iService);
    if (sockFd < 0) {
        ERROR("KafkaClient: connection to Kafka Server failed. Exiting");
        exit(0);
    }

    // NOTE : When KafkaProducer() object is created with new topic (which is not present on)
    //        Kafka Server, then the 1st metadata request (sent below) creates that topic on
    //        Kafka Server (if auto.create.topics.enable = true in server config). But it takes
    //        time to create topic and all the partitions. In this period, the metadata response
    //        only contains brokers details. Partitions details are blank. In the below logic,
    //        we send metadata request multiple times, till we get valid partitions details
sendMetaDataRequest:

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    MetadataRequest request;
    request.set_correlation_id(libkafka_asio::constants::kApiKeyMetadataRequest);
    request.AddTopicName(m_strTopic);
    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);
    const std::string osStr = os.str();
    int res = write(sockFd, osStr.c_str(), osStr.length());
    if (res == -1) {
        ERROR("KafkaClient write() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);

    /* Handle the metadata response */
    uint32_t size;
    res = AsyncKakfa::utils::readData(sockFd, (char*)&size, sizeof(uint32_t));
    if (res == -1) {
        ERROR("KafkaClient read() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);

    if (res == 0) { // The other end (Kafka server closed the connection)
        ERROR("%s: KafkaClient Kafka Server closed the connection. ", m_config.clientId.c_str());
        exit(0);
    }

    size = ntohl(size);
    char *data = new char [size];

    res = AsyncKakfa::utils::readData(sockFd, data, size);
    if (res == -1) {
        delete [] data;
        ERROR("KafkaClient read() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);

    if (res == 0) { // The other end (Kafka server closed the connection)
        ERROR("%s: KafkaClient Kafka Server closed the connection. ", m_config.clientId.c_str());
        delete [] data;
        exit(0);
    }

    std::string msg(data, size);
    std::stringstream  is(msg, std::ios_base::in);
    delete [] data;

    MetadataRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);

    if (ec) {
        ERROR("The meta data response error code is : %d", ec.value());
        exit(0);
    }

    const MetadataResponse::OptionalType& rsp = response.response();
    MetadataResponse::BrokerVector brokers = rsp->brokers();

    MetadataResponse::BrokerVector::const_iterator broker_iter;
    DEBUG("Kafka Brokers :");
    for (broker_iter = brokers.begin(); broker_iter != brokers.end(); ++broker_iter) {
        MetadataResponse::Broker::OptionalType  broker = *broker_iter;
        char ip[100] = {0};
        KafkaBroker kBroker;
        kBroker.m_iId = broker->node_id;
        kBroker.m_strHost  = broker->host;
        AsyncKakfa::utils::hostnameToIp(broker->host.c_str(), ip);
        kBroker.m_strIp = ip;
        kBroker.m_iPort = broker->port;

        DEBUG("\t ID   : %d", kBroker.m_iId);
        DEBUG("\t Host : %s", kBroker.m_strHost.c_str());
        DEBUG("\t IP   : %s", kBroker.m_strIp.c_str());
        DEBUG("\t Port : %d", kBroker.m_iPort);
        m_brokers.push_back(kBroker);
    }

    if (m_iPartition == KAFKA_ALL_PARTITIONS) {
        MetadataResponse::TopicVector::const_iterator topic_iter = libkafka_asio::detail::FindTopicByName(m_strTopic, rsp->topics());
        if (topic_iter == rsp->topics().end()) {
            ERROR("%s topic not found in metadata response", m_strTopic.c_str());
            exit(0);
        }

        MetadataResponse::Topic::PartitionVector partitions = topic_iter->partitions;
        if (partitions.empty()) {
            ERROR("No partitions found for topic %s", m_strTopic.c_str());
            goto sendMetaDataRequest;
        }

        MetadataResponse::Topic::PartitionVector::const_iterator partition_iter;
        DEBUG("Partitions of the Topic: %s", m_strTopic.c_str());
        for (partition_iter = partitions.begin(); partition_iter != partitions.end(); ++partition_iter) {
            ++m_numPartitions;
            if (partition_iter->error_code) {
                ERROR("Metadata response partition error code : %d", partition_iter->error_code);
                exit(0);
            }

            int32_t partitionID = partition_iter->partition;
            int32_t leaderNodeID = partition_iter->leader;

            std::vector<KafkaBroker>::iterator it;
            it = std::find_if(m_brokers.begin(), m_brokers.end(), [&](KafkaBroker broker){ return (broker.m_iId == leaderNodeID);});
            if (it == m_brokers.end()) {
                ERROR("Partition %d leader node (%d) is not present in the broker list", partitionID, leaderNodeID);
                exit(0);
            }

            KafkaBroker & kLeader = *it;
            DEBUG("\t Partition ID   : %d", partitionID);
            DEBUG("\t Partition Leader");

            DEBUG("\t\t ID   : %d", kLeader.m_iId);
            DEBUG("\t\t Host : %s", kLeader.m_strHost.c_str());
            DEBUG("\t\t IP   : %s", kLeader.m_strIp.c_str());
            DEBUG("\t\t Port : %d", kLeader.m_iPort);

            std::shared_ptr<KafkaPartition> partition( new KafkaPartition(m_partitionType,
                                                                              m_reactor,
                                                                              m_strTopic,
                                                                              partitionID,
                                                                              m_brokers,
                                                                              kLeader,
                                                                              m_config
                                                                             ));

            partition->onReceivedData = std::bind(&KafkaClient::handleFetchedData,
                                                  this,
                                                  std::placeholders::_1);

            partition->onProduceResponse = std::bind(&KafkaClient::handleProduceResponse,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);

            partition->onMsgQueued = std::bind(&KafkaClient::handleMsgQueuedResponse,
                                               this);

            m_partitions.push_back(partition);
        }

    } else {
        MetadataResponse::Broker::OptionalType leader = rsp->PartitionLeader(m_strTopic, m_iPartition);
        if (leader) {
            int32_t leaderNodeID = leader->node_id;
            std::vector<KafkaBroker>::iterator it;
            it = std::find_if(m_brokers.begin(), m_brokers.end(), [&](KafkaBroker broker){ return (broker.m_iId == leaderNodeID);});
            if (it == m_brokers.end()) {
                ERROR("Partition %d leader node (%d) is not present in the broker list", m_iPartition, leaderNodeID);
                exit(0);
            }

            KafkaBroker & kLeader = *it;
            DEBUG("\t Partition ID   : %d", m_iPartition);
            DEBUG("\t Partition Leader");

            DEBUG("\t\t ID   : %d", kLeader.m_iId);
            DEBUG("\t\t Host : %s", kLeader.m_strHost.c_str());
            DEBUG("\t\t IP   : %s", kLeader.m_strIp.c_str());
            DEBUG("\t\t Port : %d", kLeader.m_iPort);

            std::shared_ptr<KafkaPartition> partition( new KafkaPartition(m_partitionType,
                                                                              m_reactor,
                                                                              m_strTopic,
                                                                              m_iPartition,
                                                                              m_brokers,
                                                                              kLeader,
                                                                              m_config
                                                                             ));

            partition->onReceivedData = std::bind(&KafkaConsumer::handleFetchedData,
                                                  this,
                                                  std::placeholders::_1);

            partition->onProduceResponse = std::bind(&KafkaConsumer::handleProduceResponse,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3);

            partition->onMsgQueued = std::bind(&KafkaClient::handleMsgQueuedResponse,
                                               this);

            m_partitions.push_back(partition);

        } else {
            ERROR("No leader found for topic + partition (%s + %d) combination\n", m_strTopic.c_str(), m_iPartition);
            goto sendMetaDataRequest;
        }
    }

    close(sockFd);
    sockFd = -1;
}

int KafkaClient::handleFetchedData(std::vector<std::string> msgs)
{
    if (onFetchedData) {
        onFetchedData(std::move(msgs));
    }

    return 0;
}

int KafkaClient::handleProduceResponse(std::string topic, int32_t partition, int64_t offset)
{
    if(onProduceResponse) {
        onProduceResponse(topic, partition, offset);
    }

    return 0;
}

int KafkaClient::handleMsgQueuedResponse()
{
    if (onMsgQueued) {
        onMsgQueued();
    }

    return 0;
}

KafkaConsumer::KafkaConsumer(struct event_base* base,
                    const std::string topic,
                    const int32_t partition,
                    const std::string broker,
                    const int16_t service,
                    KafkaClientConfig & config):
                KafkaClient(KafkaPartitionType::CONSUMER_P,
                              base,
                              topic,
                              partition,
                              broker,
                              service,
                              config)
{
}

KafkaConsumer::~KafkaConsumer()
{
    // decide what to clean here
}

int KafkaConsumer::startFetch()
{
    std::vector<std::shared_ptr<KafkaPartition>>::iterator iter;
    for (iter = m_partitions.begin(); iter != m_partitions.end(); ++iter) {
        std::shared_ptr<KafkaPartition> & partition = *iter;
        partition->startFetch();
    }

    return 0;
}

void KafkaConsumer::pauseFetch()
{
    std::vector<std::shared_ptr<KafkaPartition>>::iterator iter;
    for (iter = m_partitions.begin(); iter != m_partitions.end(); ++iter) {
        std::shared_ptr<KafkaPartition> & partition = *iter;
        partition->pauseFetch();
    }

    return;
}

void KafkaConsumer::resumeFetch()
{
    std::vector<std::shared_ptr<KafkaPartition>>::iterator iter;
    for (iter = m_partitions.begin(); iter != m_partitions.end(); ++iter) {
        std::shared_ptr<KafkaPartition> & partition = *iter;
        partition->resumeFetch();
    }

    return;
}

KafkaProducer::KafkaProducer(struct event_base* base,
                    const std::string topic,
                    const int32_t partition,
                    const std::string broker,
                    const int16_t service,
                    KafkaClientConfig & config):
                KafkaClient(KafkaPartitionType::PRODUCER_P,
                              base,
                              topic,
                              partition,
                              broker,
                              service,
                              config)
{
    startProduce();
}

KafkaProducer::~KafkaProducer()
{
    // decide what to clean here
}

int KafkaProducer::startProduce()
{
    std::vector<std::shared_ptr<KafkaPartition>>::iterator iter;
    for (iter = m_partitions.begin(); iter != m_partitions.end(); ++iter) {
        std::shared_ptr<KafkaPartition> & partition = *iter;
        partition->startProduce();
    }

    return 0;
}

int32_t currPartition = 0;
int KafkaProducer::produce(std::string msg)
{
    if (m_iPartition != KAFKA_ALL_PARTITIONS) {
        std::shared_ptr<KafkaPartition> & partition = m_partitions[0];
        partition->produce(msg);
    } else {

        if (m_config.pPartitioningMethod == ROUND_ROBIN) {
            std::shared_ptr<KafkaPartition> & partition = m_partitions[currPartition];
            partition->produce(msg);

            currPartition = currPartition++;
            currPartition = currPartition % m_partitions.size();
        } else {
            //TODO: Partition callback implementation to be added
            INFO("Partition callback");
        }
    }

    return 0;
}

int KafkaProducer::produce(std::vector<std::string> msgs)
{
    if (m_iPartition != KAFKA_ALL_PARTITIONS) {
        std::shared_ptr<KafkaPartition> & partition = m_partitions[0];
        partition->produce(msgs);
    } else {

        if (m_config.pPartitioningMethod == ROUND_ROBIN) {
            std::shared_ptr<KafkaPartition> & partition = m_partitions[currPartition];
            partition->produce(msgs);

            currPartition = currPartition++;
            currPartition = currPartition % m_partitions.size();
        } else {
            //TODO: Partition callback implementation to be added
            INFO("Partition callback");
        }
    }

    return 0;
}

} // namespace AsyncKakfa
