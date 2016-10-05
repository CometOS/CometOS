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
 * Based on Texas Instruments CC11xL Api
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @author Emil Kallias
 */

#ifndef CC11XLREGISTERS_H_
#define CC11XLREGISTERS_H_

/******************************************************************************
 * CONSTANTS
 */

/* Register addresses for CC110L */
#define CC110L_IOCFG2           0x00      /*  IOCFG2        - GDO2 output pin configuration  */
#define CC110L_IOCFG1           0x01      /*  IOCFG1        - GDO1 output pin configuration  */
#define CC110L_IOCFG0           0x02      /*  IOCFG1        - GDO0 output pin configuration  */
#define CC110L_FIFOTHR          0x03      /*  FIFOTHR       - RX FIFO and TX FIFO thresholds */
#define CC110L_SYNC1            0x04      /*  SYNC1         - Sync word, high byte */
#define CC110L_SYNC0            0x05      /*  SYNC0         - Sync word, low byte */
#define CC110L_PKTLEN           0x06      /*  PKTLEN        - Packet length */
#define CC110L_PKTCTRL1         0x07      /*  PKTCTRL1      - Packet automation control */
#define CC110L_PKTCTRL0         0x08      /*  PKTCTRL0      - Packet automation control */
#define CC110L_ADDR             0x09      /*  ADDR          - Device address */
#define CC110L_CHANNR           0x0A      /*  CHANNR        - Channel Number */
#define CC110L_RESERVED_0X0A    0x0A      /*  RESERVED_0X0A - Reserved register */
#define CC110L_FSCTRL1          0x0B      /*  FSCTRL1       - Frequency synthesizer control */
#define CC110L_FSCTRL0          0x0C      /*  FSCTRL0       - Frequency synthesizer control */
#define CC110L_FREQ2            0x0D      /*  FREQ2         - Frequency control word, high byte */
#define CC110L_FREQ1            0x0E      /*  FREQ1         - Frequency control word, middle byte */
#define CC110L_FREQ0            0x0F      /*  FREQ0         - Frequency control word, low byte */
#define CC110L_MDMCFG4          0x10      /*  MDMCFG4       - Modem configuration */
#define CC110L_MDMCFG3          0x11      /*  MDMCFG3       - Modem configuration */
#define CC110L_MDMCFG2          0x12      /*  MDMCFG2       - Modem configuration */
#define CC110L_MDMCFG1          0x13      /*  MDMCFG1       - Modem configuration */
#define CC110L_MDMCFG0          0x14      /*  MDMCFG1       - Modem configuration */
#define CC110L_DEVIATN          0x15      /*  DEVIATN       - Modem deviation setting */
#define CC110L_MCSM2            0x16      /*  MCSM2         - Main Radio Control State Machine configuration */
#define CC110L_MCSM1            0x17      /*  MCSM1         - Main Radio Control State Machine configuration */
#define CC110L_MCSM0            0x18      /*  MCSM0         - Main Radio Control State Machine configuration */
#define CC110L_FOCCFG           0x19      /*  FOCCFG        - Frequency Offset Compensation configuration */
#define CC110L_BSCFG            0x1A      /*  BSCFG         - Bit Synchronization configuration */
#define CC110L_AGCCTRL2         0x1B      /*  AGCCTRL2      - AGC control */
#define CC110L_AGCCTRL1         0x1C      /*  AGCCTRL1      - AGC control */
#define CC110L_AGCCTRL0         0x1D      /*  AGCCTRL0      - AGC control */
#define CC110L_RESERVED_0X20    0x20      /*  RESERVED_0X20 - Reserved register */
#define CC110L_FREND1           0x21      /*  FREND1        - Front end RX configuration */
#define CC110L_FREND0           0x22      /*  FREDN0        - Front end TX configuration */
#define CC110L_FSCAL3           0x23      /*  FSCAL3        - Frequency synthesizer calibration */
#define CC110L_FSCAL2           0x24      /*  FSCAL2        - Frequency synthesizer calibration */
#define CC110L_FSCAL1           0x25      /*  FSCAL1        - Frequency synthesizer calibration */
#define CC110L_FSCAL0           0x26      /*  FSCAL0        - Frequency synthesizer calibration */
#define CC110L_RESERVED_0X29    0x29      /*  RESERVED_0X29 - Reserved register */
#define CC110L_RESERVED_0X2A    0x2A      /*  RESERVED_0X2A - Reserved register */
#define CC110L_RESERVED_0X2B    0x2B      /*  RESERVED_0X2B - Reserved register */
#define CC110L_TEST2            0x2C      /*  TEST2         - Various test settings */
#define CC110L_TEST1            0x2D      /*  TEST1         - Various test settings */
#define CC110L_TEST0            0x2E      /*  TEST0         - Various test settings */
#define CC110L_PA_TABLE0        0x3E           /*  PA_TABLE0 - PA control settings table */

/* Register addresses for CC113L */
#define CC113L_IOCFG2           0x00      /*  IOCFG2        - GDO2 output pin configuration  */
#define CC113L_IOCFG1           0x01      /*  IOCFG1        - GDO1 output pin configuration  */
#define CC113L_IOCFG0           0x02      /*  IOCFG1        - GDO0 output pin configuration  */
#define CC113L_FIFOTHR          0x03      /*  FIFOTHR       - RX FIFO and TX FIFO thresholds */
#define CC113L_SYNC1            0x04      /*  SYNC1         - Sync word, high byte */
#define CC113L_SYNC0            0x05      /*  SYNC0         - Sync word, low byte */
#define CC113L_PKTLEN           0x06      /*  PKTLEN        - Packet length */
#define CC113L_PKTCTRL1         0x07      /*  PKTCTRL1      - Packet automation control */
#define CC113L_PKTCTRL0         0x08      /*  PKTCTRL0      - Packet automation control */
#define CC113L_ADDR             0x09      /*  ADDR          - Device address */
#define CC113L_RESERVED_0X0A    0x0A      /*  RESERVED_0X0A - Reserved register */
#define CC113L_FSCTRL1          0x0B      /*  FSCTRL1       - Frequency synthesizer control */
#define CC113L_FSCTRL0          0x0C      /*  FSCTRL0       - Frequency synthesizer control */
#define CC113L_FREQ2            0x0D      /*  FREQ2         - Frequency control word, high byte */
#define CC113L_FREQ1            0x0E      /*  FREQ1         - Frequency control word, middle byte */
#define CC113L_FREQ0            0x0F      /*  FREQ0         - Frequency control word, low byte */
#define CC113L_MDMCFG4          0x10      /*  MDMCFG4       - Modem configuration */
#define CC113L_MDMCFG3          0x11      /*  MDMCFG3       - Modem configuration */
#define CC113L_MDMCFG2          0x12      /*  MDMCFG2       - Modem configuration */
#define CC113L_MDMCFG1          0x13      /*  MDMCFG1       - Modem configuration */
#define CC113L_MDMCFG0          0x14      /*  MDMCFG1       - Modem configuration */
#define CC113L_DEVIATN          0x15      /*  DEVIATN       - Modem deviation setting */
#define CC113L_MCSM2            0x16      /*  MCSM2         - Main Radio Control State Machine configuration */
#define CC113L_MCSM1            0x17      /*  MCSM1         - Main Radio Control State Machine configuration */
#define CC113L_MCSM0            0x18      /*  MCSM0         - Main Radio Control State Machine configuration */
#define CC113L_FOCCFG           0x19      /*  FOCCFG        - Frequency Offset Compensation configuration */
#define CC113L_BSCFG            0x1A      /*  BSCFG         - Bit Synchronization configuration */
#define CC113L_AGCCTRL2         0x1B      /*  AGCCTRL2      - AGC control */
#define CC113L_AGCCTRL1         0x1C      /*  AGCCTRL1      - AGC control */
#define CC113L_AGCCTRL0         0x1D      /*  AGCCTRL0      - AGC control */
#define CC113L_RESERVED_0X20    0x20      /*  RESERVED_0X20 - Reserved register */
#define CC113L_FREND1           0x21      /*  FREND1        - Front end RX configuration */
#define CC113L_FSCAL3           0x23      /*  FSCAL3        - Frequency synthesizer calibration */
#define CC113L_FSCAL2           0x24      /*  FSCAL2        - Frequency synthesizer calibration */
#define CC113L_FSCAL1           0x25      /*  FSCAL1        - Frequency synthesizer calibration */
#define CC113L_FSCAL0           0x26      /*  FSCAL0        - Frequency synthesizer calibration */
#define CC113L_RESERVED_0X29    0x29      /*  RESERVED_0X29 - Reserved register */
#define CC113L_RESERVED_0X2A    0x2A      /*  RESERVED_0X2A - Reserved register */
#define CC113L_RESERVED_0X2B    0x2B      /*  RESERVED_0X2B - Reserved register */
#define CC113L_TEST2            0x2C      /*  TEST2         - Various test settings */
#define CC113L_TEST1            0x2D      /*  TEST1         - Various test settings */
#define CC113L_TEST0            0x2E      /*  TEST0         - Various test settings */

/* Register addresses for CC115L */
#define CC115L_IOCFG2           0x00      /*  IOCFG2        - GDO2 output pin configuration  */
#define CC115L_IOCFG1           0x01      /*  IOCFG1        - GDO1 output pin configuration  */
#define CC115L_IOCFG0           0x02      /*  IOCFG1        - GDO0 output pin configuration  */
#define CC115L_FIFOTHR          0x03      /*  FIFOTHR       - RX FIFO and TX FIFO thresholds */
#define CC115L_SYNC1            0x04      /*  SYNC1         - Sync word, high byte */
#define CC115L_SYNC0            0x05      /*  SYNC0         - Sync word, low byte */
#define CC115L_PKTLEN           0x06      /*  PKTLEN        - Packet length */
#define CC115L_PKTCTRL0         0x08      /*  PKTCTRL0      - Packet automation control */
#define CC115L_RESERVED_0X0A    0x0A      /*  RESERVED_0X0A - Reserved register */
#define CC115L_FSCTRL0          0x0C      /*  FSCTRL0       - Frequency synthesizer control */
#define CC115L_FREQ2            0x0D      /*  FREQ2         - Frequency control word, high byte */
#define CC115L_FREQ1            0x0E      /*  FREQ1         - Frequency control word, middle byte */
#define CC115L_FREQ0            0x0F      /*  FREQ0         - Frequency control word, low byte */
#define CC115L_MDMCFG4          0x10      /*  MDMCFG4       - Modem configuration */
#define CC115L_MDMCFG3          0x11      /*  MDMCFG3       - Modem configuration */
#define CC115L_MDMCFG2          0x12      /*  MDMCFG2       - Modem configuration */
#define CC115L_MDMCFG1          0x13      /*  MDMCFG1       - Modem configuration */
#define CC115L_MDMCFG0          0x14      /*  MDMCFG1       - Modem configuration */
#define CC115L_DEVIATN          0x15      /*  DEVIATN       - Modem deviation setting */
#define CC115L_MCSM1            0x17      /*  MCSM1         - Main Radio Control State Machine configuration */
#define CC115L_MCSM0            0x18      /*  MCSM0         - Main Radio Control State Machine configuration */
#define CC115L_RESERVED_0X20    0x20      /*  RESERVED_0X20 - Reserved register */
#define CC115L_FREND0           0x22      /*  FREDN0        - Front end TX configuration */
#define CC115L_FSCAL3           0x23      /*  FSCAL3        - Frequency synthesizer calibration */
#define CC115L_FSCAL2           0x24      /*  FSCAL2        - Frequency synthesizer calibration */
#define CC115L_FSCAL1           0x25      /*  FSCAL1        - Frequency synthesizer calibration */
#define CC115L_FSCAL0           0x26      /*  FSCAL0        - Frequency synthesizer calibration */
#define CC115L_RESERVED_0X29    0x29      /*  RESERVED_0X29 - Reserved register */
#define CC115L_RESERVED_0X2A    0x2A      /*  RESERVED_0X2A - Reserved register */
#define CC115L_RESERVED_0X2B    0x2B      /*  RESERVED_0X2B - Reserved register */
#define CC115L_TEST2            0x2C      /*  TEST2         - Various test settings */
#define CC115L_TEST1            0x2D      /*  TEST1         - Various test settings */
#define CC115L_TEST0            0x2E      /*  TEST0         - Various test settings */
#define CC115L_PA_TABLE0        0x3E           /*  PA_TABLE0 - PA control settings table */

/* status registers CC110L */
#define CC110L_PARTNUM          0x30      /*  PARTNUM         - Chip ID */
#define CC110L_VERSION          0x31      /*  VERSION         - Chip ID */
#define CC110L_FREQEST          0x32      /*  FREQEST         - Frequency Offset Estimate from demodulator */
#define CC110L_LQI              0x33      /*  LQI             - Demodulator estimate for Link Quality */
#define CC110L_RSSI             0x34      /*  RSSI            - Received signal strength indication */
#define CC110L_MARCSTATE        0x35      /*  MARCSTATE       - Main Radio Control State Machine state */
#define CC110L_RESERVED_0X36    0x36      /*  RESERVED_0X36   - Reserved register */
#define CC110L_RESERVED_0X37    0x37      /*  RESERVED_0X37   - Reserved register */
#define CC110L_PKTSTATUS        0x38      /*  PKTSTATUS       - Current GDOx status and packet status */
#define CC110L_RESERVED_0X39    0x39      /*  RESERVED_0X39   - Reserved register */
#define CC110L_TXBYTES          0x3A      /*  TXBYTES         - Underflow and number of bytes */
#define CC110L_RXBYTES          0x3B      /*  RXBYTES         - Overflow and number of bytes */
#define CC110L_RESERVED_0X3C    0x3C      /*  RESERVED_0X3C   - Reserved register */
#define CC110L_RESERVED_0X3D    0x3D      /*  RESERVED_0X3D   - Reserved register */

/* status registers CC113L */
#define CC113L_PARTNUM          0x30      /*  PARTNUM         - Chip ID */
#define CC113L_VERSION          0x31      /*  VERSION         - Chip ID */
#define CC113L_FREQEST          0x32      /*  FREQEST         - Frequency Offset Estimate from demodulator */
#define CC113L_LQI              0x33      /*  LQI             - Demodulator estimate for Link Quality */
#define CC113L_RSSI             0x34      /*  RSSI            - Received signal strength indication */
#define CC113L_MARCSTATE        0x35      /*  MARCSTATE       - Main Radio Control State Machine state */
#define CC113L_RESERVED_0X36    0x36      /*  RESERVED_0X36   - Reserved register */
#define CC113L_RESERVED_0X37    0x37      /*  RESERVED_0X37   - Reserved register */
#define CC113L_PKTSTATUS        0x38      /*  PKTSTATUS       - Current GDOx status and packet status */
#define CC113L_RESERVED_0X39    0x39      /*  RESERVED_0X39   - Reserved register */
#define CC113L_RESERVED_0X3A    0x3A      /*  RESERVED_0X3A   - Reserved register */
#define CC113L_RXBYTES          0x3B      /*  RXBYTES         - Overflow and number of bytes */
#define CC113L_RESERVED_0X3C    0x3C      /*  RESERVED_0X3C   - Reserved register */
#define CC113L_RESERVED_0X3D    0x3D      /*  RESERVED_0X3D   - Reserved register */

/* status registers CC115L */
#define CC115L_PARTNUM          0x30      /*  PARTNUM         - Chip ID */
#define CC115L_VERSION          0x31      /*  VERSION         - Chip ID */
#define CC115L_RESERVED_0x32    0x32      /*  RESERVED_0X32   - Reserved status register */
#define CC115L_RESERVED_0x33    0x33      /*  RESERVED_0X33   - Reserved status register */
#define CC115L_RESERVED_0x34    0x34      /*  RESERVED_0X34   - Reserved status register */
#define CC115L_MARCSTATE        0x35      /*  MARCSTATE       - Main Radio Control State Machine state */
#define CC115L_RESERVED_0X36    0x36      /*  RESERVED_0X36   - Reserved register */
#define CC115L_RESERVED_0X37    0x37      /*  RESERVED_0X37   - Reserved register */
#define CC115L_PKTSTATUS        0x38      /*  PKTSTATUS       - Current GDOx status and packet status */
#define CC115L_RESERVED_0X39    0x39      /*  RESERVED_0X39   - Reserved register */
#define CC115L_TXBYTES          0x3A      /*  TXBYTES         - Underflow and number of bytes */
#define CC115L_RESERVED_0x3B    0x3B      /*  RESERVED        - Reserved status register */
#define CC115L_RESERVED_0X3C    0x3C      /*  RESERVED_0X3C   - Reserved register */
#define CC115L_RESERVED_0X3D    0x3D      /*  RESERVED_0X3D   - Reserved register */


/* burst write registers */
#define CC11xL_PA_TABLE0   0x3E           /*  PA_TABLE0 - PA control settings table */
#define CC11xL_FIFO        0x3F           /*  FIFO  - Transmit FIFO */

/* Other register bit fields */
#define CC110L_CRC_OK_BM            0x80
#define CC113L_CRC_OK_BM            0x80

#define CC110L_LQI_EST_BM               0x7F
#define CC113L_LQI_EST_BM               0x7F

/* CC11xL Command strobe register*/
#define CC11xL_SNOP                     0x3D      /*  SNOP    - No operation. Returns status byte. */

/* CC110L Command strobe registers */
#define CC110L_SRES                     0x30      /*  SRES    - Reset chip. */
#define CC110L_SFSTXON                  0x31      /*  SFSTXON - Enable and calibrate frequency synthesizer. */
#define CC110L_SXOFF                    0x32      /*  SXOFF   - Turn off crystal oscillator. */
#define CC110L_SCAL                     0x33      /*  SCAL    - Calibrate frequency synthesizer and turn it off. */
#define CC110L_SRX                      0x34      /*  SRX     - Enable RX. Perform calibration if enabled. */
#define CC110L_STX                      0x35      /*  STX     - Enable TX. If in RX state, only enable TX if CCA passes. */
#define CC110L_SIDLE                    0x36      /*  SIDLE   - Exit RX / TX, turn off frequency synthesizer. */
#define CC110L_SPWD                     0x39      /*  SPWD    - Enter power down mode when CSn goes high. */
#define CC110L_SFRX                     0x3A      /*  SFRX    - Flush the RX FIFO buffer. */
#define CC110L_SFTX                     0x3B      /*  SFTX    - Flush the TX FIFO buffer. */
#define CC110L_SNOP                     0x3D      /*  SNOP    - No operation. Returns status byte. */

/* CC113L Command strobe registers */
#define CC113L_SRES                     0x30      /*  SRES    - Reset chip. */
#define CC113L_SXOFF                    0x32      /*  SXOFF   - Turn off crystal oscillator. */
#define CC113L_SCAL                     0x33      /*  SCAL    - Calibrate frequency synthesizer and turn it off. */
#define CC113L_SRX                      0x34      /*  SRX     - Enable RX. Perform calibration if enabled. */
#define CC113L_SIDLE                    0x36      /*  SIDLE   - Exit RX / TX, turn off frequency synthesizer. */
#define CC113L_SPWD                     0x39      /*  SPWD    - Enter power down mode when CSn goes high. */
#define CC113L_SFRX                     0x3A      /*  SFRX    - Flush the RX FIFO buffer. */
#define CC113L_SNOP                     0x3D      /*  SNOP    - No operation. Returns status byte. */

/* CC115L Command Strobe registers */
#define CC115L_SRES                     0x30      /*  SRES    - Reset chip. */
#define CC115L_SFSTXON                  0x31      /*  SFSTXON - Enable and calibrate frequency synthesizer. */
#define CC115L_SXOFF                    0x32      /*  SXOFF   - Turn off crystal oscillator. */
#define CC115L_SCAL                     0x33      /*  SCAL    - Calibrate frequency synthesizer and turn it off. */
#define CC115L_STX                      0x35      /*  STX     - Enable TX. If in RX state, only enable TX if CCA passes. */
#define CC115L_SIDLE                    0x36      /*  SIDLE   - Exit RX / TX, turn off frequency synthesizer. */
#define CC115L_SPWD                     0x39      /*  SPWD    - Enter power down mode when CSn goes high. */
#define CC115L_SFTX                     0x3B      /*  SFTX    - Flush the TX FIFO buffer. */
#define CC115L_SNOP                     0x3D      /*  SNOP    - No operation. Returns status byte. */


/* CC110L Chip states returned in status byte */
#define CC110L_STATE_IDLE               0x00
#define CC110L_STATE_RX                 0x10
#define CC110L_STATE_TX                 0x20
#define CC110L_STATE_FSTXON             0x30
#define CC110L_STATE_CALIBRATE          0x40
#define CC110L_STATE_SETTLING           0x50
#define CC110L_STATE_RXFIFO_ERROR       0x60
#define CC110L_STATE_TXFIFO_ERROR       0x70

/* CC113L Chip states returned in status byte */
#define CC113L_STATE_IDLE               0x00
#define CC113L_STATE_RX                 0x10
#define CC113L_STATE_CALIBRATE          0x40
#define CC113L_STATE_SETTLING           0x50
#define CC113L_STATE_RXFIFO_ERROR       0x60


/* CC115L Chip states returned in status byte */
#define CC115L_STATE_IDLE               0x00
#define CC115L_STATE_TX                 0x20
#define CC115L_STATE_FSTXON             0x30
#define CC115L_STATE_CALIBRATE          0x40
#define CC115L_STATE_SETTLING           0x50
#define CC115L_STATE_TXFIFO_ERROR       0x70

#endif
