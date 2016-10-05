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

#ifndef AT86RF231_REGISTERS_H
#define AT86RF231_REGISTERS_H

#define AT86RF231_ACCESS_REG                            0x80
#define AT86RF231_ACCESS_FRAMEBUFFER                    0x20
#define AT86RF231_ACCESS_SRAM                           0x00

#define AT86RF231_ACCESS_READ                           0x00
#define AT86RF231_ACCESS_WRITE                          0x40

#define AT86RF231_REG_TRX_STATUS                        0x01
#define AT86RF231_REG_TRX_STATE                         0x02
#define AT86RF231_REG_TRX_CTRL_0                        0x03
#define AT86RF231_REG_TRX_CTRL_1                        0x04
#define AT86RF231_REG_PHY_TX_PWR                        0x05
#define AT86RF231_REG_PHY_RSSI                          0x06
#define AT86RF231_REG_PHY_ED_LEVEL                      0x07
#define AT86RF231_REG_PHY_CC_CCA                        0x08
#define AT86RF231_REG_CCA_THRES                         0x09
#define AT86RF231_REG_RX_CTRL                           0x0A
#define AT86RF231_REG_SFD_VALUE                         0x0B
#define AT86RF231_REG_TRX_CTRL_2                        0x0C
#define AT86RF231_REG_ANT_DIV                           0x0D
#define AT86RF231_REG_IRQ_MASK                          0x0E
#define AT86RF231_REG_IRQ_STATUS                        0x0F
#define AT86RF231_REG_VREG_CTRL                         0x10
#define AT86RF231_REG_BATMON                            0x11
#define AT86RF231_REG_XOSC_CTRL                         0x12

#define AT86RF231_REG_CC_CTRL_0                         0x13
#define AT86RF231_REG_CC_CTRL_1                         0x14


#define AT86RF231_REG_RX_SYN                            0x15

#define AT86RF231_REG_XAH_CTRL_1                        0x17
#define AT86RF231_REG_FTN_CTRL                          0x18

#define AT86RF231_REG_PLL_CF                            0x1A
#define AT86RF231_REG_PLL_DCU                           0x1B
#define AT86RF231_REG_PART_NUM                          0x1C
#define AT86RF231_REG_VERSION_NUM                       0x1D
#define AT86RF231_REG_MAN_ID_0                          0x1E
#define AT86RF231_REG_MAN_ID_1                          0x1F
#define AT86RF231_REG_SHORT_ADDR_0                      0x20
#define AT86RF231_REG_SHORT_ADDR_1                      0x21
#define AT86RF231_REG_PAN_ID_0                          0x22
#define AT86RF231_REG_PAN_ID_1                          0x23

#define AT86RF231_REG_IEEE_ADDR_0                       0x24
#define AT86RF231_REG_IEEE_ADDR_1                       0x25
#define AT86RF231_REG_IEEE_ADDR_2                       0x26
#define AT86RF231_REG_IEEE_ADDR_3                       0x27
#define AT86RF231_REG_IEEE_ADDR_4                       0x28
#define AT86RF231_REG_IEEE_ADDR_5                       0x29
#define AT86RF231_REG_IEEE_ADDR_6                       0x2A
#define AT86RF231_REG_IEEE_ADDR_7                       0x2B

#define AT86RF231_REG_XAH_CTRL_0                        0x2C
#define AT86RF231_REG_CSMA_SEED_0                       0x2D
#define AT86RF231_REG_CSMA_SEED_1                       0x2E
#define AT86RF231_REG_CSMA_BE                           0x2F

#define AT86RF231_REG_TST_CTRL_DIGI                     0x36

#define AT86RF231_TRX_CTRL_0_MASK_PAD_IO                0xC0
#define AT86RF231_TRX_CTRL_0_MASK_PAD_IO_CLKM           0x30
#define AT86RF231_TRX_CTRL_0_MASK_CLKM_SHA_SEL          0x08
#define AT86RF231_TRX_CTRL_0_MASK_CLKM_CTRL             0x07

#define AT86RF231_TRX_CTRL_0_DEFAULT_PAD_IO             0x00
#define AT86RF231_TRX_CTRL_0_DEFAULT_PAD_IO_CLKM        0x10
#define AT86RF231_TRX_CTRL_0_DEFAULT_CLKM_SHA_SEL       0x08
#define AT86RF231_TRX_CTRL_0_DEFAULT_CLKM_CTRL          0x01

#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_OFF              0x00
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_1MHz             0x01
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_2MHz             0x02
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_4MHz             0x03
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_8MHz             0x04
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_16MHz            0x05
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_250kHz           0x06
#define AT86RF231_TRX_CTRL_0_CLKM_CTRL_62_5kHz          0x07

#define AT86RF231_RX_SYN_PDT_DIS_MASK                   0x80
#define AT86RF231_RX_SYN_PDT_LEVEL_MASK                 0x0F

#define AT86RF231_TRX_CTRL_1_MASK_PA_EXT_EN             0x80
#define AT86RF231_TRX_CTRL_1_MASK_IRQ_2_EXT_EN          0x40
#define AT86RF231_TRX_CTRL_1_MASK_TX_AUTO_CRC_ON        0x20
#define AT86RF231_TRX_CTRL_1_MASK_RX_BL_CTRL            0x10
#define AT86RF231_TRX_CTRL_1_MASK_SPI_CMD_MODE          0x0C
#define AT86RF231_TRX_CTRL_1_MASK_IRQ_MASK_MODE         0x02
#define AT86RF231_TRX_CTRL_1_MASK_IRQ_POLARITY          0x01

#define AT86RF231_TRX_CTRL_2_MASK_RX_SAFE_MODE          0x80
#define AT86RF231_TRX_CTRL_2_MASK_OQPSK_DATA_RATE       0x03

#define AT86RF231_TRX_CTRL_2_OQPSK_DATA_RATE_100_250    0x00
#define AT86RF231_TRX_CTRL_2_OQPSK_DATA_RATE_200_500    0x01
#define AT86RF231_TRX_CTRL_2_OQPSK_DATA_RATE_400_1000   0x02
#define AT86RF231_TRX_CTRL_2_OQPSK_DATA_RATE__2000      0x03

#define AT86RF231_IRQ_STATUS_MASK_BAT_LOW               0x80
#define AT86RF231_IRQ_STATUS_MASK_TRX_UR                0x40
#define AT86RF231_IRQ_STATUS_MASK_AMI                   0x20
#define AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE           0x10
#define AT86RF231_IRQ_STATUS_MASK_TRX_END               0x08
#define AT86RF231_IRQ_STATUS_MASK_RX_START              0x04
#define AT86RF231_IRQ_STATUS_MASK_PLL_UNLOCK            0x02
#define AT86RF231_IRQ_STATUS_MASK_PLL_LOCK              0x01

#define AT86RF231_TRX_STATUS_MASK_CCA_DONE              0x80
#define AT86RF231_TRX_STATUS_MASK_CCA_STATUS            0x40
#define AT86RF231_TRX_STATUS_MASK_TRX_STATUS            0x1F

#define AT86RF231_TRX_STATUS_P_ON                       0x00
#define AT86RF231_TRX_STATUS_BUSY_RX                    0x01
#define AT86RF231_TRX_STATUS_BUSY_TX                    0x02
#define AT86RF231_TRX_STATUS_RX_ON                      0x06
#define AT86RF231_TRX_STATUS_TRX_OFF                    0x08
#define AT86RF231_TRX_STATUS_PLL_ON                     0x09
#define AT86RF231_TRX_STATUS_SLEEP                      0x0F
#define AT86RF231_TRX_STATUS_BUSY_RX_AACK               0x11
#define AT86RF231_TRX_STATUS_BUSY_TX_ARET               0x12
#define AT86RF231_TRX_STATUS_RX_AACK_ON                 0x16
#define AT86RF231_TRX_STATUS_TX_ARET_ON                 0x19
#define AT86RF231_TRX_STATUS_RX_ON_NOCLK                0x1C
#define AT86RF231_TRX_STATUS_RX_AACK_ON_NOCLK           0x1D
#define AT86RF231_TRX_STATUS_BUSY_RX_AACK_NOCLK         0x1E
#define AT86RF231_TRX_STATUS_STATE_TRANS_IN_PROGRESS    0x1F

#define AT86RF231_TRX_STATE_NOP                         0x00
#define AT86RF231_TRX_STATE_TX_START                    0x02
#define AT86RF231_TRX_STATE_FORCE_TRX_OFF               0x03
#define AT86RF231_TRX_STATE_FORCE_PLL_ON                0x04
#define AT86RF231_TRX_STATE_RX_ON                       0x06
#define AT86RF231_TRX_STATE_TRX_OFF                     0x08
#define AT86RF231_TRX_STATE_PLL_ON                      0x09
#define AT86RF231_TRX_STATE_RX_AACK_ON                  0x16
#define AT86RF231_TRX_STATE_TX_ARET_ON                  0x19

#define AT86RF231_PHY_CC_CCA_MASK_CCA_REQUEST           0x80
#define AT86RF231_PHY_CC_CCA_MASK_CCA_MODE              0x60
#define AT86RF231_PHY_CC_CCA_MASK_CHANNEL               0x1F

#define AT86RF231_PHY_CC_CCA_DEFAULT_CCA_MODE           0x20

#define AT86RF231_PHY_CC_CCA_MODE0                      0x05

#define AT86RF231_PHY_TX_PWR_MASK_PA_BUF_LT             0xC0
#define AT86RF231_PHY_TX_PWR_MASK_PA_LT                 0x30
#define AT86RF231_PHY_TX_PWR_MASK_TX_PWR                0x0F

#define AT86RF231_PHY_TX_PWR_DEFAULT_PA_BUF_LT          0xC0
#define AT86RF231_PHY_TX_PWR_DEFAULT_PA_LT              0x00
#define AT86RF231_PHY_TX_PWR_DEFAULT_TX_PWR             0x00

#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_3dBm          0x00
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_2_8dBm        0x01
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_2_3dBm        0x02
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_1_8dBm        0x03
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_1_3dBm        0x04
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_0_7dBm        0x05
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_0dBm          0x06
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m1dBm         0x07
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m2dBm         0x08
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m3dBm         0x09
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m4dBm         0x0A
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m5dBm         0x0B
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m7dBm         0x0C
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m9dBm         0x0D
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m12dBm        0x0E
#define AT86RF231_PHY_TX_PWR_TX_PWR_VALUE_m17dBm        0x0F

#define AT86RF231_PHY_RX_THRESHOLD_m101dBm              0x0
#define AT86RF231_PHY_RX_THRESHOLD_m90dBm               0x1
#define AT86RF231_PHY_RX_THRESHOLD_m87dBm               0x2
#define AT86RF231_PHY_RX_THRESHOLD_m84dBm               0x3
#define AT86RF231_PHY_RX_THRESHOLD_m81dBm               0x4
#define AT86RF231_PHY_RX_THRESHOLD_m78dBm               0x5
#define AT86RF231_PHY_RX_THRESHOLD_m75dBm               0x6
#define AT86RF231_PHY_RX_THRESHOLD_m72dBm               0x7
#define AT86RF231_PHY_RX_THRESHOLD_m69dBm               0x8
#define AT86RF231_PHY_RX_THRESHOLD_m66dBm               0x9
#define AT86RF231_PHY_RX_THRESHOLD_m63dBm               0xA
#define AT86RF231_PHY_RX_THRESHOLD_m60dBm               0xB
#define AT86RF231_PHY_RX_THRESHOLD_m57dBm               0xC
#define AT86RF231_PHY_RX_THRESHOLD_m54dBm               0xD
#define AT86RF231_PHY_RX_THRESHOLD_m51dBm               0xE
#define AT86RF231_PHY_RX_THRESHOLD_m48dBm               0xF

#define AT86RF231_PHY_RSSI_MASK_RX_CRC_VALID            0x80
#define AT86RF231_PHY_RSSI_MASK_RND_VALUE               0x60
#define AT86RF231_PHY_RSSI_RND_0                        0x05
#define AT86RF231_PHY_RSSI_MASK_RSSI                    0x1F

#define AT86RF231_XOSC_CTRL_XTAL_MODE_CRYSTAL           0xF0
#define AT86RF231_XOSC_CTRL_XTAL_MODE_EXTERNAL          0xF0

#define AT86RF231_TIMING__VCC_TO_P_ON                   330
#define AT86RF231_TIMING__SLEEP_TO_TRX_OFF              380
#define AT86RF231_TIMING__TRX_OFF_TO_PLL_ON             110
#define AT86RF231_TIMING__TRX_OFF_TO_RX_ON              110
#define AT86RF231_TIMING__PLL_ON_TO_BUSY_TX             16

#define AT86RF231_TIMING__RESET                         100
#define AT86RF231_TIMING__RESET_TO_TRX_OFF              37

#endif
