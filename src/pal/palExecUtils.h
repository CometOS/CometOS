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

#ifndef PALEXECUTILS_H_
#define PALEXECUTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include "cometosError.h"

/**
 * Returns the reason for the reset of the device
 */
uint8_t palExec_getResetReason();

uint32_t palExec_getPcAtReset();

bool palExec_writePc(uint8_t * pc);

void palExec_clearResetStatus();

bool palExec_readAssertInfoShort(uint16_t & line, uint16_t & fileId);

bool palExec_readAssertInfoLong(uint16_t & line, char * filename, uint8_t fileLen);

bool palExec_writeAssertInfoShort(uint16_t line, uint16_t fileId);

bool palExec_writeAssertInfoLong(uint16_t line, const char * filename);

bool palExec_clearAssertInfo();

void palExec_badisr();

uint32_t palExec_getStackSize();

/**
 * In case of no error, this function always resets the node.
 */
cometos_error_t palExec_reset();

#endif /* PALEXECUTILS_H_ */
