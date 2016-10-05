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

/**@file this file contains an example how to define new modules
 * and message types on basis of CometOS
 */

/*INCLUDES-------------------------------------------------------------------*/
#include "cometos.h"
#include "palLed.h"
#include "palExec.h"
#include "threadutils.h"
#include <unistd.h>
#include <iostream>

using namespace cometos;


/*PROTOTYPES-----------------------------------------------------------------*/

uint32_t counter=0;

template <class C>
C* allocate() {
    counter++;
    return new C;
}


class MyMessage: public Message {
};

class MySender: public Module {
public:
    OutputGate<MyMessage> gateOut;

    MySender() :
            gateOut(this, "gateOut") {
    }

    void initialize() {

        schedule(allocate<Message>(), &MySender::traffic);
    }

    void traffic(Message* timer) {
         gateOut.send(allocate<MyMessage>());
        schedule(timer, &MySender::traffic, 20);
    }
};

class MyReceiver: public Module {
public:

    InputGate<MyMessage> gateIn;

    MyReceiver() :
            gateIn(this, &MyReceiver::handle, "gateIn") {
    }

    void handle(MyMessage *msg) {
        usleep(1);
        delete msg;
    }
};

// Setup for AVR
MySender sender;
MyReceiver receiver;

void stopAll() {
    cometos::getScheduler().stop();
}

SimpleTask taskStop(stopAll);



void taskFunction();
SimpleTask task(taskFunction);
void taskFunction() {
    cometos::getScheduler().add(task, 50);
    usleep(5);
}




class MyTimer: public Module {
public:
    void scheduleHandle() {
        schedule(allocate<Message>(), &MyTimer::handle1, rand()%10);
    }

    void handle1(Message *msg) {
        usleep(1);
        schedule(msg, &MyTimer::handle2, rand()%10);
    }

    void handle2(Message *msg) {
         delete msg;
     }

};


MyTimer timer;


void threadFunction(thread_handler_t handler, void*) {
    while (!thread_receivedStopSignal(handler)) {
        timer.scheduleHandle();
        usleep(10);
    }
}

int main() {
    sender.gateOut.connectTo(receiver.gateIn);
    cometos::initialize();
    cometos::getScheduler().add(taskStop, 30000);

    std::cout<<"Start Threads"<<std::endl;
    thread_handler_t thread1 = thread_run(threadFunction, NULL);
    thread_handler_t thread2 = thread_run(threadFunction, NULL);

    cometos::getScheduler().add(task, 50);

    cometos::run();

    std::cout<<"Stop Threads"<<std::endl;
    std::cout<<"Allocated "<<counter <<" messages" <<std::endl;
    thread_stopAndClose(thread1);
    thread_stopAndClose(thread2);

    return 0;
}

