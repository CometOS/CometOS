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
 * For the CometOS base station and OMNeT++ implementation we need basic
 * thread functionality (e.g. for TCP server).
 *
 *
 * @author Stefan Unterschuetz
 */
#ifndef THREAD_UTILS_H_
#define THREAD_UTILS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* thread_mutex_t;
typedef void* thread_handler_t;
typedef void (*thread_function_t)(thread_handler_t, void*);

/**Runs passed function in thread.
 */
thread_handler_t thread_run(thread_function_t function, void* parameter);

/**Checks whether stop signal is set for thread. A thread should periodically
 * check whether stop signal in order to terminate correctly.
 */
bool thread_receivedStopSignal(thread_handler_t thread);

/**Sets stop signal of thread and waits until it finishes. Afterwards,
 * thread object is destroyed. Never call this inside the same thread.
 */
void thread_stopAndClose(thread_handler_t thread);

/**Only free handler, does not wait for thread to finish*/
void thread_close(thread_handler_t thread);

/**Creates mutex object
 */
thread_mutex_t thread_createMutex();

/**Locks a mutex. Wait if mutex is already locked. */
void thread_lockMutex(thread_mutex_t mutex);

/**Release mutex object*/
void thread_unlockMutex(thread_mutex_t mutex);

/**Destroy mutex
 */
void thread_destroyMutex(thread_mutex_t mutex);

#ifdef __cplusplus
}
#endif

#endif /* THREAD_UTILS_H_ */
