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

#ifndef AsyncKakfa_Logger_INCLUDED
#define AsyncKakfa_Logger_INCLUDED

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>

#include <string>
// gets rid of annoying "deprecated conversion from string constant blah blah" warning
#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifndef LOG_FILE_NAME
#define LOG_FILE_NAME "kafka-logs.txt"
#endif

#define LOG_FATAL   1
#define LOG_ALERT   2
#define LOG_CRIT    3
#define LOG_ERROR   4
#define LOG_WARN    5
#define LOG_NOTICE  6
#define LOG_INFO    7
#define LOG_DEBUG   8
#define LOG_TRACE   9

static char *priStr[] =  { "UNKNOWN",
                           "FATAL",
                           "ALERT",
                           "CRIT",
                           "ERROR",
                           "WARN",
                           "NOTICE",
                           "INFO",
                           "DEBUG",
                           "TRACE"
                         };


#define FATAL(format, ...)      logMessage (LOG_FATAL, __func__, __LINE__, format, ##__VA_ARGS__)
#define ALERT(format, ...)      logMessage (LOG_ALERT, __func__, __LINE__, format, ##__VA_ARGS__)
#define CRIT(format, ...)       logMessage (LOG_CRIT, __func__, __LINE__, format, ##__VA_ARGS__)
#define ERROR(format, ...)      logMessage (LOG_ERROR, __func__, __LINE__, format, ##__VA_ARGS__)
#define WARN(format, ...)       logMessage (LOG_WARN, __func__, __LINE__, format, ##__VA_ARGS__)
#define NOTICE(format, ...)     logMessage (LOG_NOTICE, __func__, __LINE__, format, ##__VA_ARGS__)
#define INFO(format, ...)       logMessage (LOG_INFO, __func__, __LINE__, format, ##__VA_ARGS__)
#define DEBUG(format, ...)      logMessage (LOG_DEBUG, __func__, __LINE__, format, ##__VA_ARGS__)
#define TRACE(format, ...)      logMessage (LOG_TRACE, __func__, __LINE__, format, ##__VA_ARGS__)


static inline std::string timeStamp()
{
    timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 10;
    char buffer [0xFF] = {0};

    strftime(buffer, 80, "%Y-%b-%d %H:%M:%S", localtime(&curTime.tv_sec));
    char currentTime[0xFF] = {0};
    sprintf(currentTime, "%s.%05d", buffer, milli);

    return currentTime;
}

static bool firstMsg = true;
static inline void logMessage(int a_priority,
                              const char *fun_name,
                              int line_no,
                              const char * a_format,...)
{
    va_list va;
    FILE *fp =  NULL;

    //if (firstMsg) {
    //    fp = fopen(LOG_FILE_NAME, "w");
    //    firstMsg = false;
    //} else {
        fp = fopen(LOG_FILE_NAME, "a+");
    //}

    if (fp == NULL) {
        printf("Not able to open log file\n");
        return;
    }

    std::string ts = timeStamp();
    fprintf(fp, "[ %s ] %s : %s (%d) : ", ts.c_str(), priStr[a_priority], fun_name, line_no);
    va_start(va, a_format);
    vfprintf(fp, a_format, va);
    va_end(va);
    fprintf(fp, "\n");

    fclose(fp);
    return;
}

#endif //AsyncKakfa_Logger_INCLUDED
