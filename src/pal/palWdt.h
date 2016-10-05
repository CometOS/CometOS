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
 * @author Andreas Weigel
 */
#ifndef PAL_WDT_H_
#define PAL_WDT_H_

#include <stdint.h>
#include <stdbool.h>

typedef void (*palWdt_callback)();

/**
 * Enable a watchdog timer with given timeout interval and optionally a
 * callback function
 *
 * @param timeout  desired timeout in milli seconds. as most platforms will
 *                 not support arbitrary timeouts, the next lowest will be chosen
 * @param cb       optional callback function pointer; NULL deactivates the callback
 * @return         the actual timeout interval set by the platform
 */
uint16_t palWdt_enable(uint16_t timeout, palWdt_callback cb);


/**
 * Check whether the watchdog is currently running.
 *
 * @return true if watchdog is running, false else
 */
bool palWdt_isRunning();

/**
 * Resets the watchdog timer. Needs to be called regularly to prevent watchdog
 * from firing.
 */
void palWdt_reset();

/**
 * Disable the watchdog temporarily.
 */
void palWdt_pause();
void palWdt_resume();

#endif
