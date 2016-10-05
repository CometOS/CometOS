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
 * @author Florian Meier (Amplifier support)
 */

#include "mac_interface.h"
#include "atrf_hardware.h"
#include "cometos.h"

/**
 * Minimum power level the transceiver used by this MAL supports; the
 * minimum power level SHALL correspond to the minimum actual transmission power
 */
#define MAC_MIN_TX_POWER_LEVEL 0 	// -16.5 dBm

/**
 * Maximum power level the transceiver used by this MAL supports; the
 * maximum power level SHALL correspond to the maximum actual transmission power
 */
#define MAC_MAX_TX_POWER_LEVEL 31	//   3.5 dBm+22 dB

#define PLL_TX_FLT 4

#define SET_BIT(port, bit)    ((port) |= _BV(bit))
#define CLR_BIT(port, bit)    ((port) &= ~_BV(bit))

static bool mac_sleeping = true;
static bool mac_lna_enabled = true;
static bool mac_diversity_enabled = true;
static uint8_t mac_tx_power_level = MAC_MAX_TX_POWER_LEVEL;
static uint8_t mac_selected_antenna = 0;

mac_result_t mac_realizePowerStates() {
    SET_BIT(DDRF,0); // RF_ON
    SET_BIT(DDRF,1); // CPS

    if(mac_sleeping) {
        // disable external front-end
        CLR_BIT(PORTF,0);
        CLR_BIT(PORTF,1);

        // disable PA_EXT_EN and PLL_TX_FLT
        CLR_BIT(TRX_CTRL_1, PA_EXT_EN);
        CLR_BIT(TRX_CTRL_1, PLL_TX_FLT);

        // clear CTX
        SET_BIT(DDRG,0);
        CLR_BIT(PORTG,0);

	// disable antenna switch
	CLR_BIT(ANT_DIV,ANT_EXT_SW_EN);
	ANT_DIV = (ANT_DIV & ~0x3) | 0x3;
        SET_BIT(DDRG,1);	
        CLR_BIT(PORTG,1);	
        SET_BIT(DDRF,2);	
        CLR_BIT(PORTF,2);	
    }
    else {
        // enable PA (at least bypass mode)
        SET_BIT(PORTF,0);

        // set state of external RX LNA
        if(mac_lna_enabled) {
            SET_BIT(PORTF,1);
        }
        else {
            CLR_BIT(PORTF,1);
        }

        // calculate power level
	// level 16-31 are mapped to 0-15 with enabled TX PA
        uint8_t power_level = mac_tx_power_level;
        if(power_level > 15) {
            power_level -= 16;
	
	    // enable external TX PA and TX filter
	    // DIG3 switches automatically when in TX mode
            SET_BIT(TRX_CTRL_1, PA_EXT_EN);
            SET_BIT(TRX_CTRL_1, PLL_TX_FLT);
        }
	else {
            // disable PA_EXT_EN and PLL_TX_FLT
            CLR_BIT(TRX_CTRL_1, PA_EXT_EN);
            CLR_BIT(TRX_CTRL_1, PLL_TX_FLT);
	}
        	
        // set power level
        PHY_TX_PWR = 0xF & (15 - power_level);

	// set antenna diversity state
	SET_BIT(ANT_DIV,ANT_EXT_SW_EN);
	if(mac_diversity_enabled) {
	    // enable automatic diversity
	    SET_BIT(ANT_DIV,ANT_DIV_EN);
	}
	else {
	    // disable automatic diversity
	    CLR_BIT(ANT_DIV,ANT_DIV_EN);

            // select specific antenna
	    ANT_DIV = (ANT_DIV & ~0x3) | (0x2-mac_selected_antenna);
	}
    }

    return MAC_SUCCESS;
}

uint8_t mac_getMinTxPowerLvl() {
    return MAC_MIN_TX_POWER_LEVEL;
}

uint8_t mac_getMaxTxPowerLvl() {
    return MAC_MAX_TX_POWER_LEVEL;
}

mac_result_t mac_setTxPower(uint8_t pwrLevel) {
    if (pwrLevel < MAC_MIN_TX_POWER_LEVEL || pwrLevel > MAC_MAX_TX_POWER_LEVEL) {
        return MAC_ERROR_FAIL;
    }

    mac_tx_power_level = pwrLevel;
    return mac_realizePowerStates();
}

uint8_t mac_getTxPower() {
    return mac_tx_power_level;
}

mac_result_t mac_setRxLnaState(bool enable) {
    mac_lna_enabled = enable;
    return mac_realizePowerStates();
}

mac_result_t mac_on() {
    mac_sleeping = false;
    return mac_realizePowerStates();
}

mac_result_t mac_sleep() {
    mac_sleeping = true;
    return mac_realizePowerStates();
}

mac_result_t mac_enableAutomaticDiversity() {
    mac_diversity_enabled = true;
    return mac_realizePowerStates();
}

mac_result_t mac_disableAutomaticDiversity(uint8_t selected_antenna) {
    if(selected_antenna != 0 && selected_antenna != 1) {
      return MAC_ERROR_FAIL;
    }

    mac_diversity_enabled = false;
    mac_selected_antenna = selected_antenna;
    return mac_realizePowerStates();
}
