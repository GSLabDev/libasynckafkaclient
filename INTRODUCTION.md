# Introduction to libasynckafka - the Apache Kafka C++ Async Client Library


libasynckafka is a high performance C++ implementation of the Apache
Kafka client, providing a reliable and performant client for production use.
It is designed using [libevent](http://libevent.org/).

## Contents

The following chapters are available in this document

  * Supported Features
  * Limitation
  * Documentation
  * Initiazation
  * Configuration
  * Brokers
  * Producer
  * Consumer
  * Performance
  * Appendix


## Supported Features

  * Kakfa Protocol version 0.8 is supported only
  * Producer and Consumer APIs
  * Offset auto commit to local store
  * Single/Multi partition producer and consumer can be created
  * Consumer can be started from any of three offset (beginning/stored/end)
  * Message reliability for the producer
  * Client gracefully handles the following events. Application doesn't warry about them:
    * Broker connection failure
    * Metadata Updates
    * Topic leader change


## Limitations
  * This client is based on libevent for aysnc operation. So this client can be used
    by "libevent" based Application only. The Producer or Consumer object from this client
    needs libevents event_base as input parameter
  * Compression is NOT supported

### Documentation

The libasynckafka API is documented in the
[`KafkaClient.h`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/src/KafkaClient.h)
header file, the configuration properties are documented in
[`CONFIGURATION.md`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/CONFIGURATION.md)

### Initialization

The application needs to have one object of libevents event_base. Application then can create
/instantiate object of `AsyncKakfa::KafkaConsumer` or `AsyncKakfa::KafkaProducer`. These are
the top level objects. Both of these object take few input arguments which are explained below:

    AsyncKakfa::KafkaClientConfig kConfig; /* libasynckafka configuration object to configure few properties of Kafka Protocol */
                                           /* Details of the object are given below in Configuration section                   */

    AsyncKakfa::KafkaConsumer consumer(base,              /* Libevent's event base object pointer */
                                       topicStr,          /* Topic Name */
                                       partitionId,       /* Partition ID: 0 ... N OR -1 to consume/producer from all partitions */
                                       serverIp,          /* Broker IP */
                                       port,              /* Broker Port number */
                                       kConfig);          /* Configuration object */

**Note**: For the `serverIp` only IP is supported currently. Hostname is not supported.

   You also have to register the callback to receive the fetched messages. For all callback APIs details please check the KafkaClient.h file

### Configuration

To ease the integration with official Apache Kafka Protocol, libasynckafka implements identical configuration properties found in the official
clients of Apache Kafka. These properties can be used to tune the performance/reliability of the Kafka client. You have to create an object
of `AsyncKakfa::KafkaClientConfig` and set required properties to appropriate value.

This object is mandatory parameter for creating `AsyncKakfa::KafkaConsumer` OR `AsyncKakfa::KafkaProducer` object. Details of configuration
properties and their default value are given in [`CONFIGURATION.md`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/CONFIGURATION.md) file.

### Brokers

Only one broker details (IP:PORT) are needed to create the KafkaClient object (consumer/producer). The client will take care of finding the leader
for the partitions and connecting to them. You have to specify IP address (hostname is not supported) of any one broker.

### Producer

#### To create producer object use below API:
    AsyncKakfa::KafkaClientConfig kConfig; /* libasynckafka configuration object to configure few properties of Kafka Protocol */
                                           /* Details of the object are given below in Configuration section                   */

    AsyncKakfa::KafkaProducer producer(base,              /* Libevent's event base object pointer */
                                       topicStr,          /* Topic Name */
                                       partitionId,       /* Partition ID: 0 ... N OR -1 to consume/producer from all partitions */
                                       serverIp,          /* Broker IP */
                                       port,              /* Broker Port number */
                                       kConfig);          /* Configuration object */


   `AsyncKakfa::KafkaClientConfig` object has some properties to tune producer's performance. For details of these properties
   please check [`CONFIGURATION.md`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/CONFIGURATION.md) file:

    * `pPartitioningMethod` : Currently only ROUND_ROBIN method is supported
    * `pRequiredAcks` : # of acknowledgements from ISRs for message commit to client
    * `pTimeout` : Check details of this property in CONFIGURATION.md
    * `pBatchNumMsgs` : Batch length
    * `pMaxBatchWaitTimeMs` : Check details of this property in CONFIGURATION.md

### Callbacks to be registered and used:

	  * onMsgQueued : This callback will be called when each message is queued in the internal produce queue
                      You can send/produce next message from this callback.

                      Format : int onMsgQueued()

	  * onProduceResponse : This callback will be called when client receives response from Kafka server for produce request

                            Format : int onProduceResponse(std::string topic, int32_t partition, int64_t offset)

                            This callback will return topic, partition and last produce offset

      Both of these callbacks are optional

#### Start the producer:
Once you have registered the callback, you have to start the producer using below API:

    producer->startProduce();

This call will start the producer and will call a `onMessageQueued` callback.

#### Produce the messages:
To produce the messages you have to call produce() API:

    std::string msg;
    producer->produce(msg);

Other variant of produce API is also availiable. You can give message vector as input parameter:

    std::vector<std::string> msgs;
    producer->produce(msgs);

#### Producer object with `partitionId` = -1 (produce to all partitions of the topic)

In this configuration, messages will be send to all partitions in round robin fashion.

**Note**: Kafka server supports message ordering within a partition only. If you produce messages to all
          the partitions, the messages will go in different partitions in round robin fashion and message
          ordering will not be there.

#### Topic auto creation

Topic auto creation is supported by libasynckafka.
The broker needs to be configured with "auto.create.topics.enable=true".

**Note**: See the [`examples/producer-00.cpp`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/examples/producer-00.cpp) for producer implementation.

### Consumer

#### To create consumer object use below API:
    AsyncKakfa::KafkaClientConfig kConfig; /* libasynckafka configuration object to configure few properties of Kafka Protocol */
                                          /* Details of the object are given below in Configuration section                   */

    AsyncKakfa::KafkaConsumer consumer(base,              /* Libevent's event base object pointer */
                                       topicStr,          /* Topic Name */
                                       partitionId,       /* Partition ID: 0 ... N OR -1 to consume/producer from all partitions */
                                       serverIp,          /* Broker IP */
                                       port,              /* Broker Port number */
                                       kConfig);          /* Configuration object */


   `AsyncKakfa::KafkaClientConfig` object has some properties to tune consumer's performance. For details of these properties
   please check [`CONFIGURATION.md`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/CONFIGURATION.md) file:

     * `cMaxFetchWaitTimeMs` : Max wait time to get the sufficient amount of data
     * `cMaxFetchBytes` : Max bytes to include in the response
     * `cMinFetchBytes` : Min # of bytes of messages that should be available to give the response
     * `cAutoCommitEnable` : Auto commit of fetched offset to local store
     * `cAutoCommitIntervalMs` : The frequency in milliseconds that the consumer offsets are commited (written) to offset storage
     * `cStartOffset` : where to start the consumer (beginning/stored/end)
     * `cOffsetStorePath` : Offset store file path

#### Callbacks to be registered and used:

	  * onFetchedData : This callback will be called batch of consumed message is received from server

                      Format : int onFetchedData(std::vector<std::string> msgs)

                      The consumed messages will be returned in the form string vector

#### Start the consumer:
Once you have registered the callback, you have to start the consumer using below API:

    consumer->startFetch();

#### Offset Management:
Offset management is available through local offset file store, where the offset is periodically written to local file for
each topic+partition according to following configuration properties:

	  * `cAutoCommitEnable` : Auto commit of fetched offset to local store
	  * `cAutoCommitIntervalMs` : The frequency in milliseconds that the consumer offsets are commited (written) to offset storage
	  * `cOffsetStorePath` : Offset store file path

**Note**: See the `examples/consumer-00.cpp` [`examples/consumer-00.cpp`](https://github.com/GSLabDev/libasynckafkaclient/blob/master/examples/consumer-00.cpp) for consumer implementation

## Performance
