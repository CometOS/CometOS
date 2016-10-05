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

#ifndef PARAMETERSTORE_H_
#define PARAMETERSTORE_H_

#include "Vector.h"
#include "Module.h"
#include "firmwareVersion.h"
#include "primitives.h"
#include "cometosError.h"
#include "palLed.h"

namespace cometos {


class StoredConfiguration : public ByteVector {
public:

    static const uint8_t CFG_SIZE = 100;

    StoredConfiguration() : ByteVector(buffer, CFG_SIZE)
    {}

private:
    uint8_t buffer[CFG_SIZE];
};

/**
 * Base class for implementations of a persistent storage for the configuration
 * data of all module on the node. Offers a simple CR(U)D interface to do so.
 */
class ParameterStore : public Module {
public:
    static const char * const MODULE_NAME;

    static ParameterStore* get(Module& m);

    ParameterStore();
    virtual ~ParameterStore();

    /**
     * Delete configuration data of the given module from persistent storage.
     * @param m pointer to module, whose configuration data shall be deleted.
     * @return COMETOS_ERROR_ALREADY if the configuration data for this
     *                                module is not persisted
     *         COMETOS_SUCCESS if the configuration data was successfully
     *                         deleted.
     */
    cometos_error_t resetCfgData(Module * m);


    /**
     * Get configuration data of the given module from persistent storage.
     * @param m   Module to which the configuration belongs
     * @param val Reference to the configuration data structure, has to be a
     *            subclass of PersistableConfig.
     * @return    COMETOS_ERROR_NOT_FOUND if no configuration data could be
     *                                    found for the given module
     *            COMETOS_ERROR_SIZE if the size of the persisted data does not
     *                               fit into the given byte array
     *            COMETOS_ERROR_FAIL if some other error occurred
     *            COMETOS_ERROR_INVALID if the versions of the current firmware
     *                                  and the one used to store the
     *                                  configuration do not match.
     *            COMETOS_SUCCESS    if the configuration data was read
     *                               successfully from storage
     */
    template<typename T>
    cometos_error_t getCfgData(Module * m, T & val) {
        StoredConfiguration rawData;

        firmwareVersionNumber_t firmwareVersion;

        // get cfg data from somewhere, check stored version and current version
        cometos_error_t result = readRawData(m, rawData);

        if (result == COMETOS_SUCCESS) {
            unserialize(rawData, firmwareVersion);

            if (firmware_getVersionNumber() != firmwareVersion) {
                result = COMETOS_ERROR_INVALID;
                val.setPersistent(false);
            } else {
                unserialize(rawData, val);
                result = COMETOS_SUCCESS;
                // order matters here -- we read the complete raw data into val,
                // including the flags byte (which then obviously has to be changed
                // AFTERWARDS
                val.setPersistent(true);
            }
        } else {
            val.setPersistent(false);
        }

        val.setErrorStatus(result);
        return result;
    }

    /**
     * Store configuration data of a module into persistent storage.
     * @param m   Module to which the configuration belongs
     * @param val Reference to the data structure to store the read data. Has
     *            to be a subclass of PersistableConfig
     * @return    COMETOS_ERROR_FAIL if no persistent storage location could
     *                               be found or created
     *            COMETOS_ERROR_SIZE if the size of the configuration data
     *                               was to big to be stored
     *            COMETOS_SUCCESS    if the configuration data was persisted
     *                               successfully
     *            any error case causes the persistent bit of the
     *            PersistableConfig parameter to be cleared!
     */
    template<typename T>
    cometos_error_t setCfgData(Module * m, T & val) {
        StoredConfiguration rawData;

        if (! val.isValid()) {
            return COMETOS_ERROR_INVALID;
        }

        // write configuration data to byte stream
        serialize(rawData, val);

        // write firmwareVersion for future comparison to byte stream
        serialize(rawData, firmware_getVersionNumber());

        // put configuration data to some storage
        cometos_error_t result = storeRawData(m, rawData);
        if (result != COMETOS_SUCCESS) {
            val.setPersistent(false);
        }
        return result;
    }

private:
    /** Persists configuration data belonging to the given module. Has to be
     *  overwritten by a concrete implementation of the ParameterStore
     *  interface.
     * @param m   Module to which the configuration belongs
     * @param raw Serialized configuration data
     * @return    COMETOS_ERROR_FAIL if no persistent storage location could
     *                               be found or created
     *            COMETOS_ERROR_SIZE if the size of the configuration data
     *                               was to big to be stored
     *            COMETOS_SUCCESS    if the configuration data was persisted
     *                               successfully
     */
    virtual cometos_error_t storeRawData(Module * m, const StoredConfiguration & raw) = 0;

    /** Reads configuration data of the given module from persistent storage.
     *  Has to be overwritten by a concrete implementation of the ParameterStore
     *  interface.
     * @param m   Module to which the configuration belongs
     * @param raw Byte array to write the read configuration into.
     * @return    COMETOS_ERROR_NOT_FOUND if no configuration data could be
     *                                    found for the given module
     *            COMETOS_ERROR_SIZE if the size of the persisted data does not
     *                               fit into the given byte array
     *            COMETOS_ERROR_FAIL if some other error occurred
     *            COMETOS_SUCCESS    if the configuration data was read
     *                               successfully from storage
     */
    virtual cometos_error_t readRawData(Module * m, StoredConfiguration & raw) = 0;

    /** Removes the configuration data of a module from persistent storage.
     *  Has to be overwritten by a concrete implementation of the ParameterStore
     *  interface.
     *  @param m Module of which to delete the configuration data.
     *  @return COMETOS_ERROR_ALREADY if the configuration data for this
     *                                module is not persisted
     *          COMETOS_SUCCESS if the configuration data was successfully
     *                          deleted.
     */
    virtual cometos_error_t resetRawData(Module * m) = 0;
};

} /* namespace cometos */

#endif /* PARAMETERSTORE_H_ */
