# CometOS
A component-based, extensible, tiny operating system for wireless networks.

## Main features


* Full communication stack based on message passing [1]
    * IEEE 802.15.4
        * Provides Software- and Hardware-MACs [2]
    * IPv6, 6LoWPAN, RPL [3]
    * Neighbourhood Management and Topology Control [4]
    * UART, I2C, RS-485, SPI...
    * ...
* Runs on hardware as well as in the OMNeT++ simulator
    * CometOS components are directly mapped to OMNeT++ modules
    * Supported hardware platforms
        * ATmega128RFA1 and ATmega256RFR2 based nodes
        * M3 open node (used in the IoT-LAB)
        * Freescale FRDM-K64F (without IEEE 802.15.4 support)
        * C110L based nodes (without IEEE 802.15.4 support) 
    * Suitable for large automated testbeds
        * Over the Air Programming (OTAP) [5]
        * Remote control
        * Persistent data and configuration storage
        * Time Synchronization
        * Logging

## Literature

[1] Stefan Unterschütz, Andreas Weigel and Volker Turau. Cross-Platform Protocol Development Based on OMNeT++. In Proceedings of the 5th International Workshop on OMNeT++ (OMNeT++'12), March 2012. Desenzano, Italy. 

[2] Andreas Weigel and Volker Turau. Hardware-Assisted IEEE 802.15.4 Transmissions and Why to Avoid Them. In Conference proceedings of the 8th International Conference on Internet and Distributed Computer Systems, IDCS 2015, September 2015, pp. 223–234. Windsor, UK. 

[3] Andreas Weigel, Martin Ringwelski, Volker Turau and Andreas Timm-Giel. Route-over forwarding techniques in a 6LoWPAN. EAI Endorsed Transactions on Mobile Communications and Applications, 14(5), December 2014. 

[4] Gerry Siegemund, Volker Turau und Christoph Weyer. A Dynamic Topology Control Algorithm for Wireless Sensor Networks. In Proceedings of the International Conference on Ad-hoc, Mobile and Wireless Networks, ADHOC-NOW 2015, Juni 2015, pp. 3–18. Athens, Greece. 

[5] Stefan Unterschütz and Volker Turau. Fail-Safe Over-The-Air Programming and Error Recovery in Wireless Networks. In Proceedings of the 10th Workshop on Intelligent Solutions in Embedded Systems (WISES'12), June 2012. Klagenfurt, Austria. 

## Getting Started for Hardware

This describes how to compile and flash a minimal example to a ATmega128RFA1 based hardware with LEDs connected to the pins 1,2 and 5 of port G on an up-to-date Ubuntu system using the Olimex ISP.

1. Install the prerequisites
    
        sudo apt-get install gcc-avr binutils-avr avrdude avr-libc git scons

2. Clone the repository to a location of your choice. For the following we assume ~/cometos 

        git clone https://github.com/CometOS/CometOS.git ~/cometos

3. Add the required paths

        echo 'export COMETOS_PATH=~/cometos' >> ~/.bashrc
        echo 'export PATH=$PATH:$COMETOS_PATH/support/builder' >> ~/.bashrc
        source ~/.bashrc

4. Compile

        cd ~/cometos/examples/hardware/blink
        cob platform=devboard

5. Connect your hardware and flash

        cob platform=devboard programmer=olimex go

## Getting Started for Simulation

1. Install OMNeT++ 5

2. Clone the repository to a location of your choice. For the following we assume ~/cometos 

        git clone https://github.com/CometOS/CometOS.git ~/cometos

3. Create a new OMNeT++ project, but use `~/cometos` as the location.

4. Exclude the following paths in the Makemake settings and link the pthread library

        examples/hardware
        src/platform
        src/core/platform
        src/files/platform
        test

5. Clone the [CometOS_Externals](https://github.com/CometOS/CometOS_Externals) repository and add the required path

        git clone https://github.com/CometOS/CometOS_Externals.git ~/cometos_externals
        echo 'export COMETOS_EXTERNALS_PATH=~/cometos_externals' >> ~/.bashrc
        source ~/.bashrc

6. Create a new OMNeT++ project for the externals repository. Here exclude everything except the MiXiM directory. It has to be built as library and the base and module folder of MiXiM have to be selected as NED source folder.

7. Add the project reference to the externals project in the main project.

	
## License

CometOS itself is published under a a 3-clause BSD-style license:

Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Institute nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

However, for full functionality, several components with other licenses are used.
For details see the LICENSES.txt.
Some components - including the MiXiM framework needed for OMNeT++ simulations - are only
available under more strict licenses such as the LGPL. They are provided in an additional
repository [CometOS_Externals](https://github.com/CometOS/CometOS_Externals).
