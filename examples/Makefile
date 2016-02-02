# Temnos configuration mkefile

CXXFLAGS += -std=c++11
CXXFLAGS += -I ../libkafka-asio/lib
CXXFLAGS += -I ../src

LDFLAGS += -L ../src

LIBLINK = -lasynckafka -levent

ALLSRCS = consumer-00.cpp \
		  producer-00.cpp \
		  performance-00.cpp \
		  performance-01.cpp

ALLOBJS = $(ALLSRCS:.cpp=)
TARGETS= $(foreach obj,$(ALLSRCS), $(subst .cpp,,$(obj)))

.PHONY: all clean distclean

all: $(TARGETS)

consumer-00:
	$(CXX) -o $@  consumer-00.cpp  $(LIBLINK) $(CXXFLAGS) $(LDFLAGS)

producer-00:
	$(CXX) -o $@  producer-00.cpp  $(LIBLINK) $(CXXFLAGS) $(LDFLAGS)

performance-00:
	$(CXX) -o $@  performance-00.cpp  $(LIBLINK) $(CXXFLAGS) $(LDFLAGS)

performance-01:
	$(CXX) -o $@  performance-01.cpp  $(LIBLINK) $(CXXFLAGS) $(LDFLAGS)

clean:
	@rm -f $(TARGETS)

distclean: clean

