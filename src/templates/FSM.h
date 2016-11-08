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

#ifndef FSM_H_
#define FSM_H_

#include "palExec.h"
#include "cometosAssert.h"

namespace cometos {

class FSMEvent {
public:
    enum : uint8_t {
        EMPTY_SIGNAL = 0,
        ENTRY_SIGNAL = 1,
        EXIT_SIGNAL,
        INIT_SIGNAL,
        USER_SIGNAL_START
    };

    FSMEvent(uint8_t signal)
    : signal(signal) {
    }

    FSMEvent()
    : signal(EMPTY_SIGNAL) {
    }

    uint8_t signal;
};

enum : uint8_t {
    FSM_HANDLED,    //< event was handled
    FSM_IGNORED,    //< event was ignored; not used in this implementation
    FSM_TRANSITION, //< event was handled and a state transition occurred
    FSM_SUPER       //< event was not handled by the inner state
};

typedef uint8_t fsmReturnStatus;

/**
 * Template for implementing a finite state machine (FSM).
 */
template<typename C, typename E>
class FSM {
public:
	typedef fsmReturnStatus (C::*state_t)(E& event);

	/**
	 * Created FSM is put into initial state
	 */
	FSM(const state_t& initial) :
			state(initial) {
	}

	/**
	 * Execute the ENTRY event of the initial state.
	 */
	void run() {
	    E entryEvent;
        entryEvent.signal = FSMEvent::ENTRY_SIGNAL;
	    (((C*)this)->*state)(entryEvent);
	}

	void dispatch(E& event) {
        E entryEvent;
        entryEvent.signal = FSMEvent::ENTRY_SIGNAL;
        E exitEvent;
        exitEvent.signal = FSMEvent::EXIT_SIGNAL;

        state_t s = state;
        fsmReturnStatus r = (((C*)this)->*state)(event);

        while (r == FSM_TRANSITION) {
            /* call the exit action from last state */
		    ASSERT((((C*)this)->*s)(exitEvent) != FSM_TRANSITION);
		    s = state;

            /* call entry action of new state */
		    r = (((C*)this)->*state)(entryEvent);
        }
	}

	inline fsmReturnStatus transition(state_t next) {
        palExec_atomicBegin();
		state = next;
        palExec_atomicEnd();
        return FSM_TRANSITION;
	}

	const state_t& getState() {
		return state;
	}

    bool setStateIfIdle(state_t idleState, state_t nextState) {
        palExec_atomicBegin(); 

        if(state != idleState) {
            palExec_atomicEnd();
            return false;
        }

        state = nextState;
        palExec_atomicEnd();
        return true;
    }

private:
	state_t state;
};


}
#endif /* FSM_H_ */
