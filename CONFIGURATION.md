## Global configuration properties

Property                                 | C/P |       Default | Description
-----------------------------------------|-----|--------------:|--------------------------
clientId                                 |  *  |   kafkaClient | Client identifier for Kafka Server.
mRefreshIntervalMs                       |  *  |        300000 | Topic metadata refresh interval in milliseconds. The metadata is automatically refreshed on error and connect.
cMaxFetchWaitTimeMs                      |  C  |          1000 | The max wait time is the maximum amount of time in milliseconds to block waiting if insufficient data is available at the time the request is issued.
cMaxFetchBytes                           |  C  |   16K (16384) | The maximum bytes to include in the message set for this partition. This helps bound the size of the response.
cMinFetchBytes                           |  C  |             1 | This is the minimum number of bytes of messages that must be available to give a response.If the client sets this to 0 the server will always respond immediately, however if there is no new data since their last request they will just get back empty message sets.
cAutoCommitEnable                        |  C  |          true | If true, periodically commit offset of the last message handed to the application. This commited offset will be used when the process restarts to pick up where it left off. If flase, client will not save the last fetched offset
cAutoCommitIntervalMs                    |  C  |         60000 | The frequency in milliseconds that the consumer offsets are commited (written) to offset storage.
cStartOffset                             |  C  |   FROM_STORED | From which offset to start consumtion. The same value will be used if desired offset goes out-of-range. (FROM_BEGINNING/FROM_END/FROM_STORED)
cOffsetStorePath                         |  C  |            "" | Path to local file for storing offsets. If the path is a directory a filename will be automatically generated in that directory based on the topic and partition.
pPartitioningMethod                      |  P  |   ROUND_ROBIN | Partitioning method to use while producing messages to all partitions of the topic. ROUND_ROBIN : Messages are sent to partitons in round robin fashion. PARTITION_FUNCTION : Partitioner function is used to findout partition ID. Application should register partitioner callback function
pRequiredAcks                            |  P  |             1 | This field indicates how many acknowledgements the leader broker must receive from ISR brokers before responding to the request. 0 = broker does not send any response, 1 = broker will wait until the data is written to local log before sending a response, -1 = broker will block until message is committed by all in sync replicas (ISRs), > 1 = the server will block waiting for this number of acknowledgements to occur
pTimeout                                 |  P  |          5000 | The ack timeout of the producer request in milliseconds. This value is only enforced by the broker and relies on request.required.acks being > 0
pBatchNumMsgs                            |  P  |          1000 | Maximum number of messages batched in one MessageSet. Produce request will be sends when client receives this many # of messages from application.
pMaxBatchWaitTimeMs                      |  P  |          5000 | Maximum time, in milliseconds, for buffering pBatchNumMsgs # in produce queue. If this time expires, produce request will be sent with whatever # of messages there in the produce queue.

### C/P legend: C = Consumer, P = Producer, * = both
