/*
 * Copyright (c) 2007, Vanderbilt University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Miklos Maroti
 *         Andreas Huber <huberan@ee.ethz.ch>
 * Author: Andreas Weigel (modifications for CometOS)
 */


#ifndef MESSAGEBUFFERLAYER_H_
#define MESSAGEBUFFERLAYER_H_


#include "tosMsgReplace.h"
#include "mac_definitions.h"
#include "cometosError.h"
#include "mac_interface.h"


/* ------------------ COMMANDS ----------------------------------- */
mac_result_t mblSend_send(message_t *msg);

mac_result_t mblSend_cancel(message_t *msg);

mac_result_t mbl_init();

void tasklet_messageBufferLayer_run();

/* ------------------ EVENTS ------------------------------------- */
message_t* mblReceive_receive(message_t* msg);

void mblSend_sendDone(message_t * msg, mac_result_t error);

#endif /* MESSAGEBUFFERLAYER_H_ */
