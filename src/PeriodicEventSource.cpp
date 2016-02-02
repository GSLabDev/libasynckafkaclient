/*
 * libasynckafka - Apache Kafka C++ Async Client library
 *
 * Copyright (c) 2016, Ganesh Nikam, Great Software Laboratory Pvt Ltd
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <string.h>
#include <assert.h>
#include <iostream>

#include "Logger.h"
#include "PeriodicEventSource.h"

namespace AsyncKakfa
{

PeriodicEventSource::
PeriodicEventSource()
    : m_iTimerFd(-1),
      m_iEventFlags(AEVENTIN),
      m_dTimePeriodSeconds(0),
      m_funOnTimeout(NULL)

{
}

PeriodicEventSource::
PeriodicEventSource(double timePeriodSeconds,
                    std::function<void (uint64_t)> onTimeout)
    : m_iTimerFd(-1),
      m_iEventFlags(AEVENTIN),
      m_dTimePeriodSeconds(timePeriodSeconds),
      m_funOnTimeout(onTimeout)
{
    init(timePeriodSeconds, onTimeout);
}

PeriodicEventSource::
~PeriodicEventSource()
{
    int res = close(m_iTimerFd);
    if (res == -1) {
        WARN("close() on timerfd failed with errno: %d (%s)", errno, strerror(errno));
    }
}

void
PeriodicEventSource::init(double timePeriodSeconds,
                          std::function<void (uint64_t)> onTimeout)
{
    if(m_iTimerFd != -1) {
        WARN("double initialization of periodic event source");
    }

    this->m_dTimePeriodSeconds = timePeriodSeconds;
    this->m_funOnTimeout = onTimeout;

    m_iTimerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (m_iTimerFd == -1) {
        ERROR("timerfd_create() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(m_iTimerFd != -1);

    itimerspec spec;

    uint64_t seconds, nanoseconds;
    seconds = timePeriodSeconds;
    nanoseconds = (timePeriodSeconds - seconds) * 1000000000;

    spec.it_interval.tv_sec = spec.it_value.tv_sec = seconds;
    spec.it_interval.tv_nsec = spec.it_value.tv_nsec = nanoseconds;

    int res = timerfd_settime(m_iTimerFd, 0, &spec, 0);
    if (res == -1) {
        ERROR("timerfd_settime() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);

    m_eventData.fd = m_iTimerFd;
    m_eventData.cb = std::bind(&PeriodicEventSource::handleEvent,
                               this,
                               std::placeholders::_1,
                               std::placeholders::_2,
                               std::placeholders::_3);
    m_eventData.edata = NULL;
}

void
PeriodicEventSource::update_timer(double timePeriodSeconds)
{
    struct itimerspec spec;
    struct itimerspec old_val;
    uint64_t seconds = 0, nanoseconds = 0;

    //DEBUG("timePeriodSeconds = %f", timePeriodSeconds);
    seconds = timePeriodSeconds;
    double nsec = timePeriodSeconds - seconds;
    nsec = nsec * 1000000000;
    nanoseconds = nsec;

    memset(&spec, 0, sizeof(struct itimerspec));
    spec.it_interval.tv_sec = spec.it_value.tv_sec = seconds;
    spec.it_interval.tv_nsec = spec.it_value.tv_nsec = nanoseconds;


    int res = timerfd_settime(m_iTimerFd, 0, &spec, &old_val);
    if (res == -1) {
        ERROR("timerfd_settime() failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);
}
void
PeriodicEventSource::handleEvent(const int fd, const short event, void * arg)
{
    if(event & AEVENTIN) {
        for(;;) {
            uint64_t numWakeups = 0;
            int res = read(m_iTimerFd, &numWakeups, 8);
            if (res == -1 && errno == EINTR) {
                continue;
            }
            if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
            if (res == -1) {
                ERROR("read() failed on timerFd with errno: %d (%s)", errno, strerror(errno));
            }
            assert(res != -1);

            if (res != 8) {
                ERROR("wrong number of byte read: %d",res);
                break;
            }

            if(m_funOnTimeout) {
                m_funOnTimeout(numWakeups);
                break;
            }
        }
    }
}

int
PeriodicEventSource::selectFd() const
{
    return m_iTimerFd;
}

PeriodicEventSource::EventData *
PeriodicEventSource::eventData()
{
    return &m_eventData;
}

int
PeriodicEventSource::getEventFlags() const
{
    return m_iEventFlags;
}
} //namespace AsyncKakfa

