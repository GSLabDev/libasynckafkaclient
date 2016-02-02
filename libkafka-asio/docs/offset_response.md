
class `OffsetResponse`
======================

**Header File:** `<libkafka_asio/offset_response.h>`

**Namespace:** `libkafka_asio`

Implementation of the Kafka OffsetResponse as described on the 
[Kafka wiki](https://cwiki.apache.org/confluence/display/KAFKA/A+Guide+To+The+Kafka+Protocol#AGuideToTheKafkaProtocol-OffsetResponse).
An object of this type will be given as response object to the handler function
when invoking an offset request.

<img src="http://yuml.me/diagram/nofunky;scale:80/class/
[OffsetResponse]++-*[Topic], 
[Topic]++-*[Topic::Partition]" 
/>

Member Functions
----------------

### TopicPartitionOffset
```cpp
Topic::Partition::OptionalType TopicPartitionOffset(const String& topic_name, 
                                                    Int32 partition) const
```

Search for offset data for the given topic partition inside this response 
object. If no such data can be found, the return value is empty.

```cpp
// Assume the response is an argument of the request handler function
OffsetResponse::OptionalType response;

// Get the offset data for topic 'foo' partition 1
OffsetResponse::Topic::Partition::OptionalType offsets;
offsets = response->TopicPartitionOffset("foo", 1);
if (offsets)
{
    // [...]
}
```


### topics
```cpp
const TopicVector& topics() const
```

Returns a reference to the set of topics, offsets have been received for.


Types
-----

### Topic
```cpp
struct Topic
```

+ `topic_name`:
   Name of the topic to fetch data for.
+ `partition_offsets`:
   Set of partitions of this topic for which offset data has been received.


### Topic::Partition
```cpp
struct Topic::Partition
```

+ `partition`:
   Number, identifying this topic partition.
+ `error_code`:
   Kafka error for this topic partition.
+ `offsets`:
   Vector of offsets (`std::vector<Int64>`) received for this topic partition.


### TopicVector
```cpp
typedef std::vector<Topic> TopicVector
```

Vector of `Topic` structures.


### OptionalType
```cpp
typedef boost::optional<OffsetResponse> OptionalType
```

A offset response object wrapped using _Boost optional_. Such an object will
be used for offset request handler functions.
