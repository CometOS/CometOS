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
 * @author: Martin Ringwelski
 */

#ifndef FOLLOWINGHEADER_H_
#define FOLLOWINGHEADER_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "IPv6Address.h"

/*TYPES----------------------------------------------------------------------*/

/*MACROS---------------------------------------------------------------------*/

/*TYPES----------------------------------------------------------------------*/

namespace cometos_v6 {

typedef uint8_t headerType_t;


/**
 * TODO Handling with deletable pointers uglyly error-prone, virtual destructor
 * usually necessitates sensible assignment operator and copy constructor!
 */
class FollowingHeader {
public:
    static const headerType_t NoNextHeaderNumber = 59;
    static const uint8_t IPV6_EXTENSION_HEADER_ALIGNMENT_SIZE = 8;

    FollowingHeader(uint8_t headerType, FollowingHeader* next = NULL);
    virtual ~FollowingHeader();

    virtual FollowingHeader * cloneAll() const;

    virtual FollowingHeader * clone() const = 0;

    headerType_t getHeaderType () const;


    FollowingHeader* getNextHeader(uint8_t num = 0) const;


    /**
     * Returns a pointer to the last header without removing it from the
     * chain of headers.
     * @return pointer to last header of the chain, usually this should be
     *         an upper protocol header (UDP/ICMP/TCP)
     */
    FollowingHeader* getLastHeader();

    /**
     * Get the number of header in the chain of headers. Count
     * is started at this header.
     * @return number of header coming after this one.
     */
    uint8_t getNumNextHeaders() const;

    /**
     * Sets the next header of this header to the given one. Ownership
     * of the passed header is passed to this header/the first header
     * containing the chain of headers.
     * @param[in] pointer to the next header
     */
    void setNextHeader(FollowingHeader* next);

    /**
     * Similar to decapsulateHeader, but only removes the header from the
     * chain of headers and can also free the memory if desired.
     *
     * @param[in]
     *     number index of the header to be removed, relative to this header.
     *     0 means the next header, 1 the one after that and so on. -1 means
     *     the last header in the chain.
     * @param[in] delheader
     *     if true, delete will be called on this header, i.e., the caller of
     *     this function has to know how this header was created
     *     TODO this sucks! --> we should really start using unique_ptr here
     */
    void removeHeader (int8_t number, bool delHeader = true);

    /**
     * Remove a single header from the chain of headers, starting count
     * with this header.
     * @param[in] number
     *      index of the header to remove from the chain, starting with this
     *      header. 0 means the first following header, 1 the one after that
     *      and so on... -1 can be given to get the last header.
     * @return pointer to the header which had been removed from the chain of
     *         headers. Caller is responsible to free memory for this header.
     */
    FollowingHeader* decapsulateHeader(int8_t number);

    /**
     * Removes all headers following this on from the chain of headers.
     * The caller is responsible to clean up the memory of the returned
     * list of headers.
     *
     * @return header chain of all headers of the datagram following this one.
     */
    FollowingHeader* decapsulateAllHeaders();

    /**
     * Returns the total size of this header in bytes,
     * including next header and len fields and variable length
     * fields for extension headers. Does not include any padding
     * fields to realize alignment. Does NOT include any transport
     * layer payload.
     *
     * @return total size of header object
     */
    virtual uint16_t getSize() const = 0;

    /**
     * Returns the total size of this header in bytes,
     * including next header and len field and variable length fields
     * for extension header. In contrast to getSize, this function DOES
     * include padding fields necessary for extension header alignment.
     * Does NOT include any transport layer payload!
     */
    uint16_t getAlignedSize();

    /**
     * Return the fixed size of the header in bytes, i.e., the portion of the
     * header that is always the same and is carried in and can be
     * derived from the object's data structures.
     *
     * @return size of the fixed portion of the header
     */
    virtual uint16_t getFixedSize() const = 0;

    /**
     * Stores this header's data into the given buffer, respecting alignment
     * requirements of IPv6 extension headers, if applicable. Basically the
     * opposite of parse
     *
     * @param[out] buffer pointer to memory to write this header to
     */
    virtual uint16_t writeHeaderToBuffer(uint8_t* buffer) const = 0;

    /**
     * Returns the size of this header plus all the IPv6 extension
     * headers and/or terminating headers (UDP/TCP/ICMP) following
     * this one. Does not include any upper layer payload, i.e., UDP payload
     * is not counted!
     *
     * @return size in bytes of all chained headers, including this one,
     *         without payload
     */
    uint16_t getCompleteHeaderLength() const;

    /**
     * Returns a pointer to the upper layer payload, i.e, ICMP/UDP/TCP payload.
     * @return pointer to upper layer payload
     */
    virtual const uint8_t* getData();

    /**
     * Extension headers that define variable length fields are difficult
     * to capture within a fixed class structure. These headers may
     * store data in an external buffer and parse the content from there
     * on demand. This functions specifies the location and the size of
     * this buffer.
     *
     * @param data read-only pointer to where variable-length data of this
     *             header is stored
     * @param length size of the variable-length data
     */
    virtual void setData(const uint8_t* data, uint16_t length);


    /**
     * Returns the payload any upper layer protocol, without accounting
     * for the length of any extension headers or the size of the
     * upper layer protocol itself, i.e., UDP shall return only the size
     * of the payload, NOT the size it carries within its own header. If
     * the total size of upper layer protocols (including their headers)
     * is needed the results of getSize and this function have to be
     * added.
     * Has to be implemented by any subclass that represents an upper
     * layer protocol and terminates the chain of
     * headers e.g. UDP/TCP/ICMP.
     *
     * @return size of payload of an upper layer protocol
     */
    virtual uint16_t getUpperLayerPayloadLength() const;

    /**
     * Parses header data from a given buffer and fills the internal data
     * structure of the header. The opposite of writeHeaderToBuffer.
     *
     * @param[in] buffer contains the header
     * @param[in] length size of the header in bytes
     * @return number of bytes read from the buffer
     */
    virtual uint16_t parse(const uint8_t* buffer, uint16_t length) = 0;

    /**
     * To be implemented by upper layer headers; generates a checksum for
     * the header's content.
     * TODO should probably go into an UpperLayerFollowingHeader subclass
     *
     * @param[in] src IPv6 address of datagram's source
     * @param[in] dst IPv6 address of datagram's destination
     */
    virtual void generateChecksum(const IPv6Address& src, const IPv6Address& dst);


protected:
    headerType_t header;
    FollowingHeader* nextHeader;

private:
    /** dummy to prevent assignment */
   FollowingHeader & operator=(const FollowingHeader & other) {
       return *this;
   }

   /** dummy to prevent copy construction */
   FollowingHeader(const FollowingHeader & rhs) {

   }
};

uint16_t doingChecksum(uint32_t chksum,
        const uint8_t* data, uint16_t length,
        const IPv6Address &src, const IPv6Address &dst,
        uint16_t allLength, uint8_t headerNumber);


}

#endif /* FOLLOWINGHEADER_H_ */
