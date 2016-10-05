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

#include "StaticConcentricMobility.h"

Define_Module(StaticConcentricMobility);

void StaticConcentricMobility::initialize(int stage)
{
    BaseMobility::initialize(stage);

    if (stage == 0)
    {
        unsigned int numHosts = par("numHosts");
        double distance = par("distance");

        static int index = -1; // TODO is there an alternative?
        index++;


    unsigned int totalCircles = 0;
    unsigned int totalNodesOnInnerCircles = 0;
    unsigned int nodesOnThisCircle = 1;

    unsigned int myCircle = 0;
    unsigned int nodesOnInnerCircles = 0;

    for(unsigned int i = 0; i < numHosts; i++) {
        if(i - totalNodesOnInnerCircles >= nodesOnThisCircle) {
            // start new circle
            totalCircles++;
            totalNodesOnInnerCircles += nodesOnThisCircle;
            nodesOnThisCircle = (int)(2*M_PI*totalCircles);
        }

        if(i == index) {
            myCircle = totalCircles;
            nodesOnInnerCircles = totalNodesOnInnerCircles;
        }
    }

    double x = distance*totalCircles;
    double y = distance*totalCircles;

    if(index > 0) {
        double radius = distance*myCircle;
        double angularStep = 2.0*M_PI/(int)(2*M_PI*myCircle);
        double angle = angularStep*(index-nodesOnInnerCircles);
        x += radius*cos(angle);
        y += radius*sin(angle);
    }

        move.setSpeed(0);
        move.setStart( Coord(x, y) );

        std::cout << x << " " << y << std::endl;

        recordScalar("x", x);
        recordScalar("y", y);
    }
}
