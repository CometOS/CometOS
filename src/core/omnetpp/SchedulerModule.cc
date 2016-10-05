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
 * @author Florian Meier
 */

#include "TaskScheduler.h"
#include "omnetpp.h"
#include "TaskBase.h"
#include "palExec.h"

using namespace omnetpp;

namespace cometos {

class SchedulerModule : public TaskScheduler, public cSimpleModule {
private:
    cMessage msg;
    simtime_t lastTime;

public:
    void initialize()
    {
        lastTime = simTime();

        // scheduler will be started by the first add
    }

    void handleMessage(cMessage* msg)
    {
        setupNextTask();

        if(currTask != NULL) {
//            std::cout << "context before: " << (cSimulation::getActiveSimulation())->getContextModule()->getFullPath();
            (cSimulation::getActiveSimulation())->setContext(currTask->context);
//            std::cout << " context during: " << (cSimulation::getActiveSimulation())->getContextModule()->getFullPath();
            currTask->invoke();
            currTask=NULL;
            (cSimulation::getActiveSimulation())->setContext(this);
//            std::cout << " context after: " << (cSimulation::getActiveSimulation())->getContextModule()->getFullPath() << std::endl;
        }

        // updateExpiration after each task execution or sleep period
        updateExpiration();

        palExec_atomicBegin();
        time_ms_t sleep = getTimeUntilNextExecution();
        if(sleep != (time_ms_t)(-1)) {
            cancelEvent(msg);
            scheduleAt((cSimulation::getActiveSimulation())->getSimTime()+SimTime(sleep,SIMTIME_MS), msg);
        }
        palExec_atomicEnd();
    }

    virtual void wakeup()
    {
        Enter_Method_Silent();
        palExec_atomicBegin();
        cancelEvent(&msg);
        scheduleAt(simTime(), &msg);
        palExec_atomicEnd();
    }

    virtual time_ms_t elapsedMS()
    {
        simtime_t now = simTime();
        simtime_t diff = lastTime-now;
        lastTime = now;
        return diff.inUnit(SIMTIME_MS);
    }

    virtual void add_unsafe(Task& task, time_ms_t expiration = 0)
    {
        task.context = (cSimulation::getActiveSimulation())->getContextModule();
        TaskScheduler::add_unsafe(task,expiration);
    }
};

Define_Module(SchedulerModule);

TaskScheduler& getScheduler()
{
    cModule* module = (cSimulation::getActiveSimulation())->getContextModule();
    cModule* sch = NULL;

    while(module) {
        sch = module->getSubmodule("scheduler");
        if(sch) {
            break;
        }
        module = module->getParentModule();
    }

    SchedulerModule* scheduler = dynamic_cast<SchedulerModule*>(sch);
    ASSERT(scheduler);
    return *scheduler;
}

}

