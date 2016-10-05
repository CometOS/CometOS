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
 * @author Andreas Weigel
 */

#ifndef PERSISTABLECONFIG_H_
#define PERSISTABLECONFIG_H_

#include "cometosError.h"
#include "Vector.h"

namespace cometos {

/**
 * Helper class to be used as base for configuration classes for
 * different purposes:
 * <ul>
 *   <li>Prevent the last byte of a file to contain a "0", so that
 *       the coffee file system, which may or may not be used, does not
 *       get confused about file sizes</li>
 *   <li>To store some (meta) information about the result of the request to
 *       read a configuration. As remote access is currently not able to
 *       return an arbitrary number of return values, we can otherwise not
 *       encode any failure state. Thus, the error state of the getCfg
 *       request is encoded, along with the information, whether the
 *       read configuration equals the one currently used in memory.</li>
 *  </ul>
 *
 */
class PersistableConfig {
public:
    enum FlagMasks {
        ERROR_MASK = 0x0F,
        EQUALS_ACTIVE = 0x10,
        IS_PERSISTENT = 0x20,
        RESERVED = 0x80
    };

    PersistableConfig() :
        flags(RESERVED)
    {}

    virtual ~PersistableConfig() {}

    /** Part of the non-virtual interface. Serializes the base class part of
     * the object into a given byte vector and calls the subclass' method
     * to serialize its own data.
     *
     * @param buf  byte vector to store the data to
     */
    void serializeConfig(ByteVector & buf) const;

    /** Part of the non-virtual interface. Unserializes the base class part of
     * the object from the given byte vector into this object and calls the
     * subclass' unserialize method to unserialize its own data.
     *
     * @param buf  byte vector to unserialize the data from
     */
    void unserializeConfig(ByteVector & buf);

    /** Sets the error status of the configuration object.
     * @param error  the error status of the (de)serialization of this object
     */
    void setErrorStatus(cometos_error_t error) {
        flags &= ~ERROR_MASK;
        flags |= ERROR_MASK & error;
    }

    /** Get the error status of the configuration object
     * @return error status of the (de)serialization of this object.
     */
    cometos_error_t getErrorStatus() {
        return flags & ERROR_MASK;
    }

    /** @return true,  if data was deserialized from persistent storage
     *          false, if not
     */
    bool isPersistent() {
        return flags & IS_PERSISTENT;
    }

    /** Set the persistency status.
     * @param value  persistency status, can be retrieved by isPersistent()
     */
    void setPersistent(bool value) {
        flags &= ~IS_PERSISTENT;
        if (value) {
            flags |= IS_PERSISTENT;
        }
    }

    /** Sets a flag, which reflect if the configuration data is equal to the
     *  configuration currently in use by a module.
     *  @param value  value of the equality flag
     */
    void setEqualToActiveConfig(bool value) {
        flags &= ~EQUALS_ACTIVE;
        if (value) {
            flags |= EQUALS_ACTIVE;
        }
    }

    /** @return true, if this configuration is equal to the module's active
     *                configuration
     *          false, else
     */
    bool isEqualToActiveConfiguration() {
        return flags & EQUALS_ACTIVE;
    }

private:
    /**
     * Serializes the configuration data, has to be implemented by a subclass.
     * Called by serializeConfig()
     * @param buf byte vector to serialize the data into.
     */
    virtual void doSerialize(ByteVector & buf) const = 0;

    /**
     * Deserializes the configuration data, has to be implemented by a subclass.
     * Called by unserializeConfig()
     * @param buf byte vector to deserialize the data from.
     */
    virtual void doUnserialize(ByteVector & buf) = 0;

    virtual bool isValid() = 0;


    uint8_t flags;
};

void serialize(ByteVector & buf, const PersistableConfig & val);
void unserialize(ByteVector & buf, PersistableConfig & val);

} /* namespace cometos */

#endif /* PERSISTABLECONFIG_H_ */
