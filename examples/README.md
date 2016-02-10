libasynckafka Examples:
==============================================

  * producer-00         : Basic producer functionality

    Command line options for this example:

                # ./producer-00 -h
                #Usage:
                #-S <server ip>
                #-P <port number>
                #-t <topic>
                #-p <partition ID (-1 for all partitions)>
                #-b <batch length (# of messages)>
                #-a <# of required acks(1 = leader only, -1 = All ISR, N = N # of ISRs)>
                #-h help

    **Note** : This program internally generates the messages continuously produces these
               messages on Kafka server. To stop the producer press "CTL + c" .

    Example run command:
                # ./producer-00 -S 127.0.0.1 -P 9092 -t TEST-2 -p 1 -b 1 -a -1
                #Messages produced on server. The last offset is: 50080
                #Messages produced on server. The last offset is: 50084
                #Messages produced on server. The last offset is: 50088
                #Messages produced on server. The last offset is: 50092
                #Messages produced on server. The last offset is: 50096
                #
                #

  * consumer-00         : Basic consumer functionality

    Command line options for this example:

                # ./consumer-00 -h
                #Usage:
                #-S <server ip>
                #-P <port number>
                #-t <topic>
                #-p <partition ID (-1 for all partitions)>
                #-h help

    **Note** : This program will continuously consume the messages. To stop the consumer press "CTL + c" .

    Example run command:
                # ./consumer-00 -S 127.0.0.1 -P 9092 -t TEST-2 -p 1
                #The mssage number is : 1
                #The mssage number is : 2
                #The mssage number is : 3
                #The mssage number is : 4
                #The mssage number is : 5
                #The mssage number is : 6
                #The mssage number is : 7
                #The mssage number is : 8
                #The mssage number is : 9
                #The mssage number is : 10
                #
                #

  * performance-00      : Performance test core for Producer

    Command line options for this example:

                # ./performance-00 -h
                #Usage:
                #-S <server ip>
                #-P <port number>
                #-t <topic>
                #-p <partition ID (-1 for all partitions)>
                #-b <batch length (# of messages)>
                #-a <# of required acks(1 = leader only, -1 = All ISR, N = N # of ISRs)>
                #-s <size of the message (in bytes)>
                #-c <# of messages>
                #-h help

    Example run command:
                # ./performance-00 -S 127.0.0.1 -P 9092 -t TEST-1 -p 1 -b 100 -a -1 -s 100 -c 500000
                #
                #   500000 messages produced in :  9019 ms
                #   55438.00 msgs/s
                #   5.29 Mb/s
                #

  * performance-01      : Performance test case for Consumer

    Command line options for this example:

                # ./performance-01 -h
                #Usage:
                #-S <server ip>
                #-P <port number>
                #-t <topic>
                #-p <partition ID (-1 for all partitions)>
                #-o <start offset (beginning/stored/end)>
                #-m <Max fetch bytes>
                #-c <# of messages to consume>
                #-h help

    Example run command:
                # ./performance-01 -S 127.0.0.1 -P 9092 -t TEST-1 -p 1 -o beginning -m 16384 -c 500000
                #
                #   500000 messages consumed in :  7243 ms
                #   69032.00 msgs/s
                #   6.58 Mb/s
                #

