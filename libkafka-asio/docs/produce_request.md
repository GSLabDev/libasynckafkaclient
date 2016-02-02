
class `ProduceRequest`
======================

**Header File:** `<libkafka_asio/produce_request.h>`

**Namespace:** `libkafka_asio`

Implementation of the Kafka ProduceRequest as described on the 
[Kafka wiki](https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-ProduceRequest).
Produce requests are used to send data for one or more topic partitions to a
remote Kafka server.

<img src="http://yuml.me/diagram/nofunky;scale:80/class/
[ProduceRequest]++-*[Topic], 
[Topic]++-*[Topic::Partition]" 
/>

Member Functions
----------------

### AddValue (overload 1 of 2)
```cpp
void AddValue(const Bytes& value, 
              const String& topic_name,
              Int32 partition)
```

Constructs a `Message` with value set to byte vector given in `value`. The
message will be added to this request object to produce it for the topic
partition with the given `topic_name` and `partition`.
The given value will be copied.


### AddValue (overload 2 of 2)
```cpp
void AddValue(const String& value,
              const String& topic_name,
              Int32 partition)
```

Constructs a `Message` with value set to the bytes of the given `value` string.
The message will be added to this request object to produce it for the topic
partition with the given `topic_name` and `partition`.


### AddMessage
```cpp
void AddMessage(const Message& message,
                const String& topic_name,
                Int32 partition)
```

Adds a copy of the given message to this produce request. The message will be
produced for the given `topic_name` and `partition`.


### AddMessageSet
```cpp
void AddMessageSet(const MessageSet& message_set,
                   const String& topic_name,
                   Int32 partition)
```

Copies the given set of message into this produce request. It has the same
effect as calling `AddMessage` for each message inside the given `MessageSet`.
The messages will be produced for the given `topic_name` and `partition`.


### Clear
```cpp
void Clear()
```

Clears all message data of this produce request.


### ClearTopic
```cpp
void ClearTopic(const String& topic_name)
```

Clears the message data for the topic with the given name.


### ResponseExpected
```cpp
bool ResponseExpected() const
```

Returns `true` in case a response is expected for this produce request. (Produce
requests are the only kinds of requests where there is the possibility that
the server will not reply with a response). This function is called internally
after the request was successfully sent to the server and the library needs to
determine, if a response is expected.


### set_required_ack
```cpp
void set_required_acks(Int16 required_acks)
```

Sets the number of acknowledgements that need to be received by the server
before the response for this request is sent. If `0` is specified for this
parameter, the server will not wait for acknowledgements. In this case, no
response will be sent by the server.


### set_timeout
```cpp
void set_timeout(Int32 timeout)
```

Timeout in milliseconds to wait for required acknowledgements.


Types
-----

### Topic
```cpp
struct Topic
```

+ `topic_name`:
   Name of the topic to produce messages for.
+ `partitions`:
   Vector of `TopicPartition` objects.   
   

### Topic::Partition
```cpp
struct Topic::Partition
```

+ `partition`:
   Number, identifying this topic partition.
+ `messages`:
   Set of messages to produce for this topic partition


### ResponseType
```cpp
typedef ProduceResponse ResponseType
```

Type of the response object of a produce request.


### MutableResponseType
```cpp
typedef MutableProduceResponse MutableResponseType
```

Type of a mutable response object for a produce request. This type is used by 
the library at when reading-in the response from a Kafka server.


### TopicVector
```cpp
typedef std::vector<Topic> TopicVector
```

Vector of `Topic` objects. Produce requests can produce message for multiple
topics and partitions.
