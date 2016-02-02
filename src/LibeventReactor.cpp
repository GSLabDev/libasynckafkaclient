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

#include <assert.h>
#include <iostream>

#include "LibeventReactor.h"
#include "Logger.h"

namespace AsyncKakfa
{

LibeventReactor::LibeventReactor()
    : m_pBase(NULL)
{
}

LibeventReactor::LibeventReactor(struct event_base* base)
    : m_pBase(NULL)
{
	init(base);
}

LibeventReactor::~LibeventReactor()
{
    std::map<int, struct event *>::iterator itr;

    for(itr = m_events.begin(); itr != m_events.end(); ++itr) {
        struct event *ev = itr->second;
        if (ev != NULL) {
            if(event_del(ev) == -1) {
                ERROR("Error in deleting event.");
            }

            event_free(ev);
        }
    }

    m_events.clear();
}

bool LibeventReactor::init(struct event_base* base)
{
    if (base == NULL) {
        ERROR("Event base is NULL. Reactor initialization failed");
        return false;
    }

    m_pBase = base;
    return true;
}

void LibeventReactor::shutdown(int sigNum)
{
    INFO("shutting down the reactor");
    struct timeval delay = { 0, 0 };

    event_base_loopexit(m_pBase, &delay);
}

void LibeventReactor::registerFdCallback(int fd,
                                   AsyncEventSource::EventData *eventData,
                                   int eventFlags,
                                   bool oneshot = false,
                                   bool modify = false)
{
    if(m_pBase == NULL)
    {
        ERROR("Event base is not created.");
        return;
    }

    struct event *ev = NULL;
    short eventFlag = 0;

    if (eventFlags & AEVENTIN)
    {
        eventFlag |= EV_READ;
    }

    if (eventFlags & AEVENTOUT)
    {
        eventFlag |= EV_WRITE;
    }

    if (!oneshot) {
        eventFlag |= EV_PERSIST;
    }

    if(modify)
    {
        unregisterFdCallback(fd);
    }

    ev = event_new(m_pBase, fd, eventFlag, &dispatchEvents, eventData);

    if(ev == NULL)
    {
        ERROR("Error in creating event.");
        return;
    }

    int res = event_add(ev,NULL);
    if(res == -1)
    {
        ERROR("Error in adding  event.");
    }

    assert(res != -1);

    m_events.insert(std::pair<int, struct event *>(fd, ev));

}

void LibeventReactor::unregisterFdCallback(int fd)
{
    struct event *ev = m_events[fd];

    if (ev != NULL) {
        if(event_del(ev) == -1)
        {
            ERROR("Error in deleting  event.");
        }

        event_free(ev);
    }

    m_events.erase(fd);
}


void LibeventReactor::dispatchEvents(int fd, short what, void *arg)
{
    AsyncEventSource::EventData *eventData = static_cast<AsyncEventSource::EventData *>(arg);

    short event = 0;
    if (what & EV_READ) {
        event |= AEVENTIN;
    }
    if (what & EV_WRITE) {
        event |= AEVENTOUT;
    }

    if (eventData->cb) {
        eventData->cb(fd, event, eventData);
    }

}

void LibeventReactor::handle_events()
{
    INFO("handle_events()");
}

} //namespace AsyncKakfa


