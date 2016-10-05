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

#include "TunTap.h"

// standard
#include <stdio.h>
#include <unistd.h>

// TunTap
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
//#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "cometos.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <getopt.h>
#include <sys/ioctl.h>

/**
 * Constructor of TUN Device.
 *
 * @param name name of desired TUN Interface
 */
TunTap::TunTap(string name, const IPv6Address& ip_prefix) {
        this->dev = name;
        tun_alloc(ip_prefix);
    }

/**
 * Get the name of the TUN Interface
 *
 * @return name of TUN Interface
 */
std::string TunTap::getName() {
    return dev;
}

/**
 * Get the filedescriptor of TUN Device
 *
 * @return used filedescriptor.
 */
int TunTap::getFd() {
    return fd;
}

/**
 * Initialisation of TUN interface.
 */
void TunTap::tun_alloc(const IPv6Address& ip_prefix) {
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        std::perror("error: failed to open /dev/net/tun");
        std::exit(1);
    }

    struct ifreq ifr;

    std::memset(&ifr, 0, sizeof(ifr));

    // TUN device and no Packet info flags
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    // Create with desired name
    if (!dev.empty())
        std::strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ);

    int err;

    // Get file descriptor
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        std::perror("error: ioctl failed");
        std::exit(1);
    }

    // set the name as attribute of class
    dev = ifr.ifr_name;

    // Info
    std::cout << "TUN Device " << dev << " set up properly."<< std::endl;

    // Auto set up TUN device /w IPv4 and IPv6 Addresses. Pay attention to the name. Automatically done with standard name
    if (dev == "tunBR") {
        system("sudo ip link set tunBR up");
        system("sudo ip addr add 192.168.0.0/16 dev tunBR");
        string cmd = "sudo ip addr add ";
        cmd += ip_prefix.str().c_str();
        cmd += "/112 dev tunBR";
        system(cmd.c_str());
    }

}

/**
 * Write buffer to network interface
 *
 * @param buffer Packet as buffer
 * @param length bufferlength
 */
void TunTap::send(const uint8_t* buffer, uint16_t length) {
    write(fd, buffer,length);
}
