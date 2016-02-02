# Temnos configuration makefile

CXXFLAGS += -std=c++11
CXXFLAGS += $(FPIC)
CXXFLAGS += $(foreach includedir,$(LOCAL_INCLUDE_DIR),-I$(includedir))

LDFLAGS += $(SHARED)

LIBLINK = -lrt -lboost_system -levent

ALLHEADERS = PeriodicEventSource.h \
		     WakeupEventSource.h \
             KafkaEndpointMP.h \
             KafkaClient.h \
             LibeventReactor.h \
             ReactorBase.h

ALLSRCS = PeriodicEventSource.cpp \
		  WakeupEventSource.cpp \
          KafkaEndpointMP.cpp \
          KafkaClient.cpp \
          LibeventReactor.cpp

ALLOBJS = $(ALLSRCS:.cpp=.o)

TARGET = libasynckafka.so

.PHONY: all lib install clean distclean

all: lib

lib: $(TARGET)

$(TARGET): $(ALLOBJS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $(ALLOBJS) $(LIBLINK)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

install:
	install -d $(DST_LIB)
	install -d $(DST_INCLUDE)
	install $(TARGET) $(DST_LIB)
	@cp -rf $(LOCAL_INCLUDE_DIR)/libkafka_asio $(DST_INCLUDE)
	install -D -m 644 $(ALLHEADERS) $(DST_INCLUDE)

clean:
	@rm -f $(TARGET)
	@rm -f $(ALLOBJS)

distclean: clean

