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

#include "KafkaEndpointMP.h"

using libkafka_asio::ProduceRequest;
using libkafka_asio::ProduceResponse;
using libkafka_asio::FetchRequest;
using libkafka_asio::FetchResponse;
using libkafka_asio::MessageAndOffset;
using libkafka_asio::MetadataRequest;
using libkafka_asio::MetadataResponse;
using libkafka_asio::OffsetRequest;
using libkafka_asio::OffsetResponse;

namespace AsyncKakfa
{

KafkaPartition::KafkaPartition
                            (KafkaPartitionType partitionType,
                             LibeventReactor & reactor,
                             const std::string topic,
                             const int32_t partition,
                             const std::vector<KafkaBroker> brokers,
                             const KafkaBroker leader,
                             KafkaClientConfig & config)
            : m_iNextFetchOffset(0),
              m_bFetchPaused(false),
              m_iMsgCnt(0),
              m_iSockFd(-1),
              m_iEventFlags(AEVENTIN | AEVENTOUT),
              m_partitionType(partitionType),
              m_reactor(reactor),
              m_strTopic(topic),
              m_iPartition(partition),
              m_sBrokers(brokers),
              m_sLeader(leader),
              m_config(config)
{
    if (m_partitionType == CONSUMER_P) {
        getKafkaOffset();
    }

    init();
}

KafkaPartition::~KafkaPartition()
{
    // clean the internal produce queue
    createProduceRequest(0);

    // Send all the pending request to Kafka server before shutdown
    while (!m_dqWriteBuffers.empty()) {
        onWriteReady();
    }

    // Let socket buffer be cleared
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    close(m_iSockFd);
}

void
KafkaPartition::getKafkaOffset() {

    std::stringstream ss;
    ss << m_config.clientId << "-" << m_strTopic << "-" << m_iPartition << "-" << "start-offset";
    m_strStoredOffsetFileName = ss.str();

    std::string storePath = m_config.cOffsetStorePath;
    if (!storePath.empty()) {
        struct stat st;
        if (stat(storePath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            if((*storePath.rbegin()) == '/') {
                m_strStoredOffsetFileName = storePath + m_strStoredOffsetFileName;
            } else {
                m_strStoredOffsetFileName = storePath + "/" + m_strStoredOffsetFileName;
            }
        } else {
            WARN("The cOffsetStorePath : %s does not exist. Creating the OffsetFile in current directory.", storePath.c_str());
        }
    } else {
        WARN("The cOffsetStorePath is empty. Creating the OffsetFile in current directory.");
    }

    std::fstream file(m_strStoredOffsetFileName);
    if(!file.good()) {
        std::ofstream myfile1;
        myfile1.open (m_strStoredOffsetFileName);
        myfile1 << m_iNextFetchOffset;
        myfile1.close();
    } else {
        if (m_config.cStartOffset == KafkaFetchOffest::FROM_STORED) {
            std::string content;
            std::ifstream file1(m_strStoredOffsetFileName);
            while(file1 >> content) {
                INFO("m_iNextFetchOffset = %s", content.c_str());
            }
            std::string::size_type sz;
            m_iNextFetchOffset = std::stoll (content, &sz);
        }
    }
}

void
KafkaPartition::init()
{

    if (!connectLeader()) {
        ERROR("connection to kafka server failed");
        exit(0);
    }

    DEBUG("Kafka socket fd %d",selectFd());
    /* send the metadata request to get the leader for the topic + partition combination */
    /* if the leader and the input broker didn't match then disconnect from the current  */
    /* broker and connect to the new leader.                                             */
    createMetadataRequest(0);
    onWriteReady();
    onReadReady();

    createOffsetRequest(0);
    onWriteReady();
    onReadReady();

    m_mdTimerEvent.init((m_config.mRefreshIntervalMs/1000),
                        std::bind(&KafkaPartition::createMetadataRequest, this, std::placeholders::_1));

    m_reactor.registerFdCallback(m_mdTimerEvent.selectFd(),
                                 m_mdTimerEvent.eventData(),
                                 m_mdTimerEvent.getEventFlags(),
                                 false,
                                 false);

    return;
}

bool KafkaPartition::connectLeader()
{
    int32_t sockFd = AsyncKakfa::utils::connectServer(m_sLeader.m_strIp, m_sLeader.m_iPort);
    if (sockFd < 0) {
        return false;
    }

    m_iSockFd = sockFd;
    m_eventData.fd = m_iSockFd;
    m_eventData.cb = std::bind(&KafkaPartition::handleEvent,
                               this,
                               std::placeholders::_1,
                               std::placeholders::_2,
                               std::placeholders::_3);
    m_eventData.edata = NULL;

    /* register the KafkaPartition socket fd with Reactor */
    m_reactor.registerFdCallback(selectFd(),
                                 eventData(),
                                 getEventFlags(),
                                 false,
                                 false);

    return true;
}

int KafkaPartition::startFetch()
{
    INFO("startFetch");

    m_fetchEvent.init(std::bind(&KafkaPartition::createFetchRequest, this, std::placeholders::_1));
    m_reactor.registerFdCallback(m_fetchEvent.selectFd(),
                                m_fetchEvent.eventData(),
                                m_fetchEvent.getEventFlags(),
                                false,
                                false);

    m_ocTimerEvent.init((m_config.cAutoCommitIntervalMs/1000),
                        std::bind(&KafkaPartition::commitKafkaOffset, this, std::placeholders::_1));

    m_reactor.registerFdCallback(m_ocTimerEvent.selectFd(),
                                 m_ocTimerEvent.eventData(),
                                 m_ocTimerEvent.getEventFlags(),
                                 false,
                                 false);

    /* start fetching */
    if (m_partitionType == CONSUMER_P) {
        m_fetchEvent.wakeup();
    }

    return 0;
}

int KafkaPartition::startProduce()
{
    INFO("startProduce");
    m_produceEvent.init(std::bind(&KafkaPartition::createProduceRequest, this, std::placeholders::_1));
    m_reactor.registerFdCallback(m_produceEvent.selectFd(),
                                 m_produceEvent.eventData(),
                                 m_produceEvent.getEventFlags(),
                                 false,
                                 false);

    m_produceMsgQEvent.init(std::bind(&KafkaPartition::notifyProduceMsgQ, this, std::placeholders::_1));
    m_reactor.registerFdCallback(m_produceMsgQEvent.selectFd(),
                                 m_produceMsgQEvent.eventData(),
                                 m_produceMsgQEvent.getEventFlags(),
                                 false,
                                 false);

    m_pReqTimerEvent.init((m_config.pMaxBatchWaitTimeMs/1000),
                        std::bind(&KafkaPartition::createProduceRequest, this, std::placeholders::_1));

    m_reactor.registerFdCallback(m_pReqTimerEvent.selectFd(),
                                 m_pReqTimerEvent.eventData(),
                                 m_pReqTimerEvent.getEventFlags(),
                                 false,
                                 false);

    m_produceMsgQEvent.wakeup();
    return 0;
}

int KafkaPartition::produce(std::string msg)
{
    if ((m_iMsgCnt == m_config.pBatchNumMsgs) && (m_partitionType == PRODUCER_P)) {
        m_produceEvent.wakeup();
        m_iMsgCnt = 0;
    }

    m_qMsgs.push(std::move(msg));
    ++m_iMsgCnt;

    m_produceMsgQEvent.wakeup();
    return 0;
}

int KafkaPartition::produce(std::vector<std::string> msgs)
{
    std::vector<std::string>::iterator itr;

    if (msgs.empty()) {
        ERROR("Produce message list is empty.");
        return 1;
    }

    for(itr= msgs.begin(); itr != msgs.end(); ++itr) {
        std::string & msg = *itr;
        if ((m_iMsgCnt == m_config.pBatchNumMsgs) && (m_partitionType == PRODUCER_P)) {
            m_produceEvent.wakeup();
            m_iMsgCnt = 0;
        }

        m_qMsgs.push(std::move(msg));
        ++m_iMsgCnt;
    }

    m_produceMsgQEvent.wakeup();
    return 0;
}

void KafkaPartition::createProduceRequest(uint64_t wakeups)
{
    // DEBUG("Producing messages to partition ID: %d", m_iPartition);
    size_t req_size = 0;
    ProduceRequest request;
    request.set_correlation_id(libkafka_asio::constants::kApiKeyProduceRequest);
    request.set_required_acks(m_config.pRequiredAcks);
    request.set_timeout(m_config.pTimeout);
    while (!m_qMsgs.empty()) {
        if (req_size >= (m_config.cMaxFetchBytes - 1024)) {
            DEBUG("%s : produce req size >= (m_config.cMaxFetchBytes - 1024)", m_config.clientId.c_str());
            std::stringstream os(std::ios_base::out);
            libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);
            m_dqWriteBuffers.push_back(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyProduceRequest, os.str()));
            request.Clear();
            req_size = 0;
        }
        std::string &  msg = m_qMsgs.front();
        /* AddValue() creates it own copy of message. So passing reference is fine */
        request.AddValue(msg, m_strTopic, m_iPartition);
        req_size += msg.length() + 10 /* Extra header bytes for each msg */;
        m_qMsgs.pop();
    }
    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);

    m_dqWriteBuffers.push_back(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyProduceRequest, os.str()));

    m_reactor.registerFdCallback(selectFd(),
                                eventData(),
                                getEventFlags(),
                                false,
                                true);

}

void KafkaPartition::handleProduceResponse(std::stringstream & is)
{
    //INFO("%s : handleProduceResponse", m_strEndpointId.c_str());
    ProduceRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);

    // TODO: In the errored case, we should re send all the messages in the failed produce request.
    //       This case is not handled currently
    if (ec) {
        ERROR("There is error in produce request.");
        handleKafkaError(ec.value());

        std::string req = m_qInTransitProduceRequest.front();
        m_dqWriteBuffers.push_back(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyProduceRequest, req.c_str()));
        m_qInTransitProduceRequest.pop();
    } else {
        m_qInTransitProduceRequest.pop();

        const ProduceResponse::OptionalType& rsp = response.response();
        ProduceResponse::Topic::Partition::OptionalType partition = rsp->FindTopicPartition(m_strTopic, m_iPartition);

        if (partition && onProduceResponse) {
            onProduceResponse(m_strTopic, m_iPartition, partition->offset);
        }
    }

}

void KafkaPartition::notifyProduceMsgQ(uint64_t wakeups)
{
    if (onMsgQueued) {
        onMsgQueued();
    }
}

void KafkaPartition::createFetchRequest(uint64_t wakeups)
{
    INFO("%s : Partition %d : m_iNextFetchOffset = %lld", m_config.clientId.c_str(), m_iPartition, m_iNextFetchOffset);
    if (m_bFetchPaused) {
        return;
    }

    FetchRequest request;
    request.set_correlation_id(libkafka_asio::constants::kApiKeyFetchRequest);
    request.set_max_wait_time(m_config.cMaxFetchWaitTimeMs);
    request.set_min_bytes(m_config.cMinFetchBytes);
    request.FetchTopic(m_strTopic, m_iPartition, m_iNextFetchOffset, m_config.cMaxFetchBytes);
    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);

    m_dqWriteBuffers.push_back(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyFetchRequest, os.str()));

    m_reactor.registerFdCallback(selectFd(),
                                eventData(),
                                getEventFlags(),
                                false,
                                true);

}

void KafkaPartition::handleFetchResponse(std::stringstream & is)
{
    FetchRequest::MutableResponseType response;
    boost::system::error_code ec;
    libkafka_asio::detail::ReadResponse(is, response, ec);
    if(ec) {
        handleKafkaError(ec.value());
    } else {

        const FetchResponse::OptionalType& rsp = response.response();
        std::vector<std::string> recvMsgs;
        recvMsgs.reserve(1000);
        FetchResponse::const_iterator iter;
        for (iter = rsp->begin(); iter != rsp->end(); ++iter) {
            const MessageAndOffset & message= *iter;
            /* When there is not message on broker for this T+P combination, broker returns */
            /* blank message where the offest field will be set to 0                        */
            /* This check also handles the condition in the below mention bug               */
            /* https://issues.apache.org/jira/browse/KAFKA-1744                             */
            if ((message.offset() >= m_iNextFetchOffset)) {
                m_iNextFetchOffset = message.offset() + 1;
            } else {
                continue;
            }

            const libkafka_asio::Bytes& bytes = message.value();
            if (bytes) {
                std::string recvMsg((const char*) &(*bytes)[0], bytes->size());
                recvMsgs.push_back(std::move(recvMsg));
            }
        }

        /* send the fetched log msgs to the application */
        if ((!recvMsgs.empty()) && onReceivedData) {
            //DEBUG("calling onReceivedData() callback");
            onReceivedData(std::move(recvMsgs));
        }
    }

    if (m_partitionType == CONSUMER_P) {
        m_fetchEvent.wakeup();
    }
}

void KafkaPartition::commitKafkaOffset(uint64_t wakeups) {
    uint64_t commitOffset = m_iNextFetchOffset - 1;
    if (commitOffset >= 0) {
        std::ofstream myfile;
        myfile.open (m_strStoredOffsetFileName);
        myfile << commitOffset;
        myfile.close();
    }
}

void KafkaPartition::createMetadataRequest(uint64_t wakeups)
{
    //INFO("createMetadataRequest");
    MetadataRequest request;
    request.set_correlation_id(libkafka_asio::constants::kApiKeyMetadataRequest);
    request.AddTopicName(m_strTopic);
    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);

    //std::cout << os.str() << std::endl;
    m_dqWriteBuffers.push_front(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyMetadataRequest, os.str()));

    m_reactor.registerFdCallback(selectFd(),
                                 eventData(),
                                 getEventFlags(),
                                 false,
                                 true);
}

void KafkaPartition::handleMetadataResponse(std::stringstream & is)
{
    MetadataRequest::MutableResponseType response;
    boost::system::error_code ec;
    char ip[100];
    libkafka_asio::detail::ReadResponse(is, response, ec);
    if(ec) {
        handleKafkaError(ec.value());
    }

    const MetadataResponse::OptionalType& rsp = response.response();
    MetadataResponse::Broker::OptionalType leader = rsp->PartitionLeader(m_strTopic, m_iPartition);

    /* Kafka server returns leader/brokers "hostname" and not its IP address. */
    /* So if there is leader changed happened, then we have to disconnect     */
    /* from current broker and connect to the new leader.                     */

    if (leader) {
        AsyncKakfa::utils::hostnameToIp((leader->host).c_str() , ip);

        if (ip != m_sLeader.m_strIp || leader->port != m_sLeader.m_iPort) {
            DEBUG("Leader for topic + partition (%s + %d )is changed. Reconnecting to new leader", m_strTopic.c_str(), m_iPartition);
            m_reactor.unregisterFdCallback(m_iSockFd);
            close(m_iSockFd);
            m_iSockFd = -1;

            /* save the new leader details */
            m_sLeader.m_iId = leader->node_id;
            m_sLeader.m_strHost = leader->host;
            m_sLeader.m_strIp = ip;
            m_sLeader.m_iPort = leader->port;

            DEBUG("New Leader IP : %s   Port : %d", m_sLeader.m_strIp.c_str(), m_sLeader.m_iPort);
            if (!connectLeader()) {
                ERROR("connection to kafka server failed");
                m_reactor.shutdown(0);
            }

            DEBUG("New leader sockFd is : %d", m_iSockFd);

            if (m_partitionType == CONSUMER_P) {
                m_fetchEvent.wakeup();
            }
        }

    } else {
        INFO("\nNo leader found!");
    }
}

void KafkaPartition::createOffsetRequest(uint64_t wakeups)
{
    OffsetRequest request;
    request.set_correlation_id(libkafka_asio::constants::kApiKeyOffsetRequest);
    using libkafka_asio::constants::kOffsetTimeLatest;
    using libkafka_asio::constants::kOffsetTimeEarliest;

    int64_t offsetTime = kOffsetTimeEarliest;
    if (m_config.cStartOffset == KafkaFetchOffest::FROM_END) {
        offsetTime = kOffsetTimeLatest;
    }

    request.FetchTopicOffset(m_strTopic,
                             m_iPartition,
                             offsetTime
                            );
    std::stringstream os(std::ios_base::out);
    libkafka_asio::detail::WriteRequest(request, m_config.clientId, os);

    //std::cout << os.str() << std::endl;
    m_dqWriteBuffers.push_front(std::pair<int32_t, std::string>(libkafka_asio::constants::kApiKeyOffsetRequest, os.str()));

    m_reactor.registerFdCallback(selectFd(),
                                 eventData(),
                                 getEventFlags(),
                                 false,
                                 true);

}

void KafkaPartition::handleOffsetResponse(std::stringstream & is)
{
    OffsetRequest::MutableResponseType response;
    boost::system::error_code ec;

    libkafka_asio::detail::ReadResponse(is, response, ec);
    if(ec) {
        handleKafkaError(ec.value());
    }

    const OffsetResponse::OptionalType& rsp = response.response();
    OffsetResponse::Topic::Partition::OptionalType partition = rsp->TopicPartitionOffset(m_strTopic, m_iPartition);
    if (!partition || partition->offsets.empty()) {
        ERROR("Failed to fetch the offsets for topic : %s   partition : %d", m_strTopic.c_str(), m_iPartition);
        return;
    }

    int64_t newStartOffset = partition->offsets[0];
    INFO ("%s : Partition newStartOffset =  %lld", m_config.clientId.c_str(), newStartOffset);

    if (m_config.cStartOffset == KafkaFetchOffest::FROM_END) {
        m_iNextFetchOffset = newStartOffset;
        INFO ("%s : Reseting the fetch offset to = %lld", m_config.clientId.c_str(), newStartOffset);
    } else {
        if (m_iNextFetchOffset < newStartOffset) {
            INFO ("%s : m_iNextFetchOffset (%lld) < partition newStartOffset (%lld)", m_config.clientId.c_str(), m_iNextFetchOffset, newStartOffset);
            INFO ("%s : Reseting the fetch offset to = %lld", m_config.clientId.c_str(), newStartOffset);
            m_iNextFetchOffset = newStartOffset;
        }
    }

}


void KafkaPartition::handleEvent(const int fd, const short event, void * arg)
{
    if (fd != (int)selectFd()) {
        return;
    }
    if(event & AEVENTIN) {
        onReadReady();
    } else if (event & AEVENTOUT) {
        onWriteReady();
    } else {
        ERROR("Unknown event of Kafka socket");
    }
}

void
inline KafkaPartition::handleKafkaError(int ec){
    switch (ec)
    {
        case kErrorNoError:
            ERROR("No error");
            break;
        case kErrorUnknown:
            ERROR("Unexpected server error");
            break;
        case kErrorOffsetOutOfRange:
            ERROR("The requested offset is outside the range of offsets "
            "maintained by the server for the given topic/partition.");
            /* send the offset request to get the updated start offset */
            createOffsetRequest(0);
            onWriteReady();
            break;
        case kErrorInvalidMessage:
            ERROR("message content does not match its CRC.");
            break;
        case kErrorUnknownTopicOrPartition:
            ERROR("Topic or partition does not exist on this broker.");
            break;
        case kErrorInvalidMessageSize:
            ERROR("The message has a negative size");
            break;
        case kErrorLeaderNotAvailable:
        case kErrorNotLeaderForPartition:
            {
                if (ec == kErrorLeaderNotAvailable) {
                    ERROR("There is currently no leader for this partition and hence it "
                    "is unavailable for writes.");
                } else {
                    ERROR("Message was sent to a replica that is not the leader for this"
                    " partition (%d). Client metadata is out of date.", m_iPartition);
                }
                createMetadataRequest(0);
                onWriteReady();
            }
            break;
        case kErrorRequestTimedOut:
            ERROR("Request exceeded the user-specified time limit");
            break;
        case kErrorBrokerNotAvailable:
            ERROR("Broker not available.");
            break;
        case kErrorReplicaNotAvailable:
            ERROR("Replica was expected on this broker but is not available.");
            break;
        case kErrorMessageSizeTooLarge:
            ERROR("Message was too large");
            break;
        case kErrorStaleControllerEpochCode:
            ERROR("Internal error StaleControllerEpochCode");
            break;
        case kErrorOffsetMetadataTooLargeCode:
            ERROR("Specified string larger than configured maximum for offset "
            "metadata");
            break;
        case kErrorOffsetLoadInProgressCode:
            ERROR("Offset fetch request is still loading offsets");
            break;
        case kErrorConsumerCoordinatorNotAvailableCode:
            ERROR("Offsets topic has not yet been created.");
            break;
        case kErrorNotCoordinatorForConsumerCode:
            ERROR("Request was for a consumer group that is not coordinated by "
            "this broker.");
            break;
        default:
            ERROR("Kafka error");
    }
}

void KafkaPartition::onReadReady()
{
    uint32_t size;
    int res = -1;

    auto readError = [&]() {
        if (res == -1) {
            ERROR("KafkaEndpoint read() failed with errno: %d (%s)", errno, strerror(errno));
        }

        if (   res == 0 // The other end (Kafka server closed the connection)
            || ((res == -1) && (errno == ECONNRESET))) {

            if (errno == ECONNRESET) { // ECONNRESET (104) - Connection reset by peer)
                ERROR("%s: Kafka Server reset the connection. Partition : %d", m_config.clientId.c_str(), m_iPartition);
            } else {
                ERROR("%s:(%d) Kafka Server closed the connection. Partition : %d", m_config.clientId.c_str(), __LINE__, m_iPartition);
            }
            m_reactor.unregisterFdCallback(m_iSockFd);
            close(m_iSockFd);
            m_iSockFd = -1;
            DEBUG("%s: Reconnecting to other broker... ", m_config.clientId.c_str());
            std::vector<KafkaBroker>::iterator it;
            int32_t leaderNodeID = m_sLeader.m_iId;
            DEBUG("\t\t Old Leader ID   : %d", m_sLeader.m_iId);
            it = std::find_if(m_sBrokers.begin(), m_sBrokers.end(), [&](KafkaBroker broker){ return (broker.m_iId != leaderNodeID);});
            m_sLeader = *it;
            DEBUG("\t The new broker is:");
            DEBUG("\t\t ID   : %d", m_sLeader.m_iId);
            DEBUG("\t\t Host : %s", m_sLeader.m_strHost.c_str());
            DEBUG("\t\t IP   : %s", m_sLeader.m_strIp.c_str());
            DEBUG("\t\t Port : %d", m_sLeader.m_iPort);

            if (!connectLeader()) {
                ERROR("connection to kafka server failed");
                m_reactor.shutdown(0);
                return;
            }

            createMetadataRequest(0);
            onWriteReady();
            if (m_partitionType == CONSUMER_P) {
                m_fetchEvent.wakeup();
            }

            INFO("New kafkaPartition sockFd is : %d", m_iSockFd);
            return;
        }
    };
    /* The reason for this sleep:                            */
    /*    After read event if er immediately for read, then  */
    /*    most of the times we don't receive any data. If we */
    /*    go after this sleep time we get all the data. This */
    /*    is the observation till now. We need to find out   */
    /*    reason for why data is not ready immediately and   */
    /*    we should remove this sleep part.                  */
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    res = AsyncKakfa::utils::readData(m_iSockFd, (char*)&size, sizeof(uint32_t));
    if (res <= 0) {
        readError();
        return;
    }
    size = ntohl(size);
    std::unique_ptr<char[]> upData(new char[size]);
    char *data = upData.get();
    res = AsyncKakfa::utils::readData(m_iSockFd, data, size);
    if (res <= 0) {
        readError();
        return;
    }

    char *tmp = data;
    int32_t correlation_id = -1;
    memcpy((char *)&correlation_id, tmp, sizeof(int32_t));
    correlation_id = ntohl(correlation_id);
    //INFO("onReadReady: The correlation id after ntohl(): %d", correlation_id);
    assert(correlation_id != -1);

    std::string msg(data, size);
    std::stringstream  is(msg, std::ios_base::in);
    data = NULL;
    tmp = NULL;
    switch (correlation_id) {
        case libkafka_asio::constants::kApiKeyProduceRequest:
            handleProduceResponse(is);
            break;
        case libkafka_asio::constants::kApiKeyFetchRequest:
            handleFetchResponse(is);
            break;
        case libkafka_asio::constants::kApiKeyMetadataRequest:
            handleMetadataResponse(is);
            break;
        case libkafka_asio::constants::kApiKeyOffsetRequest:
            handleOffsetResponse(is);
            break;
    }

    return;
}

void KafkaPartition::onWriteReady()
{
   // std::cout << "onWriteReady()" << std::endl;
    if (m_dqWriteBuffers.empty()) {
        m_reactor.registerFdCallback(selectFd(),
                                    eventData(),
                                    AEVENTIN,
                                    false,
                                    true);

        //INFO("No write buffer available");
        return;
    }

    std::pair<int32_t, std::string>  req = m_dqWriteBuffers.front();
    m_dqWriteBuffers.pop_front();

    // Save the last sent produce request. If any error occures in the response, then we will
    // re-send the last failed request.
    if (req.first == libkafka_asio::constants::kApiKeyProduceRequest) {
        m_qInTransitProduceRequest.push(req.second);
    }

    //std::cout << os << std::endl;

    /* TODO: We can write multiple requests from the m_dqWriteBuffers  */
    /*       in this single write call, till the write buffer is full  */
    /*       (till it don't return EAGIAN errno )                      */
    int res = write(m_iSockFd, req.second.c_str(), req.second.length());
    if (res == -1) {
        ERROR("KafkaEndpoint write() failed with errno: %d (%s)", errno, strerror(errno));
        if (errno == ECONNRESET) { // ECONNRESET (104) - Connection reset by peer
            ERROR("%s: Kafka Server reset the connection. ", m_config.clientId.c_str());
            m_reactor.unregisterFdCallback(m_iSockFd);
            close(m_iSockFd);
            m_iSockFd = -1;
            DEBUG("%s: Reconnecting to other broker... ", m_config.clientId.c_str());
            std::vector<KafkaBroker>::iterator it;
            int32_t leaderNodeID = m_sLeader.m_iId;
            DEBUG("\t\t Old Leader ID   : %d", m_sLeader.m_iId);
            it = std::find_if(m_sBrokers.begin(), m_sBrokers.end(), [&](KafkaBroker broker){ return (broker.m_iId != leaderNodeID);});
            m_sLeader = *it;
            DEBUG("\t The new broker is:");
            DEBUG("\t\t ID   : %d", m_sLeader.m_iId);
            DEBUG("\t\t Host : %s", m_sLeader.m_strHost.c_str());
            DEBUG("\t\t IP   : %s", m_sLeader.m_strIp.c_str());
            DEBUG("\t\t Port : %d", m_sLeader.m_iPort);

            if (!connectLeader()) {
                ERROR("connection to kafka server failed");
                m_reactor.shutdown(0);
                return;
            }

            createMetadataRequest(0);
            onWriteReady();
            if (m_partitionType == CONSUMER_P) {
                m_fetchEvent.wakeup();
            }

            INFO("New kafkaPartition sockFd is : %d", m_iSockFd);
            return;
        }
    }
    //printf(" size = %ld, res = %d\n", os.length(), res);
}

int
KafkaPartition::selectFd() const
{
    return m_iSockFd;
}

int
KafkaPartition::getEventFlags() const
{
    return m_iEventFlags;
}

KafkaPartition::EventData *
KafkaPartition::eventData()
{
    return &m_eventData;
}

} // namespace AsyncKakfa
