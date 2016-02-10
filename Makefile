# Temnos configuration makefile
CXX ?= /usr/bin/g++
CC ?= /usr/bin/gcc

PREFIX ?= /usr/local
DST_LIB ?= $(PREFIX)/lib
DST_INCLUDE ?= $(PREFIX)/include
DST_BIN ?= $(PREFIX)/bin

ROOTDIR := $(shell pwd)
LOCAL_INCLUDE_DIR?=$(ROOTDIR)

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
	@cp -rf $(LOCAL_INCLUDE_DIR)/libkafka-asio/lib/libkafka_asio $(DST_INCLUDE)

clean:
	for dir in $(SRC_DIRS); do \
		$(MAKE) -C $$dir $@; \
	done

distclean: clean

