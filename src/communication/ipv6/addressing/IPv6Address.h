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

#ifndef IPV6ADDRESS_H_
#define IPV6ADDRESS_H_

/*INCLUDES-------------------------------------------------------------------*/


#include "primitives.h"
#include "SString.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

class IPv6Context {
public:
    IPv6Context (const uint8_t a[12]) {
        set(a);
    }
    IPv6Context (const uint16_t a[6]) {
        set(a);
    }
    IPv6Context (uint8_t a1, uint8_t a2,
            uint8_t a3, uint8_t a4,
            uint8_t a5, uint8_t a6,
            uint8_t a7, uint8_t a8,
            uint8_t a9, uint8_t a10,
            uint8_t a11, uint8_t a12) {
        set(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
    }
    IPv6Context (uint16_t a1 = 0, uint16_t a2 = 0,
            uint16_t a3 = 0, uint16_t a4 = 0,
            uint16_t a5 = 0, uint16_t a6 = 0) {
        set(a1, a2, a3, a4, a5, a6);
    }
    IPv6Context (const IPv6Context& other) {
        set(other);
    }
    IPv6Context& operator= (IPv6Context const & other) {
        set(other);
        return *this;
    }
    IPv6Context& operator= (const uint16_t a[4]) {
        set(a);
        return *this;
    }
    void set (const uint8_t a[12]) {
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t j = i<<1;
            address[i] = (a[j] << 8) | a[j+1];
        }
    }
    void set (const uint16_t a[6]) {
        for (uint8_t i = 0; i < 6; i++) {
            address[i] = a[i];
        }
    }
    void set (uint8_t a1, uint8_t a2,
            uint8_t a3, uint8_t a4,
            uint8_t a5, uint8_t a6,
            uint8_t a7, uint8_t a8,
            uint8_t a9, uint8_t a10,
            uint8_t a11, uint8_t a12) {
        address[0] = (a1 << 8) | a2;
        address[1] = (a3 << 8) | a4;
        address[2] = (a5 << 8) | a6;
        address[3] = (a7 << 8) | a8;
        address[4] = (a9 << 8) | a10;
        address[5] = (a11 << 8) | a12;
    }
    void set (uint16_t a1, uint16_t a2,
            uint16_t a3, uint16_t a4,
            uint16_t a5, uint16_t a6) {
        address[0] = a1;
        address[1] = a2;
        address[2] = a3;
        address[3] = a4;
        address[4] = a5;
        address[5] = a6;
    }
    void set (const IPv6Context& other) {
        for (uint8_t i = 0; i < 6; i++) {
            address[i] = other.getAddressPart(i);
        }
    }
    uint16_t getAddressPart(uint8_t part) const {
        if (part < 6) return address[part];
        return 0;
    }
protected:
    uint16_t address[6];
};

class IPv6Address {
public:
    static const uint8_t MAX_PREFIX_LENGTH = 128;

    IPv6Address (const uint8_t a[16]) {
        set(a);
    }
    IPv6Address (const uint16_t a[8]) {
        set(a);
    }
    IPv6Address (uint8_t a1, uint8_t a2,
                 uint8_t a3, uint8_t a4,
                 uint8_t a5, uint8_t a6,
                 uint8_t a7, uint8_t a8,
                 uint8_t a9, uint8_t a10,
                 uint8_t a11, uint8_t a12,
                 uint8_t a13, uint8_t a14,
                 uint8_t a15, uint8_t a16) {
        set(a1,a2,a3,a4,a5,a6,a7,a8,
            a9,a10,a11,a12,a13,a14,a15,a16);
    }
    IPv6Address (uint16_t a1 = 0, uint16_t a2 = 0,
                 uint16_t a3 = 0, uint16_t a4 = 0,
                 uint16_t a5 = 0, uint16_t a6 = 0,
                 uint16_t a7 = 0, uint16_t a8 = 0) {
        set(a1,a2,a3,a4,a5,a6,a7,a8);
    }
    IPv6Address (IPv6Address const & other) {
        set(other);
    }
    IPv6Address& operator= (IPv6Address const & other) {
        set(other);
        return *this;
    }
    IPv6Address& operator= (const uint16_t a[8]) {
        set(a);
        return *this;
    }
//    IPv6Address& operator= (const uint8_t a[16]) {
//        set(a);
//        return *this;
//    }
    bool operator== (IPv6Address const & other) const;
    inline bool operator!= (IPv6Address const & other) const {
        return !this->operator ==(other);
    };
    bool operator< (IPv6Address const & other) const;
    bool operator> (IPv6Address const & other) const;
    IPv6Address getPrefix (uint8_t length) const;
    IPv6Address getSuffix (uint8_t length) const;
    uint16_t getAddressPart(uint8_t part) const;
    void setAddressPart(uint16_t a, uint8_t part);
    void setAddressPart(uint8_t a, uint8_t b, uint8_t part) {
        setAddressPart(((uint16_t)a << 8 | b), part);
    }
    void setPrefix (IPv6Address const & prefix,
                    uint8_t length);
    void setSuffix (IPv6Address const & suffix,
                    uint8_t length);
    void setLinkLocal () {
        address[0] &= 0x003F;
        address[0] |= 0xFE80;
    }
    bool matches (IPv6Address const & addr,
                  uint8_t length) const;
    bool isUnspecified () const;
    bool isLoopback () const;
    bool isLinkLocal () const;
    bool isUnicast () const;
    bool isMulticast () const;
    bool isLoopbackBroadcast () const;
    bool isLocalBroadcast () const;
    bool isLoopbackRouterBroadcast() const;
    bool isLocalRouterBroadcast() const;
    bool isSiteRouterBroadcast() const;
    void setContext(const IPv6Context& context,
            uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8) {
        for (uint8_t i = 0; i < 4; i++) {
            address[i] = context.getAddressPart(i);
        }
        address[4] = a5;
        address[5] = a6;
        address[6] = a7;
        address[7] = a8;
    }
    void set(uint8_t a1, uint8_t a2,
            uint8_t a3, uint8_t a4,
            uint8_t a5, uint8_t a6,
            uint8_t a7, uint8_t a8,
            uint8_t a9, uint8_t a10,
            uint8_t a11, uint8_t a12,
            uint8_t a13, uint8_t a14,
            uint8_t a15, uint8_t a16);
    void set(uint16_t a1, uint16_t a2,
            uint16_t a3, uint16_t a4,
            uint16_t a5, uint16_t a6,
            uint16_t a7, uint16_t a8);
    void set(const uint8_t a[16]);
    void set(const uint16_t a[8]);
    void set(IPv6Address const & other);
    void writeAddress(uint8_t dest[16]) const;
    uint8_t getMulticastScope () const;

    /**
    * Returns the textual representation of the address in the standard
    * notation.
    */
   cometos::SString<40> str() const;

#if defined(OMNETPP) || defined(BOARD_python) || defined(BOARD_local)
    IPv6Address (const char *addr) {
        tryParse(addr);
    }
    IPv6Address& operator= (const char *addr) {
        tryParse(addr);
        return *this;
    }

   bool doTryParse(const char *&addr);

   /**
    * Tries parsing an IPv6 address string into the object.
    * Returns true if the string contains a well-formed IPv6 address,
    * and false otherwise. All RFC 3513 notations are accepted (e.g.
    * FEDC:BA98:7654:3210:FEDC:BA98:7654:3210, FF01::101, ::1), plus
    * also "<unspec>" as a synonym for the unspecified address (all-zeroes).
    */
   bool tryParse(const char *addr);

   /**
    * Expects a string in the "<address>/<prefixlength>" syntax, parses
    * the address into the object (see tryParse(), and returns the prefix
    * length (a 0..128 integer) in the second argument. The return value
    * is true if the operation was successful, and false if it was not
    * (e.g. no slash in the input string, invalid address syntax, prefix
    * length is out of range, etc.).
    */
   bool tryParseAddrWithPrefix(const char *addr, uint8_t prefixLen);

#endif

protected:
    bool constructMask(uint8_t length, uint16_t mask[8]) const;

    bool rangeIsZero(uint8_t start, uint8_t end) const {
        while (start <= end) {
            if (address[start]) return false;
            start++;
        }
        return true;
    }

    uint16_t address[8];
};

inline void IPv6Address::set(uint8_t a1, uint8_t a2,
        uint8_t a3, uint8_t a4,
        uint8_t a5, uint8_t a6,
        uint8_t a7, uint8_t a8,
        uint8_t a9, uint8_t a10,
        uint8_t a11, uint8_t a12,
        uint8_t a13, uint8_t a14,
        uint8_t a15, uint8_t a16) {
    address[0] = (uint16_t)a1<<8 | a2;
    address[1] = (uint16_t)a3<<8 | a4;
    address[2] = (uint16_t)a5<<8 | a6;
    address[3] = (uint16_t)a7<<8 | a8;
    address[4] = (uint16_t)a9<<8 | a10;
    address[5] = (uint16_t)a11<<8 | a12;
    address[6] = (uint16_t)a13<<8 | a14;
    address[7] = (uint16_t)a15<<8 | a16;
}

inline void IPv6Address::set(uint16_t a1, uint16_t a2,
        uint16_t a3, uint16_t a4,
        uint16_t a5, uint16_t a6,
        uint16_t a7, uint16_t a8) {
    address[0] = a1;
    address[1] = a2;
    address[2] = a3;
    address[3] = a4;
    address[4] = a5;
    address[5] = a6;
    address[6] = a7;
    address[7] = a8;
}

inline void IPv6Address::set(const uint8_t a[16]) {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t j = i<<1;
        address[i] = (uint16_t)a[j]<<8 | a[j+1];
    }
}

inline void IPv6Address::set(const uint16_t a[8]) {
    for (uint8_t i = 0; i < 8; i++) {
        address[i] = a[i];
    }
}

inline void IPv6Address::set(IPv6Address const & other) {
    for (uint8_t i = 0; i < 8; i++) {
        address[i] = other.getAddressPart(i);
    }
}

/**
 * setAddress
 * sets one part of the Address
 *
 * @param a         16 bit part of the address
 * @param part      part of the address between two ':' (0 to 7)
 */
inline void IPv6Address::setAddressPart(uint16_t a, uint8_t part) {
    address[part] = a;
}

/**
 * getAddress
 * returns one part of a IPv6Address between to ':'.
 *
 * @param part      part of the address to be returned (0 to 7)
 *
 * @return          16 bit value of the address part
 */
inline uint16_t IPv6Address::getAddressPart(uint8_t part) const {
    if (part < 8) {
        return address[part];
    }
    return 0;
}

inline bool IPv6Address::isLinkLocal () const {
    return ((address[0] & 0xFFC0) == 0xFE80);
}

inline bool IPv6Address::isUnicast () const {
    return !isMulticast() && !isUnspecified();
}

inline bool IPv6Address::isMulticast () const {
    return ((address[0] & 0xFF00) == 0xFF00);
}


inline uint8_t IPv6Address::getMulticastScope () const {
    return (uint8_t) (address[0] & 0x000F);
}

struct uint16_nbo {
    uint8_t    msb;
    uint8_t    lsb;
    uint16_nbo(uint16_t a = 0): msb(a >> 8), lsb(a) {}
    void operator=(const uint16_t& a) {
        msb = a >> 8;
        lsb = a;
    }
    uint16_nbo& operator=(const uint16_nbo& a) {
        msb = a.msb;
        lsb = a.lsb;
        return *this;
    }
    uint16_t getUint16_t() const {
        return (((uint16_t)msb << 8) | lsb);
    }
};
struct uint32_nbo {
    uint16_nbo    msb;
    uint16_nbo    lsb;
    uint32_nbo(uint32_t a = 0): msb(a >> 16), lsb(a) {}
    void operator=(const uint32_t& a) {
        msb = a >> 16;
        lsb = a;
    }
    uint32_nbo& operator=(const uint32_nbo& a) {
        msb = a.msb;
        lsb = a.lsb;
        return *this;
    }
    uint32_t getUint32_t() {
        return (((uint32_t)msb.getUint16_t() << 16) | lsb.getUint16_t());
    }
};

namespace cometos {
void serialize(ByteVector& buffer, const IPv6Address& value);
void unserialize(ByteVector& buffer, IPv6Address& value);
void serialize(ByteVector& buffer, const uint16_nbo& value);
void unserialize(ByteVector& buffer, uint16_nbo& value);
void serialize(ByteVector& buffer, const uint32_nbo& value);
void unserialize(ByteVector& buffer, uint32_nbo& value);
}


#endif /* IPV6ADDRESS_H_ */
