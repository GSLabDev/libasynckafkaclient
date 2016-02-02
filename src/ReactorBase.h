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

#ifndef AsyncKakfa_ReactorBase_INCLUDED
#define AsyncKakfa_ReactorBase_INCLUDED

#include "AsyncEventSource.h"

namespace AsyncKakfa
{

/* NOTE:
    With current implementation, the reactor class and all the AsyncEventSources are tightly coupled
    with "epoll". But this is not the generic/ideal design. The ideal design should be as below:
        1. There should be base class for the reactor which will define common functionality APIs.
        2. We should derive Epoll, Poll and Select Reactor class from this base class
        3. Right now the AsyncEventSources are also depend on epoll. We should remove this dependancy
           to have the generic event sources which can be registered with any reactor (epoll, select or poll
        4. This could be designed as below:
            a. Reactor base class should define the enum for the events to registed, something like
                    REACTOR_IN (corrosponds to EPOLLIN)
                    REACTOR_OUT
                    REACTOR_HUNGUP
                    ....
                    ....
                    etc
            b. Particular derived reactor will convert this flag to the corrosponding flags for it and register
               the event for that flag.
            b. Reactor/AsyncEventSource base class should define the structure to save all the event callback and
               any required data for that event. something like:
                    struct event_data {
                        onReadCallback onRead;
                        onWriteCallback onWrite;
                        onClosedCallback onClose;
                        onDisconnectCallback onDisconnect;
                        void *data;
                    }
            c. EventSource should fill this structure with proper handlers for each event and expose it through get
               method.
            d. While registring the event Reactor should save this structure against that event
            e. When the event will come Reactor should call one of the call back method for that eventSource

    Right now this generic design is not implemented because of time constriant. But in future we should implement
    above design
*/

class ReactorBase
{
public:
    ReactorBase() {}
    ~ReactorBase() {}
    virtual void registerFdCallback(int fd, AsyncEventSource::EventData * eventData, int eventFlags, bool oneshot, bool modify) = 0;
    virtual void unregisterFdCallback(int fd) = 0;
    virtual void handle_events() = 0;
    virtual void shutdown(int sigNum) = 0;
};

} //namespace AsyncKakfa

#endif // AsyncKakfa_ReactorBase_INCLUDED
