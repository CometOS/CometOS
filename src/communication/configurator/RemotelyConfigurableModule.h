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
#ifndef REMOTELYCONFIGURABLEMODULE_H_
#define REMOTELYCONFIGURABLEMODULE_H_

#include "RemoteModule.h"
#include "ParameterStore.h"

namespace cometos {
template <class T>
class RemotelyConfigurableModule : public RemoteModule {
public:
    RemotelyConfigurableModule(const char* service_name) :
        RemoteModule(service_name)
    {
    }

    virtual ~RemotelyConfigurableModule() {}

    virtual void initialize() {
        RemoteModule::initialize();
        remoteDeclare(&RemotelyConfigurableModule<T>::setConfig, "sc");
        remoteDeclare(&RemotelyConfigurableModule<T>::getConfig, "gc");
        remoteDeclare(&RemotelyConfigurableModule<T>::resetConfig, "rc");
        remoteDeclare(&RemotelyConfigurableModule<T>::getActiveConfig, "gac");
    }

    cometos_error_t setConfig(T& cfg) {
        // we do not want to delete the pool object while there are still
        // dynamically allocated datagrams flying around somewhere else!
        if (isBusy()) {
            return COMETOS_ERROR_BUSY;
        }

        if (cfg.isValid()) {

            cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
            if (ps != NULL) {
                ps->setCfgData(this, cfg);
            }

            applyConfig(cfg);

            return COMETOS_SUCCESS;
        } else {
            return COMETOS_ERROR_INVALID;
        }
    }

    cometos_error_t resetConfig() {
        cometos_error_t result = COMETOS_SUCCESS;
        cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
        if (ps != NULL) {
            result = ps->resetCfgData(this);
        }
        return result;
    }

    T getConfig() {
        cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
        T storedCfg;
        if (ps != NULL) {
            ps->getCfgData(this, storedCfg);
        } else {
            storedCfg.setPersistent(false);
        }
        storedCfg.setEqualToActiveConfig(storedCfg == getActive());

        return storedCfg;
    }

    T getActiveConfig() {
        return getActive();
    }

private:
    virtual bool isBusy() = 0;
    virtual void applyConfig(T& cfg) = 0;
    virtual T& getActive() = 0;
};

}

#endif /* REMOTELYCONFIGURABLEMODULE_H_ */
