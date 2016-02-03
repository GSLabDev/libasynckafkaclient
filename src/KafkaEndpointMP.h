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

#ifndef AsyncKakfa_KafkaEndpointMP_INCLUDED
#define AsyncKakfa_KafkaEndpointMP_INCLUDED

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <thread>
#include <chrono>
#include <deque>
#include <queue>

#include "LibeventReactor.h"
#include "AsyncEventSource.h"
#include "PeriodicEventSource.h"
#include "WakeupEventSource.h"
#include "Logger.h"

namespace AsyncKakfa
{

#define KAFKA_ALL_PARTITIONS -1

namespace utils {

static int32_t connectServer(std::string ip, int16_t port)
{
    struct sockaddr_in serv_addr;
    int32_t sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd < 0) {
        ERROR("socket creation failed with errno: %d (%s)", errno, strerror(errno));
        return sockFd;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    /* Simple connection BackOff time logic for Kafka server connectoion */
    /* There are 4 retries for connection after waiting for 1 sec, 4 sec */
    /* 16 sec and 64 sec                                                 */
    int res = -1;
    int ii = 0;
    int maxRetries = 3;
    res = connect(sockFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (res == -1) {
        for (ii = 0; ii <= maxRetries; ++ii) {
            INFO("Retrying kafka server connection : %d time", ii);
            int timeout = (1 << (ii * 2));
            INFO("Waiting for : %d secs", timeout);
            std::this_thread::sleep_for(std::chrono::seconds(timeout));
            res = connect(sockFd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            if (res == -1) {
                if (ii == maxRetries) {
                    ERROR("All kafka server connection retries failed...");
                    close(sockFd);
                    return -1;
                }
            } else {
                break;
            }
        }
    }

    INFO("Connected to Kafka Server");

    return sockFd;
}

static int hostnameToIp(const char * hostname , char* ip)
{
    struct hostent *hostDetails;
    struct in_addr **addr_list;
    int i;

    if ( (hostDetails = gethostbyname( hostname ) ) == NULL)
    {
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) hostDetails->h_addr_list;

    /* TODO : if multiple addresses are returned in add_list[], then  */
    /*        the data in "ip" is not valid single IP. Need to fix it */
    for(i = 0; addr_list[i] != NULL; i++)
    {
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
}

static int readData(int32_t sockFd, char *data, size_t size)
{
    int res = -1;
    char *readPtr = data;
    size_t readBytes = 0;
    size_t tryRead = size;
    while (readBytes != size) {
        res = read(sockFd, readPtr, tryRead);
        if (res == -1 || /* some error occured while reading */
            res == 0     /* The other end (Kafka server closed the connection)*/
           ) {
            return res;
        }
        readBytes += res;
        tryRead -= res;
        readPtr = readPtr + res;
    }

    res = size;
    return res;
}

} // namespace utils

enum KafkaFetchOffest {
    FROM_BEGINNING = 0,
    FROM_END       = 1,
    FROM_STORED    = 2
};

enum KafkaPartitionType {
    CONSUMER_P = 0,
    PRODUCER_P = 1
};

enum KafkaPartitioningMethod {
    ROUND_ROBIN = 0,
    PARTITION_FUNCTION = 1
};

enum KafkaErrorType
{
    kErrorNoError = 0,
    kErrorUnknown = -1,
    kErrorOffsetOutOfRange = 1,
    kErrorInvalidMessage = 2,
    kErrorUnknownTopicOrPartition = 3,
    kErrorInvalidMessageSize = 4,
    kErrorLeaderNotAvailable = 5,
    kErrorNotLeaderForPartition = 6,
    kErrorRequestTimedOut = 7,
    kErrorBrokerNotAvailable = 8,
    kErrorReplicaNotAvailable = 9,
    kErrorMessageSizeTooLarge = 10,
    kErrorStaleControllerEpochCode = 11,
    kErrorOffsetMetadataTooLargeCode = 12,
    kErrorOffsetLoadInProgressCode = 14,
    kErrorConsumerCoordinatorNotAvailableCode = 15,
    kErrorNotCoordinatorForConsumerCode = 16
};


/* Common config parameters */
/*
    Name                    Type            Default Value       Desciption

    clientId                string          KafkaClient         Client identifier for Kafka Server

    mRefreshIntervalMs      int32_t         300000              Topic metadata refresh interval in milliseconds. The metadata
                                                                is automatically refreshed on error and connect.

    cMaxFetchWaitTimeMs     int32_t         1000                The max wait time is the maximum amount of time in
                                                                milliseconds to block waiting if insufficient data is
                                                                available at the time the request is issued.

    cMaxFetchBytes          int32_t         16K                 The maximum bytes to include in the message set for this
                                                                partition. This helps bound the size of the response.

    cMinFetchBytes          int32_t         1                   This is the minimum number of bytes of messages that must be
                                                                available to give a response.If the client sets this to 0 the
                                                                server will always respond immediately, however if there is
                                                                no new data since their last request they will just get back
                                                                empty message sets.

    cAutoCommitEnable       boolen          true                If true, periodically commit offset of the last message handed
                                                                to the application. This commited offset will be used when the
                                                                process restarts to pick up where it left off.
                                                                If flase, client will not save the last fetched offset

    cAutoCommitIntervalMs   int32_t         60000               The frequency in milliseconds that the consumer offsets are
                                                                commited (written) to offset storage.

    cStartOffset            int32_t         FROM_STORED         From which offset to start consumtion. The same value will be
                                                                used if desired offset goes out-of-range.

                                                                FROM_BEGINNING : Start from the smallest/oldest offset

                                                                FROM_END :  Start from largest/latest offset

                                                                FROM_STORED : Start from stored offset in OffsertCommit File

                                                                              If offset goes out-of-range, then oldest
                                                                              (FROM_BEGINNING) offset will be used

    cOffsetStorePath        string          ""                  Path to local file for storing offsets. If the path is a
                                                                directory a filename will be automatically generated in that
                                                                directory based on the topic and partition.

    pPartitioningMethod     int32_t         ROUND_ROBIN         Partitioning method to use while producing messages to all
                                                                partitions of the topic.

                                                                ROUND_ROBIN : Messages are sent to partitons in round robin
                                                                              fashion

                                                                PARTITION_FUNCTION : Partitioner function is used to findout
                                                                                     partition ID. Application should register
                                                                                     partitioner callback function

    pRequiredAcks           int16_t         1                   This field indicates how many acknowledgements the leader
                                                                broker must receive from ISR brokers before responding to
                                                                the request.
                                                                0 = broker does not send any response
                                                                1 = broker will wait until the data is written to local log
                                                                    before sending a response
                                                                -1 = broker will block until message is committed by all in
                                                                     sync replicas (ISRs)
                                                                > 1 = the server will block waiting for this number of
                                                                      acknowledgements to occur

    pTimeout                int32_t         5000                The ack timeout of the producer request in milliseconds. This
                                                                value is only enforced by the broker and relies on
                                                                request.required.acks being > 0.

    pBatchNumMsgs           int32_t         1000                Maximum number of messages batched in one MessageSet. Produce
                                                                request will be sends when client receives this many # of
                                                                messages from application.

    pMaxBatchWaitTimeMs     int32_t         5000                Maximum time, in milliseconds, for buffering pBatchNumMsgs #
                                                                in produce queue. If this time expires, produce request will
                                                                be sent with whatever # of messages there in the produce
                                                                queue.

*/


struct KafkaClientConfig
{

    KafkaClientConfig() {
        defaultInit();
    }

    ~KafkaClientConfig() {

    }

    KafkaClientConfig(const KafkaClientConfig & other)
    {
        clientId                = other.clientId;
        mRefreshIntervalMs      = other.mRefreshIntervalMs;

        cMaxFetchWaitTimeMs     = other.cMaxFetchWaitTimeMs;
        cMaxFetchBytes          = other.cMaxFetchBytes;
        cMinFetchBytes          = other.cMinFetchBytes;
        cAutoCommitEnable       = other.cAutoCommitEnable;
        cAutoCommitIntervalMs   = other.cAutoCommitIntervalMs;
        cStartOffset            = other.cStartOffset;
        cOffsetStorePath        = other.cOffsetStorePath;

        pPartitioningMethod     = other.pPartitioningMethod;
        pRequiredAcks           = other.pRequiredAcks;
        pTimeout                = other.pTimeout;
        pBatchNumMsgs           = other.pBatchNumMsgs;
        pMaxBatchWaitTimeMs     = other.pMaxBatchWaitTimeMs;
    }

    void defaultInit() {

        clientId                = "KafkaClient";
        mRefreshIntervalMs      = 300000;

        cMaxFetchWaitTimeMs     = 1000;
        cMaxFetchBytes          = (16 * 1024);
        cMinFetchBytes          = 1;
        cAutoCommitEnable       = true;
        cAutoCommitIntervalMs   = 60000;
        cStartOffset            = KafkaFetchOffest::FROM_STORED;
        cOffsetStorePath        = "";

        pPartitioningMethod     = KafkaPartitioningMethod::ROUND_ROBIN;
        pRequiredAcks           = 1;
        pTimeout                = 5000;
        pBatchNumMsgs           = 1000;
        pMaxBatchWaitTimeMs     = 5000;

    }

    std::string                 clientId;
    int32_t                     mRefreshIntervalMs;

    /* Consumer Specific Config Parameters */
    int32_t                     cMaxFetchWaitTimeMs;
    int32_t                     cMaxFetchBytes;
    int32_t                     cMinFetchBytes;
    bool                        cAutoCommitEnable;
    int32_t                     cAutoCommitIntervalMs;
    KafkaFetchOffest            cStartOffset;
    std::string                 cOffsetStorePath;

    /* Producer Specific Config Parameters */
    KafkaPartitioningMethod     pPartitioningMethod; //ROUND-ROBIN or with partitioner call back
    int16_t                     pRequiredAcks;
    int32_t                     pTimeout;
    int32_t                     pBatchNumMsgs;
    int32_t                     pMaxBatchWaitTimeMs;

};

struct KafkaBroker
{
    KafkaBroker(const int32_t id,
                  const std::string & host,
                  const std::string & ip,
                  const int16_t port):
            m_iId(id),
            m_strHost(host),
            m_strIp(ip),
            m_iPort(port)
        {}

    KafkaBroker():
        m_iId(-1),
        m_strHost(""),
        m_strIp(""),
        m_iPort(-1)
        {}

    ~KafkaBroker() {}

    KafkaBroker(const KafkaBroker & kOther)
    {
        m_iId     = kOther.m_iId;
        m_strHost = kOther.m_strHost;
        m_strIp   = kOther.m_strIp;
        m_iPort   = kOther.m_iPort;
    }

    int32_t     m_iId;
    std::string m_strHost;
    std::string m_strIp;
    int16_t     m_iPort;
};

class KafkaPartition: public AsyncEventSource
{
public:
    KafkaPartition(KafkaPartitionType partitionType,
                     LibeventReactor & reactor,
                     const std::string topic,
                     const int32_t partition,
                     const std::vector<KafkaBroker> brokers,
                     const KafkaBroker leader,
                     KafkaClientConfig & config);

    virtual ~KafkaPartition();

    void init();
    bool connectLeader();
    /* this method will push the msg on the internal queue */
    /* and return immediately  */
    int produce(std::string msg);
    int produce(std::vector<std::string> msgs);
    int startFetch();
    int startProduce();

    inline void pauseFetch() { m_bFetchPaused = true; }
    inline void resumeFetch() {
        if (m_bFetchPaused) {
            m_bFetchPaused = false;
            m_fetchEvent.wakeup();
        }
    }

    typedef std::function<int (std::vector<std::string> msgs)> onReceivedDataCb;
    typedef std::function<void (std::string new_leader)> onLeaderChangeCb;
    typedef std::function<int (int32_t errCode, std::string errStr)> onErrorCb;
    typedef std::function<int (std::string topic, int32_t partition, int64_t offset)> onProduceResponseCb;
    typedef std::function<int ()> onMsgQueuedCb;
    void handleKafkaError(int ec);
    onReceivedDataCb onReceivedData;
    onLeaderChangeCb onLeaderChange;
    onErrorCb        onError;
    onProduceResponseCb onProduceResponse;
    onMsgQueuedCb onMsgQueued;

    virtual int selectFd() const;
    virtual EventData * eventData ();
    virtual int getEventFlags() const;

private:
    void handleEvent(const int fd, const short event, void * arg);

    void onWriteReady();
    void onReadReady();
    void handleFetchResponse(std::stringstream & is);
    void handleProduceResponse(std::stringstream & is);
    void handleMetadataResponse(std::stringstream & is);
    void handleOffsetResponse(std::stringstream & is);
    void createProduceRequest(uint64_t wakeups);
    void createMetadataRequest(uint64_t wakeups);
    void createFetchRequest(uint64_t wakeups);
    void createOffsetRequest(uint64_t wakeups);
    void getKafkaOffset();
    void commitKafkaOffset(uint64_t wakeups);
    void notifyProduceMsgQ(uint64_t wakeups);

    WakeupEventSource             m_produceEvent;
    WakeupEventSource             m_produceMsgQEvent;
    WakeupEventSource             m_fetchEvent;

    PeriodicEventSource           m_mdTimerEvent;
    PeriodicEventSource           m_ocTimerEvent;
    PeriodicEventSource           m_pReqTimerEvent;

    std::string                     m_strStoredOffsetFileName;
    int64_t                         m_iNextFetchOffset;
    int32_t                         m_iMsgCnt;
    bool                            m_bFetchPaused;
    /* Push all the produce logs/msgs on this queue. When this count reaches one    */
    /* high limit, then send the batch of the messages on the socket               */
    std::queue<std::string>         m_qMsgs;

    /* push all the write/send requests (produce/fetch/metadata etc) on this queue  */
    /* whenever socket is ready for write pop one request from this queue and write */
    /* it on socket                                                                 */
    std::deque<std::pair<int32_t, std::string>> m_dqWriteBuffers;
    std::queue<std::string>         m_qInTransitProduceRequest;
    EventData                       m_eventData;
    int32_t                         m_iEventFlags;
    std::vector<KafkaBroker>        m_sBrokers;   /* broker Ip from config */
    KafkaBroker                     m_sLeader;
    std::string                     m_strTopic;    /* topic from the config */
    int32_t                         m_iPartition;  /* partition # from the config */
    LibeventReactor &               m_reactor;     /* Main reactor object reference */
    int32_t                         m_iSockFd;
    KafkaClientConfig &             m_config;
    KafkaPartitionType              m_partitionType;

};

} //namespace AsyncKakfa

#endif // AsyncKakfa_KafkaEndpointMP_INCLUDED
