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
 * @author Martin Ringwelski, Andreas Weigel
 */

/**
 * Macros for working with 6loWPAN encoded Packets.
 * 
 */

#ifndef LOWPAN_MACROS_H_
#define LOWPAN_MACROS_H_

#include "UDPPacket.h"
#include "IPv6HopByHopOptionsHeader.h"
#include "IPv6DestinationOptionsHeader.h"
#include "IPv6FragmentHeader.h"
#include "IPv6RoutingHeader.h"


namespace cometos_v6 {

const uint8_t LOWPAN_FRAG_HEADER1_LENGTH = 4;
const uint8_t LOWPAN_FRAG_HEADERX_LENGTH = 5;
const uint8_t LOWPAN_FRAG_HEADER_MSB_SIZE_MASK = 0x07;

const uint8_t LOWPAN_FRAG_OFFSET_BYTES_PER_UNIT = 8;
const uint8_t LOWPAN_FRAG_OFFSET_SHIFT = 3;
const uint8_t LOWPAN_FRAG_OFFSET_MASK = 0x07;
const uint8_t IPHC_HEADER_DISPATCH = 0x60;
const uint8_t LOWPAN_CONGESTION_SHIFT = 4;


/**
 *
 */
inline uint16_t offsetToByteSize(uint8_t offset) {
    return ((uint16_t)offset) << LOWPAN_FRAG_OFFSET_SHIFT;
}

inline uint8_t byteSizeToOffset(uint16_t size) {
    return size >> LOWPAN_FRAG_OFFSET_SHIFT;
}

/**
 * isNotA6LowpanHeader
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is not a 6Lowpan Header
 */
inline bool isNotA6LowpanHeader(uint8_t a) {
    return ((a & 0xC0) == 0x0);
}

/**
 * isIPv6Uncompressed
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is an uncompressed IPv6 Header
 */
inline bool isIPv6Uncompressed(uint8_t a) {
    return (a == 0x41);
}

/**
 * isMeshHeader
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is a Mesh Header
 */
inline bool isMeshHeader(uint8_t a) {
    return ((a & 0xC0) == 0x80);
}


/**
 * isBroadcastHeader
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is a LOWPAN_BC0 Broadcast
 *              Header
 */
inline bool isBroadcastHeader(uint8_t& a) {
    return (a  == 0x50);
}

//------FRAGMENTATION HEADER------

/**
 * isFragmentationHeader
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is a Fragmentation Header
 */
inline bool isFragmentationHeader(uint8_t a) {
    return ((a & 0xC0) == 0xC0);
}

inline bool isBpCongestionFlagSet(uint8_t a) {
    return isFragmentationHeader(a) && ((a & 0x10) != 0);
}


/**
 * isFragmentationHeader1
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is the first Fragmentation Header
 */
inline bool isFragmentationHeader1(uint8_t a) {
    return ((a & 0xE0) == 0xC0);
}

/**
 * isFragmentationHeaderX
 *
 * @param a     Pointer to the message
 *
 * @return      true, if the next Header is a following Fragmentation Header
 */
inline bool isFragmentationHeaderX(uint8_t a) {
    return ((a & 0xE0) == 0xE0);
}

inline bool isLFFRFragmentationHeader(uint8_t a) {
    return ((a & 0xFC) == 0xE8);
}

/**
 * setFragmentationHeader1
 * Sets the bits for the First Fragmentation Header
 *
 * @param a     Pointer to the message buffer
 */
inline void setFragmentationHeader1(uint8_t* a, uint16_t size, uint16_t tag) {
    a[0] = 0xC0 | ((size >> 8)&0x07);
    a[1] = size & 0xFF;
    a[2] = tag >> 8;
    a[3] = tag & 0xFF;
}

/**
 * setFragmentationHeader1
 * Sets the bits for the following Fragmentation Header
 *
 * @param a     Pointer to the message buffer
 */
inline void setFragmentationHeaderX(uint8_t* a, uint16_t size, uint16_t tag, uint8_t offset) {
    a[0] = 0xE0 | ((size >> 8)&0x07);
    a[1] = size & 0xFF;
    a[2] = tag >> 8;
    a[3] = tag & 0xFF;
    a[4] = offset;
}

/**
 * getFragmentationSize
 *
 * @param a     Pointer to the message
 *
 * @return      uint16_t size of the Fragmented Packet
 */
inline uint16_t getFragmentationSize(const uint8_t* a) {
    return (((uint16_t)(a[0] & 0x07)<<8)|(a[1]));
}

/**
 * setFragmentationSize
 *
 * @param a     Pointer to the message buffer
 * @param b     Size of the Fragmented Packet
 */
inline void setFragmentationSize(uint8_t* a, uint16_t b) {
    a[0] = (a[0] & 0xF8) | ((b>>8)&0x07);
    a[1] = (b & 0xFF);
}

/**
 * getFragmentationTag
 *
 * @param a     Pointer to the message
 *
 * @return      uint16_t tag of the Fragmented Packet
 */
inline uint16_t getFragmentationTag(const uint8_t* a) {
    return (((uint16_t)a[2] << 8) | a[3]);
}

/**
 * setFragmentationTag
 *
 * @param a     Pointer to the message buffer
 * @param b     Tag of the Fragmented Packet
 */
inline void setFragmentationTag(uint8_t* a, uint16_t b) {
    a[2] = (b>>8);
    a[3] = (b &0xFF);
}

/**
 * getFragmentationOffset
 *
 * @param a     Pointer to the message
 *
 * @return      uint8_t Offset of the Fragmented Packet
 */
inline uint8_t getFragmentationOffset(const uint8_t* a) {
    return (a[4]);
}

/**
 * setFragmentationOffset
 *
 * @param a     Pointer to the message buffer
 * @param b     Offset of the Fragmented Packet
 */
inline void setFragmentationOffset(uint8_t* a, uint8_t o) {
    a[4] = o;
}

//------IPHC HEADER------

/**
 * isIPHCHeader
 *
 * @param a     Pointer to the message
 *
 * @return      true, if IPv6 Header is compressed with IPHC
 */
inline bool isIPHCHeader(uint8_t a) {
    return ((a & 0xE0) == IPHC_HEADER_DISPATCH);
}
/**
 * isIPHCHeader
 * Sets the bits for the IPHC Header
 *
 * @param a     Pointer to the message buffer
 */
inline void setIPHCHeader(uint8_t* a) {
    a[0]|=IPHC_HEADER_DISPATCH;
}

#ifdef SWIG
enum TFValues {
#else
enum TFValues_t : uint8_t {
#endif
    TF_ALL_ELIDED	= 0x18,
    TF_FL_ELIDED	= 0x10,
    TF_DSCP_ELIDED	= 0x08,
    TF_INLINE		= 0x00
};

inline TFValues_t getTF(uint8_t a) {
    return (TFValues_t)(a & 0x18);
}

// Bytes used to encode the TrafficClass and FlowLabel
const uint8_t tfValues[4] = {4, 3, 1, 0};
/**
 * getTFShifted
 * output can be used with tfValues to get the number of used bytes
 *
 * @param a     Pointer to the message
 */
inline uint8_t getTFShifted(const uint8_t* a) {
    return ((a[0] >> 3) & 0x3);
}

/**
 * isCIDInline
 *
 * @param a     Pointer to the message
 *
 * @return      true, if ContextID is inline
 */
inline bool isCIDInline(uint8_t b) {
    return ((b & 0x80) == 0x80);
}

/**
 * setCIDInline
 * Set the Bit for the CID to be inline
 *
 * @param a     Pointer to the message buffer
 */
inline void setCIDInline(uint8_t* a) {
    a[1] |= 0x80;
}

/**
 * isNHInline
 *
 * @param a     Pointer to the message
 *
 * @return      true, if NextHeader Field is inline
 */
inline bool isNHInlineIPHC(uint8_t a) {
    return ((a & 0x04) == 0x00);
}


/**
 * setNHCompressed
 * Set the bit for Next Header to be compressed
 *
 * @param a     Pointer to the message buffer
 */
inline void setNHCompressed(uint8_t* a) {
    a[0] |= 0x04;
}

#ifdef SWIG
enum HLIM_Values_t {
#else
enum HLIM_Values_t : uint8_t {
#endif
    HLIM_INLINE = 0,
    HLIM_1      = 1,
    HLIM_64     = 2,
    HLIM_255    = 3
};
/**
 * getHLim
 *
 * @param a     Pointer to the message
 *
 * @return      HLIM_INLINE, HLIM_1, HLIM_64 or HLIM_255
 */
inline HLIM_Values_t getHLim(uint8_t a) {
    return (HLIM_Values_t)(a & 0x03);
}

inline bool isContextBasedSAC(uint8_t b) {
    return ((b & 0x40) == 0x40);
}
inline void setContextBasedSAC(uint8_t* a) {
    a[1] |= 0x40;
}

inline bool isContextBasedDAC(uint8_t b) {
    return ((b & 0x04) == 0x04);
}
inline void setContextBasedDAC(uint8_t* a) {
    a[1] |= 0x04;
}

inline bool isMulticast(uint8_t b) {
    return ((b & 0x08) == 0x08);
}
inline void setMulticast(uint8_t* a) {
    a[1] |= 0x08;
}

/** Stateless address compression, possible values */
#ifdef SWIG
enum AM_SL_Values_t {
#else
enum AM_SL_Values_t : uint8_t {
#endif
    AM_SL_INLINE = 0x00,
    AM_SL_8BYTE  = 0x01,
    AM_SL_2BYTE  = 0x02,
    AM_SL_ELIDED = 0x03
};

/** Stateful address compression, possible values */
#ifdef SWIG
enum AM_SF_Values_t {
#else
enum AM_SF_Values_t : uint8_t {
#endif
    AM_SF_UNSPEC = 0x00,
    AM_SF_8BYTE = 0x01,
    AM_SF_2BYTE = 0x02,
    AM_SF_ELIDED = 0x03
};

/** Source address mode possible values */
#ifdef SWIG
enum SAM_Values_t {
#else
enum SAM_Values_t : uint8_t {
#endif
    SAM_INLINE  = 0x00,
    SAM_8BYTE   = 0x01,
    SAM_2BYTE   = 0x02,
    SAM_ELIDED  = 0x03,
    SAM_SHIFT   = 4
};

//const uint8_t samValues[4] = {16, 8, 2, 0};
inline uint8_t getSAM(uint8_t b) {
    return ((b >> SAM_SHIFT) & 0x03);
}

enum {
    DAM_INLINE  = 0x00,
    DAM_8BYTE   = 0x01,
    DAM_2BYTE   = 0x02,
    DAM_ELIDED  = 0x03,
    DAM_SHIFT   = 0
};
const uint8_t damValues[4] = {16, 8, 2, 0};
enum {
    DAM_M_INLINE	= 0x00,
    DAM_M_6BYTE		= 0x01,
    DAM_M_4BYTE		= 0x02,
    DAM_M_1BYTE		= 0x03
};
const uint8_t damMValues[4] = {16, 6, 4, 1};

inline uint8_t getDAM(uint8_t b) {
    return (b & 0x03);
}

//------NHC HEADER------

const uint8_t NUM_COMPRESSIBLE_HEADERS = 5;
const uint8_t compressibleHeaders[NUM_COMPRESSIBLE_HEADERS] = {
        IPv6HopByHopOptionsHeader::HeaderNumber,
        IPv6RoutingHeader::HeaderNumber,
        IPv6FragmentHeader::HeaderNumber,
        IPv6DestinationOptionsHeader::HeaderNumber,
        UDPPacket::HeaderNumber
};

inline bool headerIsCompressible(uint8_t a) {
    for (uint8_t i = 0; i < NUM_COMPRESSIBLE_HEADERS; i++) {
        if (compressibleHeaders[i] == a) {
            return true;
        }
    }
    return false;
}

// Extension Header
inline bool isExtensionHeader(uint8_t a) {
    return ((a & 0xF0) == 0xE0);
}
inline bool isNHInlineNHC(uint8_t a) {
    return ((a & 0x01) == 0x00);
}

#ifdef SWIG
enum EH_Id_t {
#else
enum EH_Id_t : uint8_t {
#endif
    EH_EID_HBH		= 0x0,
    EH_EID_ROUTING	= 0x2,
    EH_EID_FRGMNT	= 0x4,
    EH_EID_DSTOBT	= 0x6,
    EH_EID_MOBILITY	= 0x8,
    EH_EID_IPv6		= 0xE
};
inline EH_Id_t getEHId(uint8_t a) {
    return (EH_Id_t)(a & 0x0E);
}

// UDP Header
inline bool isUDPHeader(uint8_t a) {
    return ((a & 0xF8) == 0xF0);
}
inline bool udpHasChecksum(uint8_t a) {
    return ((a & 0x04) == 0x00);
}

#ifdef SWIG
enum UDP_CType {
#else
enum UDP_CType : uint8_t {
#endif
    UDP_INLINE		= 0x0,
    UDP_SRC_INLINE	= 0x1,
    UDP_DST_INLINE	= 0x2,
    UDP_SHORT_PORTS	= 0x3
};
inline UDP_CType getUDPPortCompression(uint8_t a) {
    return (UDP_CType)(a & 0x03);
}

}

#endif
