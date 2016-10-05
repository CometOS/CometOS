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

/**@file Example for using FNET to provide ethernet support to ARM boards
 * @author Florian Meier
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "pal.h"
#include "palLed.h"
#include "TaskScheduler.h"
#include "logging.h"
#include "cometos.h"
#include "palPin.h"

extern "C"
{
#include "fnet.h"
#include "fnet_cpu.h"
}

/*PROTOTYPES-----------------------------------------------------------------*/

void toggleLed();


cometos::TaskScheduler scheduler;
cometos::SimpleTask task(toggleLed);

void toggleLed() {
	static uint8_t counter = 0;
	palLed_off(0xFF);
	palLed_on(counter++);
	scheduler.add(task, 500);
	cometos::getCout() << ".";
}

int main() {
	pal_init();
	scheduler.add(task, 500);
	cometos::getCout() << "Booting" << cometos::endl;

	cometos::getCout() << __TIME__ << cometos::endl;

	fnet_netif_desc_t netif = fnet_netif_get_default();
	cometos::getCout() << "Link Status: " << (fnet_netif_connected(netif) ? "connected" : "unconnected") << cometos::endl;

	fnet_mac_addr_t macaddr;
	if(fnet_netif_get_hw_addr(netif, macaddr, sizeof(fnet_mac_addr_t)) == FNET_OK)
	{
		char mac_str[FNET_MAC_ADDR_STR_SIZE];
		cometos::getCout() << "MAC Address: " << fnet_mac_to_str(macaddr, mac_str) << cometos::endl;
	}

	fnet_ip4_addr_t local_ip;
	local_ip = fnet_netif_get_ip4_addr(netif);
	char ip_str[FNET_IP_ADDR_STR_SIZE]={0};
	cometos::getCout() << "IP Address: " << fnet_inet_ntoa(*(struct in_addr *)(&local_ip), ip_str) << cometos::endl;

	scheduler.run(true);

	return 0;
}

