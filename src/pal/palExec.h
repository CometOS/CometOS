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
 * CometOS Platform Abstraction Layer for Program Execution
 *
 * @author Stefan Unterschuetz
 */

#ifndef PALEXEC_H_
#define PALEXEC_H_

#include "types.h"

/**
 * Initializes module.
 */
void palExec_init();


/**
 * @return elapsed time in milliseconds since last call
 *
 * Note that this must not be called during palExec_sleep(). This allows a more
 * energy efficient sleep implementation.
 *
 */
time_ms_t palExec_elapsed();


/**
 * Blocks the execution for the  given number of milliseconds.
 * The occurrence of asynchronous events may still be possible.
 * A early return of this function is allowed. For this reasons. the
 * actual sleeping  time should always be measured with the
 * function @see palExec_elapsed().
 *
 * @param ms	time to sleep in milliseconds. In case of 0 the
 * 				function should return immediately.
 *
 */
void palExec_sleep(time_ms_t ms);


/**
 * Aborts an executing call of @see palExec_sleep().
 * Important: A call of palExec_wakeup() will also abort
 * a possible following call of palExec_sleep()!
 */
void palExec_wakeup();


/**
 * Enables atomicity. Atomic blocks must not be interrupted
 * by asynchronous events.
 */
void  palExec_atomicBegin();


/**
 * Disables atomicity.
 */
void  palExec_atomicEnd();

#endif /* PALEXEC_H_ */
