# **********************************************************************
#
# Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
#
# <Email: luo (dot) xiaowei (at) hotmail (dot) com>
#
# **********************************************************************

top_srcdir	= ../..


TARGETS		= $(OBJS)

#OBJS		= Cond.o \
#		  CountdownLatch.o \
#		  Mutex.o \
#		  RecMutex.o \
#		  RWRecMutex.o \
#		  Semaphore.o \
#		  SpinMutex.o \
#		  StaticMutex.o \
#		  Thread.o \
#		  ThreadControl.o \
#		  ThreadException.o \
#		  ThreadPool.o \
#		  Timer.o
#
#SRCS		= $(OBJS:.o=.cpp)

SRCS		= $(wildcard *.cpp)

OBJS		= $(SRCS:.cpp=.o)

include $(top_srcdir)/config/Make.rules

CPPFLAGS        := $(CPPFLAGS) $(UTIL_FLAGS) -DTHREADING_API_EXPORTS -I..
LINKWITH        := $(UTIL_OS_LIBS)

all::  $(SRCS) $(TARGETS)
	$(INSTALL) *.o ..

include .depend
