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

#ifndef AsyncKakfa_AsyncEventSource_INCLUDED
#define AsyncKakfa_AsyncEventSource_INCLUDED

#include <functional>

namespace AsyncKakfa
{

enum AsyncEventFlag {
    AEVENTIN        = 0x0001,
    AEVENTOUT       = 0x0002,
    AEVENTRDHUP     = 0x0004,
    AEVENTPRI       = 0x0008,
    AEVENTERR       = 0x0010,
    AEVENTHUP       = 0x0020,
    AEVENTET        = 0x0040
};

class AsyncEventSource
{
public:
    AsyncEventSource() {}
    virtual ~AsyncEventSource() {}
    virtual int selectFd() const
    {
        /* derived class should override this function */
        return -1;
    }

    /* type of callback invoked whenever an epoll event is reported for a
     * file descriptor */
    typedef std::function<void (const int, const short, void*)> EventCallback;

    typedef struct _EventData
    {
        _EventData() {}
        _EventData(const _EventData & other)
        {
            fd = other.fd;
            cb = other.cb;
            edata = other.edata;
        }
        int           fd;
        EventCallback cb;
        void *        edata;
    }EventData;

    virtual EventData * eventData () = 0;
    virtual int getEventFlags() const
    {
        /* derived class should override this function */
        return (AEVENTIN | AEVENTOUT);
    }

};

} //namespace AsyncKakfa

#endif // AsyncKakfa_AsyncEventSource_INCLUDED
