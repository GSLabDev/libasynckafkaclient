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

#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <iostream>

#include "Logger.h"
#include "WakeupEventSource.h"

namespace AsyncKakfa
{

WakeupEventSource::
WakeupEventSource()
    : m_iWakeupFd(-1),
      m_iEventFlags(AEVENTIN),
      m_funOnWakeup(NULL)

{
}

WakeupEventSource::
WakeupEventSource(std::function<void (uint64_t)> onWakeup)
    : m_iWakeupFd(-1),
      m_iEventFlags(AEVENTIN),
      m_funOnWakeup(onWakeup)
{
    init(onWakeup);
}

WakeupEventSource::
~WakeupEventSource()
{
    int res = close(m_iWakeupFd);
    if (res == -1) {
        WARN("close() on wakefd failed with errno: %d (%s)", errno, strerror(errno));
    }
}

void
WakeupEventSource::init(std::function<void (uint64_t)> onWakeup)
{
    if(m_iWakeupFd != -1) {
        WARN("double initialization of wakeup event source");
    }

    this->m_funOnWakeup = onWakeup;
    m_iWakeupFd = eventfd(0, EFD_NONBLOCK);
    if (m_iWakeupFd == -1) {
        ERROR("wakefd creation failed with errno = %d (%s)", errno, strerror(errno));
    }
    assert(m_iWakeupFd != -1);

    m_eventData.fd = m_iWakeupFd;
    m_eventData.cb = std::bind(&WakeupEventSource::handleEvent,
                               this,
                               std::placeholders::_1,
                               std::placeholders::_2,
                               std::placeholders::_3);
    m_eventData.edata = NULL;
}

void
WakeupEventSource::wakeup()
{
    uint64_t value = 1;

    int res = write(m_iWakeupFd, &value, sizeof(uint64_t));
    if (res == -1) {
        ERROR("WakeupEventSource::wakeup() write failed with errno: %d (%s)", errno, strerror(errno));
    }
    assert(res != -1);
    if (res != sizeof(uint64_t)) {
        ERROR("WakeupEventSource::wakeup() write failed: Not able to write %d bytes", sizeof(uint64_t));
    }

}

void
WakeupEventSource::handleEvent(const int fd, const short event, void * arg)
{
    if(event & AEVENTIN) {
        for(;;) {
            uint64_t numWakeups = 0;
            int res = read(m_iWakeupFd, &numWakeups, 8);
            if (res == -1 && errno == EINTR)
                continue;
            if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
            if (res == -1) {
                ERROR("read() failed on wakeupFd with errno: %d (%s)", errno, strerror(errno));
            }
            assert(res != -1);

            if (res != 8) {
                ERROR("wrong number of byte read: %d", res);
                break;
            }

            if(m_funOnWakeup) {
                m_funOnWakeup(numWakeups);
                break;
            }
        }
    }
}

int
WakeupEventSource::selectFd() const
{
    return m_iWakeupFd;
}

int
WakeupEventSource::getEventFlags() const
{
    return m_iEventFlags;
}

WakeupEventSource::EventData *
WakeupEventSource::eventData()
{
    return &m_eventData;
}

} //namespace AsyncKakfa

