// Portions Copyright () 2023 OpenBuffalo
/**
  ******************************************************************************
  * @file    usb_v2_reg.h
  * @version V1.0
  * @date    2022-08-15
  * @brief   This file is the description of.IP register
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 Bouffalo Lab</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Bouffalo Lab nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __OBLFR_USBPHY_H__

#include <stdint.h>

// SUBSYSTEMS
#define BLFB_USB_BASE ((uint32_t)0x20072000)
#define BFLB_PDS_BASE ((uint32_t)0x2000e000)

// PDS REGISTERS
#define PDS_USB_CTL_OFFSET      (0x500) /* usb_ctl */
#define PDS_USB_PHY_CTRL_OFFSET (0x504) /* usb_phy_ctrl */

// PDS REGISTER VALUES

/* 0x500 : usb_ctl */
#define PDS_REG_USB_SW_RST_N   (1 << 0U)
#define PDS_REG_USB_EXT_SUSP_N (1 << 1U)
#define PDS_REG_USB_WAKEUP     (1 << 2U)
#define PDS_REG_USB_L1_WAKEUP  (1 << 3U)
#define PDS_REG_USB_DRVBUS_POL (1 << 4U)
#define PDS_REG_USB_IDDIG      (1 << 5U)

/* 0x504 : usb_phy_ctrl */
#define PDS_REG_USB_PHY_PONRST       (1 << 0U)
#define PDS_REG_USB_PHY_OSCOUTEN     (1 << 1U)
#define PDS_REG_USB_PHY_XTLSEL_SHIFT (2U)
#define PDS_REG_USB_PHY_XTLSEL_MASK  (0x3 << PDS_REG_USB_PHY_XTLSEL_SHIFT)
#define PDS_REG_USB_PHY_XTLSEL_MASK  (0x3 << PDS_REG_USB_PHY_XTLSEL_SHIFT)
#define PDS_REG_USB_PHY_OUTCLKSEL    (1 << 4U)
#define PDS_REG_USB_PHY_PLLALIV      (1 << 5U)
#define PDS_REG_PU_USB20_PSW         (1 << 6U)

void bflb_usb_phy_init(void);

// USB REGS

#define USB_GLB_INT_OFFSET          (0xC4)  /* GLB_INT */


/* 0xC4 : GLB_INT */
#define USB_MDEV_INT (1 << 0U)
#define USB_MOTG_INT (1 << 1U)
#define USB_MHC_INT  (1 << 2U)
#define USB_POLARITY (1 << 3U)


#endif
