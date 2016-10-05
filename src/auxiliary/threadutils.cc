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

#include "threadutils.h"
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>

typedef struct {
	bool stopSignal;
	HANDLE hthread;
	thread_function_t function;
	void* parameter;
} _thread_handler_t;

typedef struct {
	HANDLE mutex;
} _thread_mutex_t;

#else

#include <unistd.h>
#include <pthread.h>

typedef struct {
	bool stopSignal;
	pthread_t hthread;
	thread_function_t function;
	void* parameter;
}_thread_handler_t;

typedef struct {
	pthread_mutex_t mutex;
}_thread_mutex_t;

#endif

#ifdef _WIN32
static DWORD WINAPI ThreadFunction(LPVOID lpParam) {
	_thread_handler_t *handler = (_thread_handler_t*) lpParam;
	handler->function((thread_handler_t) handler, handler->parameter);
	return 0;
}

#else
static void* ThreadFunction(void* lpParam) {
	_thread_handler_t *handler = (_thread_handler_t*) lpParam;
	handler->function((thread_handler_t) handler, handler->parameter);
	return 0;
}
#endif

thread_handler_t thread_run(thread_function_t function, void* parameter) {

	_thread_handler_t *handler = (_thread_handler_t*) malloc(
			sizeof(_thread_handler_t));

	handler->stopSignal = false;
	handler->parameter = parameter;
	handler->function = function;

#ifdef _WIN32
	handler->hthread = CreateThread(NULL, // default security attributes
			0,// use default stack size
			ThreadFunction,// thread function name
			handler,// argument to thread function
			0,// use default creation flags
			NULL);// returns the thread identifier
#else
	pthread_create(&handler->hthread, NULL, ThreadFunction, (void*) handler);
#endif

	return (thread_handler_t) handler;
}

bool thread_receivedStopSignal(thread_handler_t thread) {
	return ((_thread_handler_t*) thread)->stopSignal;
}

void thread_close(thread_handler_t thread) {
	_thread_handler_t* handler = (_thread_handler_t*) thread;
#ifdef _WIN32
	CloseHandle(handler->hthread);
#endif
	free(handler);
}

void thread_stopAndClose(thread_handler_t thread) {
	_thread_handler_t* handler = (_thread_handler_t*) thread;
	handler->stopSignal = true;
#ifdef _WIN32
	WaitForSingleObject(handler->hthread, INFINITE);
	CloseHandle(handler->hthread);
#else
	pthread_join(handler->hthread, NULL);
#endif

	free(handler);
}

thread_mutex_t thread_createMutex() {
	_thread_mutex_t *handler = (_thread_mutex_t*) malloc(
			sizeof(_thread_mutex_t));
#ifdef _WIN32
	handler->mutex = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&handler->mutex, NULL);
#endif
	return (thread_mutex_t) handler;
}

void thread_lockMutex(thread_mutex_t mutex) {
	_thread_mutex_t* handler = (_thread_mutex_t*) mutex;
#ifdef _WIN32
	WaitForSingleObject(handler->mutex, INFINITE);
#else
	pthread_mutex_lock(&handler->mutex);

#endif
}

void thread_unlockMutex(thread_mutex_t mutex) {
	_thread_mutex_t* handler = (_thread_mutex_t*) mutex;
#ifdef _WIN32
	ReleaseMutex(handler->mutex);
#else
	pthread_mutex_unlock(&handler->mutex);
#endif
}

void thread_destroyMutex(thread_mutex_t mutex) {
	_thread_mutex_t* handler = (_thread_mutex_t*) mutex;
#ifdef _WIN32
	CloseHandle(handler->mutex);
#else
	pthread_mutex_destroy(&handler->mutex);
#endif
	free(handler);
}

