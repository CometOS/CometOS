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
 * @author Martin Ringwelski
 */

#include "IPv6Address.h"
#include <string.h>
#include "OutputStream.h"

const uint8_t IPv6Address::MAX_PREFIX_LENGTH;

bool IPv6Address::operator== (IPv6Address const & other) const {
    return (address[0] == other.getAddressPart(0) &&
            address[1] == other.getAddressPart(1) &&
            address[2] == other.getAddressPart(2) &&
            address[3] == other.getAddressPart(3) &&
            address[4] == other.getAddressPart(4) &&
            address[5] == other.getAddressPart(5) &&
            address[6] == other.getAddressPart(6) &&
            address[7] == other.getAddressPart(7));
}

bool IPv6Address::operator< (IPv6Address const & other) const {
    for (uint8_t i = 0; i < 8; i++) {
        if (address[i] < other.getAddressPart(i)) {
            return true;
        } else if (address[i] > other.getAddressPart(i)) {
            return false;
        }
    }
    return false;
}

bool IPv6Address::operator> (IPv6Address const & other) const {
    for (uint8_t i = 0; i < 8; i++) {
        if (address[i] > other.getAddressPart(i)) {
            return true;
        } else if (address[i] < other.getAddressPart(i)) {
            return false;
        }
    }
    return false;
}

bool IPv6Address::constructMask(uint8_t length, uint16_t mask[8]) const {
    uint8_t     i;
    uint16_t    numShifts = length & 0xF;
    uint8_t     whole = length >> 4;

    if (length > 128) {
        ASSERT(false);
        return false;
    }

    for (i = 0; i < whole; i++) {
        mask[i] = 0xFFFF;
    }
    if (numShifts > 0) {
        mask[i] = 0xFFFF << (16 - numShifts);
    }

    return true;
}

/**
 * getPrefix
 * returns the prefix of the address.
 *
 * @param length    length of the prefix in bits
 *
 * @return          IPv6Address Object with the prefix and the rest 0
 */
IPv6Address IPv6Address::getPrefix (uint8_t length) const {
    uint16_t    mask[8] = {0,0,0,0,0,0,0,0};

    constructMask(length, mask);

    IPv6Address ret(address[0] & mask[0],
            address[1] & mask[1],
            address[2] & mask[2],
            address[3] & mask[3],
            address[4] & mask[4],
            address[5] & mask[5],
            address[6] & mask[6],
            address[7] & mask[7]);

    return ret;
}

/**
 * getSuffix
 * returns the suffix of the address.
 *
 * @param length    length of the prefix in bits
 *
 * @return          IPv6Address Object with the suffix and the rest 0
 */
IPv6Address IPv6Address::getSuffix (uint8_t length) const {
    uint16_t    mask[8] = {0,0,0,0,0,0,0,0};

    constructMask(length, mask);

    IPv6Address ret(
            address[0] & ~mask[0],
            address[1] & ~mask[1],
            address[2] & ~mask[2],
            address[3] & ~mask[3],
            address[4] & ~mask[4],
            address[5] & ~mask[5],
            address[6] & ~mask[6],
            address[7] & ~mask[7]);

    return ret;
}

void IPv6Address::setPrefix (IPv6Address const & prefix, uint8_t length) {
    uint16_t mask[8] = {0,0,0,0,0,0,0,0};

    constructMask(length, mask);

    for (uint8_t i = 0; i < 8; i++) {
        address[i] = (address[i] & ~mask[i]) | (prefix.getAddressPart(i) & mask[i]);
    }
}

void IPv6Address::setSuffix (IPv6Address const & suffix, uint8_t length) {
    uint16_t mask[8] = {0,0,0,0,0,0,0,0};

    constructMask(length, mask);

    for (uint8_t i = 0; i < 8; i++) {
        address[i] = (address[i] & mask[i]) | (suffix.getAddressPart(i) & ~mask[i]);
    }
}

void IPv6Address::writeAddress(uint8_t dest[16]) const {
    dest[0] = address[0] >> 8;
    dest[1] = address[0] & 0xFF;
    dest[2] = address[1] >> 8;
    dest[3] = address[1] & 0xFF;
    dest[4] = address[2] >> 8;
    dest[5] = address[2] & 0xFF;
    dest[6] = address[3] >> 8;
    dest[7] = address[3] & 0xFF;
    dest[8] = address[4] >> 8;
    dest[9] = address[4] & 0xFF;
    dest[10] = address[5] >> 8;
    dest[11] = address[5] & 0xFF;
    dest[12] = address[6] >> 8;
    dest[13] = address[6] & 0xFF;
    dest[14] = address[7] >> 8;
    dest[15] = address[7] & 0xFF;
}

bool IPv6Address::matches (IPv6Address const & addr, uint8_t length) const {
    uint16_t mask[8] = {0,0,0,0,0,0,0,0};

    constructMask(length, mask);

    return (((address[0]^addr.getAddressPart(0))&mask[0]) |
            ((address[1]^addr.getAddressPart(1))&mask[1]) |
            ((address[2]^addr.getAddressPart(2))&mask[2]) |
            ((address[3]^addr.getAddressPart(3))&mask[3]) |
            ((address[4]^addr.getAddressPart(4))&mask[4]) |
            ((address[5]^addr.getAddressPart(5))&mask[5]) |
            ((address[6]^addr.getAddressPart(6))&mask[6]) |
            ((address[7]^addr.getAddressPart(7))&mask[7])) == 0;
}

bool IPv6Address::isUnspecified () const {
    return rangeIsZero(0,7);
}

bool IPv6Address::isLoopback () const {
    return (rangeIsZero(0,6) &&
            address[7] == 1
            );
}

bool IPv6Address::isLoopbackBroadcast () const {
    return (
            address[0] == 0xff01 &&
            rangeIsZero(1,6) &&
            address[7] == 1
            );
}

bool IPv6Address::isLocalBroadcast () const {
    return (
            address[0] == 0xff02 &&
            rangeIsZero(1,6) &&
            address[7] == 1
            );
}

bool IPv6Address::isLoopbackRouterBroadcast() const {
    return (
            address[0] == 0xff01 &&
            rangeIsZero(1,6) &&
            address[7] == 2
            );
}

bool IPv6Address::isLocalRouterBroadcast() const {
    return (
            address[0] == 0xff02 &&
            rangeIsZero(1,6) &&
            address[7] == 2
            );
}

bool IPv6Address::isSiteRouterBroadcast() const {
    return (
            address[0] == 0xff05 &&
            rangeIsZero(1,6) &&
            address[7] == 2
            );
}

// Helper: finds the longest sequence of zeroes in the address (at least with len=2)
static void findGap(const uint16_t *groups, int& start, int& end)
{
    start = end = 0;
    int beg = -1;
    for (int i=0; i<8; i++)
    {
        if (beg==-1 && groups[i]==0)
        {
            // begin counting
            beg = i;
        }
        else if (beg!=-1 && groups[i]!=0)
        {
            // end counting
            if (i-beg>=2 && i-beg>end-start) {
                start = beg; end = i;
            }
            beg = -1;
        }
    }

    // check last zero-seq
    if (beg!=-1 && beg<=6 && 8-beg>end-start) {
        start = beg; end = 8;
    }
}

cometos::SString<40> IPv6Address::str() const
{
    cometos::SString<40> ret;
    if (isUnspecified()) {
        ret = "<unspec>";
        return ret;
    }

    // find longest sequence of zeros in groups[]
    int start, end;
    findGap(address, start, end);
    if (start==0 && end==8) {
        ret = "::0";    // the unspecified address is a special case
        return ret;
    }

    // print groups, replacing gap with "::"
    for (int i=0; i<start; i++) {
        ret += ((i==0?"":":"));
        ret.append(address[i], cometos::SString_HEX);
    }
    if (start!=end) {
        ret += "::";
    }
    for (int j=end; j<8; j++) {
        ret += (j==end?"":":");
        ret.append(address[j], cometos::SString_HEX);
    }
    return ret;
}

#if defined(OMNETPP) || defined(BOARD_python) || defined(BOARD_local)

static int parseGroups(const char *&s, int *groups)
{
    int k = 0;
    while (k < 8)
    {
        char *e;
        groups[k] = strtoul(s, &e, 16);
        if (s==e) { // no hex digit converted
            if (k!=0) s--;  // "unskip" preceding ':'
            return k;
        }
        // if negative or too big, return (s will point to beginning of large number)
        if (groups[k]<0 || groups[k]> (int)0xffff)
            return k;
        k++;  // group[k] successfully stored
        s = e;  // skip converted hex number
        if (*s!=':' || k==8)
            return k;
        s++;  // skip ':'
    }
    return k;
}

bool IPv6Address::doTryParse(const char *&addr)
{
    if (!strcmp(addr, "<unspec>"))
    {
        addr += 8;
        address[0] = address[1] = address[2] = address[3] = address[4] = address[5] = address[6] = address[7] = 0;
        return true;
    }

    // parse and store 16-bit units
    int groups[8];
    int numGroups = parseGroups(addr, groups);

    // if address string contains "::", parse and store second half too
    if (*addr==':' && *(addr+1)==':')
    {
        addr += 2;
        int suffixGroups[8];
        int numSuffixGroups = parseGroups(addr, suffixGroups);

        // merge suffixGroups[] into groups[]
        if (numGroups+numSuffixGroups>8)
            return false; // too many
        for (int i=numGroups; i<8; i++) {
            int j = i-8+numSuffixGroups;
            groups[i] = j<0 ? 0 : suffixGroups[j];
        }
        numGroups = 8;
    }

    if (numGroups!=8)
        return false; // too few

    // copy groups to d[]
    for (unsigned int i=0; i<8; i++)
        address[i] = groups[i];

    return true;
}


bool IPv6Address::tryParse(const char *addr)
{
    if (!addr)
        return false;
    if (!doTryParse(addr))
        return false;
    if (*addr!=0)
        return false; // illegal trailing character
    return true;
}

bool IPv6Address::tryParseAddrWithPrefix(const char *addr, uint8_t prefixLen)
{
    if (!addr)
        return false;
    if (!doTryParse(addr))
        return false;
    if (*addr!='/')
        return false; // no '/' after address
    addr++;

    // parse prefix
    char *e;
    prefixLen = strtoul(addr, &e, 10);
    if (addr==e)
        return false; // no number after '/'
    if (*e!=0)
        return false; // garbage after number
    if (prefixLen>128)
        return false; // wrong len value
    return true;
}

#endif

namespace cometos {

void serialize(ByteVector& buffer, const IPv6Address& value) {
    serialize(buffer, value.getAddressPart(7));
    serialize(buffer, value.getAddressPart(6));
    serialize(buffer, value.getAddressPart(5));
    serialize(buffer, value.getAddressPart(4));
    serialize(buffer, value.getAddressPart(3));
    serialize(buffer, value.getAddressPart(2));
    serialize(buffer, value.getAddressPart(1));
    serialize(buffer, value.getAddressPart(0));
}

void unserialize(ByteVector& buffer, IPv6Address& value) {
   uint16_t addr[8];
   unserialize(buffer, addr[0]);
   unserialize(buffer, addr[1]);
   unserialize(buffer, addr[2]);
   unserialize(buffer, addr[3]);
   unserialize(buffer, addr[4]);
   unserialize(buffer, addr[5]);
   unserialize(buffer, addr[6]);
   unserialize(buffer, addr[7]);
   value.set(addr);
}

void serialize(ByteVector& buffer, const uint16_nbo& value) {
    serialize(buffer, value.lsb);
    serialize(buffer, value.msb);
}

void unserialize(ByteVector& buffer, uint16_nbo& value) {
    unserialize(buffer, value.msb);
    unserialize(buffer, value.lsb);
}

void serialize(ByteVector& buffer, const uint32_nbo& value) {
    serialize(buffer, value.lsb);
    serialize(buffer, value.msb);
}

void unserialize(ByteVector& buffer, uint32_nbo& value) {
    unserialize(buffer, value.msb);
    unserialize(buffer, value.lsb);
}

}


//#endif //ifdef OMNETPP
