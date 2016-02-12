libasynckafka - Apache Kafka C++ Async Client Library
==============================================

Copyright (c) 2016, Ganesh Nikam, [Great Software Laboratory Pvt Ltd](http://gslab.com/).

[https://github.com/GSLabDev/libasynckafkaclient](https://github.com/GSLabDev/libasynckafkaclient)

**libasynckafka** is a C++ library implementation of the
[Apache Kafka](http://kafka.apache.org/) protocol, containing both
Producer and Consumer support. It is designed as single threaded async library using
[libevent](http://libevent.org/). For Apache Kafka protocol message formatting it
uses [libkafka-asio](https://github.com/danieljoos/libkafka-asio).

**libasynckafka** is licensed under the 2-clause BSD license.

For an introduction to the performance and usage of librdkafka, see
[INTRODUCTION.md](https://github.com/GSLabDev/libasynckafkaclient/blob/master/INTRODUCTION.md)

See the [wiki](https://github.com/GSLabDev/libasynckafkaclient/wiki) for a FAQ.

**NOTE**: The `master` branch is actively developed, use latest release for production use.

**Apache Kafka 0.9 support:**
  * Not Supported

**Apache Kafka 0.8 support:**

  * Branch: master
  * Producer: supported
  * Consumer: supported
  * Compression: Not supported
  * ZooKeeper: not supported
  * Status: Beta


**Apache Kafka 0.7 support:**

  * Not Supported


**Apache Kafka 0.6 support:**

  * Not Supported


# Usage

## Requirements
	The GNU toolchain
	GNU make
   	gcc 4.7 or higher
	g++ 4.7 or higher
	Boost library
    Libevent
    librt

## Instructions

### Building

      make
      sudo make install

      By default make install will install the library in /usr/local/lib and will add header files in /usr/local/include.
      If you want to install library at other location the you can specify it in PREFIX option in make install command:

      sudo make install PREFIX=/home/user


### Usage in code

See [examples](https://github.com/GSLabDev/libasynckafkaclient/tree/master/examples) for an example producer and consumer.

Link your program with `-lasynckafka -levent`.


## Documentation

The API is documented in [src/KafkaClient.h](src/KafkaClient.h)

Configuration properties are documented in
[CONFIGURATION.md](https://github.com/GSLabDev/libasynckafkaclient/blob/master/CONFIGURATION.md)

For a librdkafka introduction, see
[INTRODUCTION.md](https://github.com/GSLabDev/libasynckafkaclient/blob/master/INTRODUCTION.md)


## Examples

See the [examples](https://github.com/GSLabDev/libasynckafkaclient/tree/master/examples) sub-directory.


## Support

File bug reports, feature requests and questions using
[GitHub Issues](https://github.com/GSLabDev/libasynckafkaclient/issues)
