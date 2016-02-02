# Temnos configuration makefile
CXX ?= /usr/bin/g++
CC ?= /usr/bin/gcc

PREFIX ?= /usr/local
DST_LIB ?= $(PREFIX)/lib
DST_INCLUDE ?= $(PREFIX)/include
DST_BIN ?= $(PREFIX)/bin

ROOTDIR=$(PWD)
LOCAL_INCLUDE_DIR?=$(ROOTDIR)/libkafka-asio/lib

WARN ?= -Wall -W
FPIC ?= -fPIC
SHARED ?= -shared

CXXFLAGS ?= -g
#CXXFLAGS += $(WARN)


LDFLAGS ?= -g
export

SRC_DIRS = src examples
INSTALL_DIRS = src

.PHONY: all lib install clean distclean

all:
	for dir in $(SRC_DIRS); do \
		$(MAKE) -C $$dir $@; \
	done


install:
	for dir in $(INSTALL_DIRS); do \
		$(MAKE) -C $$dir $@; \
	done

clean:
	for dir in $(SRC_DIRS); do \
		$(MAKE) -C $$dir $@; \
	done

distclean: clean

