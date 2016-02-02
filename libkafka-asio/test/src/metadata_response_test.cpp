//
// metadata_response_test.cpp
// --------------------------
//
// Copyright (c) 2015 Daniel Joos
//
// Distributed under MIT license. (See file LICENSE)
//

#include <gtest/gtest.h>
#include <libkafka_asio/libkafka_asio.h>

using namespace libkafka_asio;

class MetadataResponseTest :
  public ::testing::Test
{
protected:
  void AddBroker(const String& host, Int32 node_id, Int32 port)
  {
    MetadataResponse::Broker broker;
    broker.host = host;
    broker.node_id = node_id;
    broker.port = port;
    response.mutable_brokers().push_back(broker);
  }

  MutableMetadataResponse response;
};


TEST_F(MetadataResponseTest, PartitionLeader)
{
  AddBroker("localhost", 123, 49152);
  AddBroker("example.com", 456, 49152);
  ASSERT_EQ(2, response.response().brokers().size());
  MetadataResponse::Topic metadata;
  metadata.topic_name = "foo";
  metadata.partitions.resize(1);
  metadata.partitions[0].partition = 1;
  metadata.partitions[0].leader = 456;
  response.mutable_topics().push_back(metadata);
  ASSERT_EQ(1, response.response().topics().size());

  MetadataResponse::Broker::OptionalType leader =
    response.response().PartitionLeader("foo", 1);
  ASSERT_TRUE(leader);
  ASSERT_EQ(456, leader->node_id);
  ASSERT_STREQ("example.com", leader->host.c_str());
}

TEST_F(MetadataResponseTest, PartitionLeader_InElection)
{
  MetadataResponse::Topic metadata;
  metadata.topic_name = "foo";
  metadata.partitions.resize(1);
  metadata.partitions[0].partition = 1;
  metadata.partitions[0].leader = -1;
  response.mutable_topics().push_back(metadata);
  ASSERT_EQ(1, response.response().topics().size());

  MetadataResponse::Broker::OptionalType leader =
    response.response().PartitionLeader("foo", 1);
  ASSERT_FALSE(leader);
}
