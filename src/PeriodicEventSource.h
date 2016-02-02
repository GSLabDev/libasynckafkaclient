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

#ifndef AsyncKakfa_PeriodicEventSource_INCLUDED
#define AsyncKakfa_PeriodicEventSource_INCLUDED

#include "AsyncEventSource.h"

namespace AsyncKakfa
{

class PeriodicEventSource: public AsyncEventSource
{
public:
    PeriodicEventSource();
    PeriodicEventSource(double timePeriodSeconds,
                        std::function<void (uint64_t)> onTimeout);

    virtual ~PeriodicEventSource();
    void init(double timePeriodSeconds,
              std::function<void (uint64_t)> onTimeout);
    void update_timer(double timePeriodSeconds);
    virtual int selectFd() const;
    virtual EventData * eventData ();
    virtual int getEventFlags() const;

private:
    void handleEvent(const int fd, const short event, void *arg);
    EventData m_eventData;
    int32_t m_iTimerFd;
    int32_t m_iEventFlags;
    double m_dTimePeriodSeconds;
    std::function<void (uint64_t)> m_funOnTimeout;
};

} //namespace AsyncKakfa

#endif // AsyncKakfa_PeriodicEventSource_INCLUDED
