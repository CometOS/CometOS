/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @author Stefan Unterschuetz
 */
#include "palExec.h"
#include <stdlib.h>
#include "palLocalTime.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include "cometosAssert.h"


#ifdef _WIN32
#include <Windows.h>
#include <mmsystem.h>

#else
#include <pthread.h>
#endif

#ifdef _WIN32
#define LOCK() WaitForSingleObject( lock, INFINITE )
#define UNLOCK() ReleaseMutex( lock )

#else

#define TIME_VAL_ADD_MS(tp,offset) 	tp.tv_usec+=offset*1000; tp.tv_sec+=tp.tv_usec/(1000000);tp.tv_usec=tp.tv_usec%(1000000)
#define TIME_VAL_TO_SPEC(tp,ts) ts.tv_sec = tp.tv_sec;	ts.tv_nsec = tp.tv_usec * 1000
#define LOCK()	pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#define LOCK2()	pthread_mutex_lock(&lock2)
#define UNLOCK2() pthread_mutex_unlock(&lock2)

#endif

static bool stayAwake;

#ifdef _WIN32

static HANDLE lock;
static HANDLE hEvent;

#else
bool isSleeping;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
#endif

void palExec_wakeup() {
//    ASSERT(false); // this should not be called in the simulation // WHY?
#ifdef _WIN32
	stayAwake = true;
	SetEvent(hEvent);
#else
	LOCK2();
	if (isSleeping == false) {
		UNLOCK2();
		return;
	}
	stayAwake = true;

	pthread_cond_signal(&cond);
	UNLOCK2();
#endif
}

time_ms_t getOffset() {
//    ASSERT(false); // this should not be called in the simulation
#ifdef _WIN32
	static ULARGE_INTEGER oldVal;

	FILETIME newTime;
	GetSystemTimeAsFileTime(&newTime);
	ULARGE_INTEGER newVal;
	newVal.HighPart = newTime.dwHighDateTime;
	newVal.LowPart = newTime.dwLowDateTime;

	// get offset in milliseconds
	uint64_t offset = (newVal.QuadPart - oldVal.QuadPart);

	oldVal.QuadPart = newVal.QuadPart - (offset % 10000);// remember this part

	return (offset / 10000);
#else
	static long remaining = 0;
	static struct timeval oldVal;
	struct timeval newVal;
	gettimeofday(&newVal, NULL);
	long elapsedTime = (newVal.tv_sec - oldVal.tv_sec) * 1000000;
	elapsedTime += (newVal.tv_usec - oldVal.tv_usec) + remaining;
	long offset = elapsedTime / 1000;
	remaining = elapsedTime - (offset * 1000); // remember remaining us for next call
	oldVal = newVal;
	return offset;
#endif
}

void palExec_init() {
#ifdef _WIN32
#ifndef NO_TIME_RESOLUTION_ADAPTION
    timeBeginPeriod(1);
#endif
	lock = CreateMutex(NULL, false, NULL);
	hEvent = CreateEvent(NULL, false, false, NULL);
#endif
	getOffset();
	stayAwake = true;

}

time_ms_t palExec_elapsed() {
//    ASSERT(false); // this should not be called in the simulation
	return getOffset();
}

void palExec_sleep(time_ms_t ms) {
//    ASSERT(false); // this should not be called in the simulation
	if (stayAwake || ms == 0) {
		stayAwake = false;
		return;
	}

#ifdef _WIN32
	WaitForSingleObject(hEvent, ms);
#else
	// get target time
	struct timespec ts;
	struct timeval tp;
	gettimeofday(&tp, NULL);
	TIME_VAL_ADD_MS(tp, ms);
	TIME_VAL_TO_SPEC(tp, ts);

	LOCK2();
	isSleeping = true;
	pthread_cond_timedwait(&cond, &lock2, &ts);
	isSleeping = false;
	UNLOCK2();
#endif
	return;

}

void palExec_atomicBegin() {
	LOCK();
}

void palExec_atomicEnd() {
	UNLOCK();
}

uint8_t palExec_getResetReason () {
    return 0;
}

