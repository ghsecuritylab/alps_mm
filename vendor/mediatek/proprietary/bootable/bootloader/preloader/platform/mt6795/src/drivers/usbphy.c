/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include "platform.h"
#include "usbphy.h"
#include "usbd.h"
#include "pll.h"

#include "ssusb_sifslv_ippc_c_header.h"

#define USB11PHY_READ8(offset)		__raw_readb(USB11_PHY_BASE + offset)
#define USB11PHY_WRITE8(offset, value)	__raw_writeb(value, USB11_PHY_BASE+offset)
#define USB11PHY_SET8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) | mask)
#define USB11PHY_CLR8(offset, mask)	USB11PHY_WRITE8(offset, USB11PHY_READ8(offset) & ~mask)

#include "mtk-phy-d60802.h"
#if CFG_FPGA_PLATFORM
#define U3_PHY_PAGE		0xff
#define I2C_CHIP		0xc0

static struct mt_i2c_t usb_i2c;

#define DEBUG 0
#if DEBUG
	#define PHY_LOG print
#else
	#define PHY_LOG
#endif

#define MYI2C 0

#if MYI2C
#define REG_I2C_START_BIT	0x1
#define I2C_READ_BIT	 	0x1

#define PHY_I2C_BASE		(0x11008000)
#define REG_I2C_DATA_PORT	(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x00)))
#define REG_I2C_SLAVE_ADDR	(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x04)))
#define REG_I2C_TRANSFER_LEN	(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x14)))
#define REG_I2C_START		(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x24)))
#define REG_I2C_SOFT_RESET	(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x50)))
#define REG_I2C_CONTROL		(*((volatile unsigned short int *) (PHY_I2C_BASE + 0x10)))

u32 i2c_write_reg(u8 dev_id, u8 addr, u8 val)
{
	REG_I2C_SLAVE_ADDR = dev_id<<1;
	REG_I2C_TRANSFER_LEN = 2;

	REG_I2C_DATA_PORT = addr;
	REG_I2C_DATA_PORT = val;

	REG_I2C_START = REG_I2C_START_BIT;

	while ((REG_I2C_START & REG_I2C_START_BIT));
	return TRUE;
}

u32 i2c_read_reg(u8 dev_id, u8 addr, u8 *data)
{
	REG_I2C_SLAVE_ADDR = dev_id<<1;
	REG_I2C_TRANSFER_LEN = 0x01;
	REG_I2C_DATA_PORT = addr;
	REG_I2C_START = REG_I2C_START_BIT;

	while ((REG_I2C_START & REG_I2C_START_BIT));

	REG_I2C_SLAVE_ADDR = (dev_id << 1) | I2C_READ_BIT;
	REG_I2C_TRANSFER_LEN = 0x01;
	REG_I2C_START = REG_I2C_START_BIT;

	while ((REG_I2C_START & REG_I2C_START_BIT));

	*data = REG_I2C_DATA_PORT;

	return TRUE; /* !!(int)*data; */
}
#endif

#define U3_PHY_I2C_DEV		0x60
u32 usb_i2c_read8(u8 addr, u8 *databuffer)
{
	#if MYI2C
	u32 ret = I2C_OK;

	ret = i2c_read_reg(U3_PHY_I2C_DEV, addr, databuffer);

	return ret;
	#else
	u32 ret_code = I2C_OK;
	u16 len;
	*databuffer = addr;

	usb_i2c.id = USB_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set USB I2C address to 0x60 (0xC0>>1) */
	usb_i2c.addr = 0x60;
	usb_i2c.mode = ST_MODE;
	usb_i2c.speed = 100;
	len = 1;

	ret_code = i2c_write_read(&usb_i2c, databuffer, len, len);

	return ret_code;
	#endif
}

u32 usb_i2c_write8(u8 addr, u8 value)
{
	#if MYI2C
	u32 ret = I2C_OK;

	ret = i2c_write_reg(U3_PHY_I2C_DEV, addr, value);

	return ret;
	#else
	u32 ret_code = I2C_OK;
	u8 write_data[2];
	u16 len;

	write_data[0]= addr;
	write_data[1] = value;

	usb_i2c.id = USB_I2C_ID;
	/* Since i2c will left shift 1 bit, we need to set USB I2C address to 0x60 (0xC0>>1) */
	usb_i2c.addr = 0x60;
	usb_i2c.mode = ST_MODE;
	usb_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&usb_i2c, write_data, len);

	return ret_code;
	#endif
}

void _u3_write_bank(u32 value)
{
	usb_i2c_write8((u8)U3_PHY_PAGE, (u8)value);
}

u32 _u3_read_reg(u32 address)
{
	u8 databuffer = 0;
	usb_i2c_read8((u8)address, &databuffer);
	return databuffer;
}

void _u3_write_reg(u32 address, u32 value)
{
	usb_i2c_write8((u8 )address, (u8 )value);
}

u32 u3_phy_read_reg32(u32 addr)
{
	u32 bank;
	u32 addr8;
	u32 data;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;

	_u3_write_bank(bank);
	data = _u3_read_reg(addr8);
	data |= (_u3_read_reg(addr8 + 1) << 8);
	data |= (_u3_read_reg(addr8 + 2) << 16);
	data |= (_u3_read_reg(addr8 + 3) << 24);
	return data;
}

u32 u3_phy_write_reg32(u32 addr, u32 data) {
	u32 bank;
	u32 addr8;
	u32 data_0, data_1, data_2, data_3;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	data_0 = data & 0xff;
	data_1 = (data >> 8) & 0xff;
	data_2 = (data >> 16) & 0xff;
	data_3 = (data >> 24) & 0xff;

	_u3_write_bank(bank);
	_u3_write_reg(addr8, data_0);
	_u3_write_reg(addr8 + 1, data_1);
	_u3_write_reg(addr8 + 2, data_2);
	_u3_write_reg(addr8 + 3, data_3);

	return 0;
}

void u3_phy_write_field32(int addr, int offset, int mask, int value)
{
	u32 cur_value;
	u32 new_value;
	cur_value = u3_phy_read_reg32(addr);
	new_value = (cur_value & (~mask)) | ((value << offset) & mask);
	//udelay(i2cdelayus);
	u3_phy_write_reg32(addr, new_value);
}

u32 u3_phy_write_reg8(u32 addr, u8 data)
{
	u32 bank;
	u32 addr8;

	PHY_LOG("addr8: %x\n", addr);
	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	_u3_write_bank(bank);
	_u3_write_reg(addr8, data);

	return 0;
}

#define USBPHY_I2C_READ8(addr, buffer)	 usb_i2c_read8(addr, buffer)
#define USBPHY_I2C_WRITE8(addr, value)	 usb_i2c_write8(addr, value)
#endif

#define USBPHY_READ32(offset)		__raw_readl(SSUSB_PHY_BASE+offset)
#define USBPHY_WRITE32(offset, value)	__raw_writel(value, SSUSB_PHY_BASE+offset)
#define USBPHY_SET32(offset, mask)	USBPHY_WRITE32(offset, USBPHY_READ32(offset) | mask)
#define USBPHY_CLR32(offset, mask)	USBPHY_WRITE32(offset, USBPHY_READ32(offset) & ~mask)

#if CFG_FPGA_PLATFORM
u32 u2_slew_rate_calibration_d60802(struct u3phy_info *info)
{
	u32 i = 0;
	u32 ret = 0;
	u32 u4fmout = 0;
	u32 u4tmp = 0;
	volatile u32 chkvalue;

	PHY_LOG("[USBPHY] %s starts\n", __func__);

	/* => RG_USB20_HSTX_SRCAL_EN = 1 */
	/* enable HS TX SR calibration */
	u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr5),
		D60802_RG_USB20_HSTX_SRCAL_EN_OFST, D60802_RG_USB20_HSTX_SRCAL_EN, 1);
	mdelay(1);
	chkvalue= u3_phy_read_reg32(((u32)&info->u2phy_regs_d->usbphyacr5));
	PHY_LOG("chkvalue: %d\n", chkvalue);

	/* => RG_FRCK_EN = 1 */
	/* Enable free run clock */
	u3_phy_write_field32(((u32)&info->sifslv_fm_regs_d->fmmonr1),
		D60802_RG_FRCK_EN_OFST, D60802_RG_FRCK_EN, 0x1);

	/* => RG_CYCLECNT = 4 */
	/* Setting cyclecnt = 4 */
	u3_phy_write_field32(((u32)&info->sifslv_fm_regs_d->fmcr0),
		D60802_RG_CYCLECNT_OFST, D60802_RG_CYCLECNT, 0x4);

	/* => RG_FREQDET_EN = 1 */
	/* Enable frequency meter */
	u3_phy_write_field32(((u32)&info->sifslv_fm_regs_d->fmcr0),
		D60802_RG_FREQDET_EN_OFST, D60802_RG_FREQDET_EN, 0x1);

	/* wait for FM detection done, set 10ms timeout */
	for (i = 0; i < 10; i++) {
		/* => u4fmout = USB_FM_OUT */
		/* read FM_OUT */
		u4fmout = u3_phy_readr_reg32(((u32)&info->sifslv_fm_regs_d->fmmonr0));
		PHY_LOG("FM_OUT value: u4fmout = %d(0x%08X)\n", u4fmout, u4fmout);

		/* check if FM detection done */
		if (u4fmout != 0) {
			ret = 0;
			PHY_LOG("FM detection done! loop = %d\n", i);
			break;
		}

		ret = 1;
		mdelay(1);
	}

	/* => RG_FREQDET_EN = 0 */
	/* disable frequency meter */
	u3_phy_write_field32(((u32)&info->sifslv_fm_regs_d->fmcr0),
		D60802_RG_FREQDET_EN_OFST, D60802_RG_FREQDET_EN, 0);

	/* => RG_FRCK_EN = 0 */
	/* disable free run clock */
	u3_phy_write_field32(((u32)&info->sifslv_fm_regs_d->fmmonr1),
		D60802_RG_FRCK_EN_OFST, D60802_RG_FRCK_EN, 0);

	/* => RG_USB20_HSTX_SRCAL_EN = 0 */
	/* disable HS TX SR calibration */
	u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr0),
		D60802_RG_USB20_HSTX_SRCAL_EN_OFST, D60802_RG_USB20_HSTX_SRCAL_EN, 0);
	mdelay(1);

	if (u4fmout == 0) {
		u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr5),
			D60802_RG_USB20_HSTX_SRCTRL_OFST, D60802_RG_USB20_HSTX_SRCTRL, 0x4);
		ret = 1;
	} else {
		/* set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits) */
		u4tmp = (((1024 * 25 * U2_SR_COEF_D60802) / u4fmout) + 500) / 1000;
		PHY_LOG("SR calibration value u1SrCalVal = %d\n", (u8)u4tmp);
		u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr5),
		D60802_RG_USB20_HSTX_SRCTRL_OFST, D60802_RG_USB20_HSTX_SRCTRL, u4tmp);
	}

	PHY_LOG("[USBPHY] %s ends\n", __func__);

	return ret;
}

void mu3d_hal_pdn_dis(void) {
	USB_CLRMASK(U3D_SSUSB_IP_PW_CTRL2, SSUSB_IP_DEV_PDN);
#ifdef SUPPORT_U3
	USB_CLRMASK(U3D_SSUSB_U3_CTRL_0P, (SSUSB_U3_PORT_DIS | SSUSB_U3_PORT_PDN | SSUSB_U3_PORT_U2_CG_EN));
#endif
	USB_CLRMASK(U3D_SSUSB_U2_CTRL_0P, (SSUSB_U2_PORT_DIS | SSUSB_U2_PORT_PDN | SSUSB_U2_PORT_U2_CG_EN));
}

#define PHY_DRV_SHIFT	3
#define PHY_PHASE_SHIFT	3
#define PHY_PHASE_DRV_SHIFT	1

char u3_phy_read_reg8(u32 addr) {
	int bank;
	int addr8;
	int data;

	bank = (addr >> 16) & 0xff;
	addr8 = addr & 0xff;
	_u3_write_bank(bank);
	data = _u3_read_reg(addr8);

	return data;
}


int phy_change_pipe_phase_d60802(struct u3phy_info *info, int phy_drv, int pipe_phase) {
	int drv_reg_value;
	int phase_reg_value;
	int temp;

	drv_reg_value = phy_drv << PHY_DRV_SHIFT;
	phase_reg_value = (pipe_phase << PHY_PHASE_SHIFT) | (phy_drv << PHY_PHASE_DRV_SHIFT);
	temp = u3_phy_read_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla)+2);
	temp &= ~(0x3 << PHY_DRV_SHIFT);
	temp |= drv_reg_value;
	u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla)+2, temp);
	temp = u3_phy_read_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla)+3);
	temp &= ~((0x3 << PHY_PHASE_DRV_SHIFT) | (0x1f << PHY_PHASE_SHIFT));
	temp |= phase_reg_value;
	u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla)+3, temp);

	return TRUE;
}

#define os_writel(addr, value) __raw_writel(value, addr)

#include "ssusb_usb3_mac_csr_c_header.h"

#define ENTER_U0_TH			5
#define MAX_PHASE_RANGE 		31
#define MAX_TIMEOUT_COUNT 		100
#define DATA_DRIVING_MASK 		0x06
#define MAX_DRIVING_RANGE 		0x04
#define MAX_LATCH_SELECT 		0x02

#define U3_PHY_I2C_PCLK_DRV_REG		0x0A
#define U3_PHY_I2C_PCLK_PHASE_REG	0x0B
#define	STATE_U0_STATE			(13)
#define CLR_RECOV_CNT			(0x1 << 16) /* 16:16 */
#define CLR_LINK_ERR_CNT		(0x1 << 16) /* 16:16 */
#define	STATE_DISABLE			(1)

int mu3d_hal_phy_scan(struct u3phy_info *u3phy, int latch_val) {
	#ifdef CONFIG_U3_PHY_GPIO_SUPPORT
	int count, fset_phase_val, recov_cnt, link_error_count, u0_count;
	u8 phase_val, temp, driving, data_driving_val, clk_driving_val;

	/* disable ip power down,disable U2/U3 ip power down. */
	mu3d_hal_ssusb_en();
	mu3d_hal_pdn_dis();
	phy_change_pipe_phase_d60802(u3phy, 0, 0);
 	__raw_writel(latch_val, U3D_PIPE_LATCH_SELECT); /* set tx/rx latch sel */

	driving = 2;
	phy_change_pipe_phase_d60802(u3phy, driving, driving);
	phase_val = 0;
	count = 0;
	fset_phase_val = TRUE;

	while (TRUE) {

		if (fset_phase_val) {
			phy_change_pipe_phase_d60802(u3phy, driving, phase_val);
			mu3d_hal_rst_dev();
			mdelay(50);
			__raw_writel(USB3_EN, U3D_USB3_CONFIG);
			__raw_writel(latch_val, U3D_PIPE_LATCH_SELECT); /* set tx/rx latch sel */
			fset_phase_val = FALSE;
			u0_count = 0;
			link_error_count = 0;
			recov_cnt = 0;
			count = 0;
		}
		mdelay(50);
		count++;

		/* read U0 recovery count */
		recov_cnt = __raw_readl(U3D_RECOVERY_COUNT);
		/* read link error count */
		link_error_count = __raw_readl(U3D_LINK_ERR_COUNT);
		/* enter U0 state */
		if ((__raw_readl(U3D_LINK_STATE_MACHINE) & LTSSM) == STATE_U0_STATE) {
			u0_count++;
		}

		 /* link up */
		if (u0_count > ENTER_U0_TH) {
			mdelay(1000);//1s
			recov_cnt = __raw_readl(U3D_RECOVERY_COUNT);
			link_error_count = __raw_readl(U3D_LINK_ERR_COUNT);
			__raw_writel(CLR_RECOV_CNT, U3D_RECOVERY_COUNT); /* clear recovery count */
			__raw_writel(CLR_LINK_ERR_CNT, U3D_LINK_ERR_COUNT); /* clear link error count */
			PHY_LOG("[PASS] Link Error Count=%d, Recovery Count=%d, I2C(0x%x) : [0x%x], I2C(0x%x) : [0x%x], Reg(0x130) : [0x%x], PhaseDelay[0x%x], Driving[0x%x], Latch[0x%x]\n",
				link_error_count, recov_cnt,
				U3_PHY_I2C_PCLK_DRV_REG, _u3_read_reg(U3_PHY_I2C_PCLK_DRV_REG),
				U3_PHY_I2C_PCLK_PHASE_REG, _u3_read_reg(U3_PHY_I2C_PCLK_PHASE_REG),
				__raw_readl(U3D_PIPE_LATCH_SELECT),
				phase_val, driving, latch_val);
			phase_val++;
			fset_phase_val = TRUE;
		} else if ((__raw_readl(U3D_LINK_STATE_MACHINE) & LTSSM) == STATE_DISABLE) { /* link fail */
			PHY_LOG("[FAIL] STATE_DISABLE, PhaseDelay[0x%x]\n", phase_val);
			phase_val++;
			fset_phase_val=TRUE;
		} else if (count > MAX_TIMEOUT_COUNT) { /* link timeout */
			PHY_LOG("[FAIL] TIMEOUT, PhaseDelay[0x%x]\n", phase_val);
			phase_val++;
			fset_phase_val = TRUE;
		}

		if (phase_val > MAX_PHASE_RANGE) {
			/* reset device */
			mu3d_hal_rst_dev();
			mdelay(50);
			/* disable ip power down, disable U2/U3 ip power down. */
			mu3d_hal_ssusb_en();
			mu3d_hal_pdn_dis();
			mdelay(10);

			break;
		}
	}
	#endif

	return 0;
}

int phy_init_d60802(struct u3phy_info *info)
{
	char temp;
	volatile u32 chkvalue = 0;

	/* u2phy part */
	/* manual set U2 slew rate ctrl = 4 */
	u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr5),
		D60802_RG_USB20_HSTX_SRCTRL_OFST, D60802_RG_USB20_HSTX_SRCTRL, 0x4);

	/* fine tune SQTH to gain margin in U2 Rx sensitivity compliance test */
	u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr6),
		D60802_RG_USB20_SQTH_OFST, D60802_RG_USB20_SQTH,0x4);

	/* disable VBUS CMP to save power since no OTG function */
	u3_phy_write_field32(((u32)&info->u2phy_regs_d->usbphyacr6),
		D60802_RG_USB20_OTG_VBUSCMP_EN_OFST, D60802_RG_USB20_OTG_VBUSCMP_EN, 1);

	/* phyd part */
	/* shorten Tx drive stable delay time from 82us -> 25us */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_mix1),
		D60802_RG_SSUSB_TX_DRV_DLY_OFST, D60802_RG_SSUSB_TX_DRV_DLY, 0x13);

	/* The same Rx LFPS detect period	rxlfps_upb as A ver */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_lfps0),
		D60802_RG_SSUSB_RXLFPS_UPB_OFST, D60802_RG_SSUSB_RXLFPS_UPB, 0x19);

	/* No switch to Lock 5g @tx_lfps enable */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_lfps0),
		D60802_RG_SSUSB_LOCK5G_BLOCK_OFST, D60802_RG_SSUSB_LOCK5G_BLOCK, 0x1);

	/* disable DFE to improve Rx JT */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_rx0),
		D60802_RG_SSUSB_RX_DFE_RST_OFST, D60802_RG_SSUSB_RX_DFE_RST, 0);

	/* calibrate CDR offset every time enter TSEQ */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_mix2),
		D60802_RG_SSUSB_CDROS_EN_OFST, D60802_RG_SSUSB_CDROS_EN, 0x1);

	/* Re-Calibration after exit P3 state */
	u3_phy_write_field32(((u32)&info->u3phyd_regs_d->phyd_pll_0),
		D60802_RG_SSUSB_PLLBAND_RECAL_OFST, D60802_RG_SSUSB_PLLBAND_RECAL, 0x1);
	/* phyd bank2 part */
	/* Disable E-Idle Low power mode */
	u3_phy_write_field32(((u32)&info->u3phyd_bank2_regs_d->b2_phyd_top1),
		D60802_RG_SSUSB_FORCE_TX_EIDLE_LP_EN_OFST , D60802_RG_SSUSB_FORCE_TX_EIDLE_LP_EN, 0x1);

	u3_phy_write_field32(((u32)&info->u3phyd_bank2_regs_d->b2_phyd_top1),
		D60802_RG_SSUSB_TX_EIDLE_LP_EN_OFST, D60802_RG_SSUSB_TX_EIDLE_LP_EN, 0);

	/* phya part */
	/* modify Tx det Rx Vth to work around the threshold back to 200mV */
	u3_phy_write_field32(((u32)&info->u3phya_regs_d->reg5),
		D60802_REG5_FLD_RG_SSUSB_RXDET_VTHSEL_L_OFST, D60802_REG5_FLD_RG_SSUSB_RXDET_VTHSEL_L,0x2);

	/* modify Tx det Rx Vth to work around the threshold back to 200mV */
	u3_phy_write_field32(((u32)&info->u3phya_regs_d->reg5),
		D60802_REG5_FLD_RG_SSUSB_RXDET_VTHSEL_H_OFST, D60802_REG5_FLD_RG_SSUSB_RXDET_VTHSEL_H,0x2);

	/* phya da part* /
	/* set to pass SSC min in electrical compliance */
	u3_phy_write_field32(((u32)&info->u3phya_da_regs_d->reg21),
		D60802_REG21_FLD_RG_SSUSB_PLL_SSC_DELTA_U3_OFST, D60802_REG21_FLD_RG_SSUSB_PLL_SSC_DELTA_U3,0x47);

	/* set R step 1 = 2 to improve Rx JT */
	u3_phy_write_field32(((u32)&info->u3phya_da_regs_d->reg32),
		D60802_REG32_FLD_RG_SSUSB_EQ_RSTEP1_U3_OFST, D60802_REG32_FLD_RG_SSUSB_EQ_RSTEP1_U3, 0x2);

	/* phy chip part */
	/* Power down bias at P3, p3 bias _pwd */

	u3_phy_write_field32(((u32)&info->sifslv_chip_regs_d->syspll1),
		(24), (0x1<<24), 0x1);

	/*  PIPE drv = 2 */
	u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla+2), 0x10);

	/*  PIPE phase */
	//u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla+3), 0x44);
	/* for 3.0 */
	//u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla+3), 0x8c);
	//u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla+3), 0x3D);
	u3_phy_write_reg8(((u32)&info->sifslv_chip_regs_d->gpio_ctla+3), 0x75);

	mdelay(1000);

	/* scan proper phase for ssusb */
	//mu3d_hal_phy_scan(info, 1);
	//mu3d_hal_phy_scan(info, 0);
}

void mt_usb_phy_poweron (void)
{
	static struct u3phy_info info;
	volatile u32 u3phy_version;

	info.phyd_version_addr = 0x2000e4;
	u3phy_version = u3_phy_read_reg32(info.phyd_version_addr);
	PHY_LOG("[USBPHY] Phy version is %x\n", u3phy_version);
	info.u2phy_regs_d = (struct u2phy_reg_d *)0x0;
	info.u3phyd_regs_d = (struct u3phyd_reg_d *)0x100000;
	info.u3phyd_bank2_regs_d = (struct u3phyd_bank2_reg_d *)0x200000;
	info.u3phya_regs_d = (struct u3phya_reg_d *)0x300000;
	info.u3phya_da_regs_d = (struct u3phya_da_reg_d *)0x400000;
	info.sifslv_chip_regs_d = (struct sifslv_chip_reg_d *)0x500000;
	info.sifslv_fm_regs_d = (struct sifslv_fm_feg_d *)0xf00000;

	phy_init_d60802(&info);

	/* for RF desense */
	//u2_slew_rate_calibration_d60802(&info);

	mu3d_hal_ssusb_en();
	mu3d_hal_rst_dev();
}
void mt_usb_phy_savecurrent(void)
{
}
void mt_usb_phy_recover(void)
{
}
void mt_usb11_phy_savecurrent(void)
{
}
#else

void enable_ssusb_xtal_clock(bool enable)
{
    if (enable) {
		/*
		* 1 *AP_PLL_CON0 =| 0x1 [0]=1: RG_LTECLKSQ_EN
		* 2 Wait PLL stable (100us)
		* 3 *AP_PLL_CON0 =| 0x2 [1]=1: RG_LTECLKSQ_LPF_EN
		* 4 *AP_PLL_CON2 =| 0x1 [0]=1: DA_REF2USB_TX_EN
		* 5 Wait PLL stable (100us)
		* 6 *AP_PLL_CON2 =| 0x2 [1]=1: DA_REF2USB_TX_LPF_EN
		* 7 *AP_PLL_CON2 =| 0x4 [2]=1: DA_REF2USB_TX_OUT_EN
		*/
		//TODO: open UNIVPLL here?
		USB_SETMASK(AP_PLL_CON0, (0x00000001));
		/*Wait 100 usec*/
		udelay(100);
		USB_SETMASK(AP_PLL_CON0, (0x00000002));
		USB_SETMASK(AP_PLL_CON2, (0x00000001));
		/*Wait 100 usec*/
		udelay(100);
		USB_SETMASK(AP_PLL_CON2, (0x00000002));
		USB_SETMASK(AP_PLL_CON2, (0x00000004));
    } else {
	    /*
		* AP_PLL_CON2 &= 0xFFFFFFF8	[2]=0: DA_REF2USB_TX_OUT_EN
		*					[1]=0: DA_REF2USB_TX_LPF_EN
		*					[0]=0: DA_REF2USB_TX_EN
		*/
		//writel(readl((void __iomem *)AP_PLL_CON2)&~(0x00000007),
		//	   (void __iomem *)AP_PLL_CON2);
    }
}

void switch_2_usb()
{
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_UART_EN);
	USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_UART_EN);
	USB_CLRMASK(U3D_U2PHYACR4, D60802_RG_USB20_GPIO_CTL);
	USB_CLRMASK(U3D_U2PHYACR4, D60802_USB20_GPIO_MODE);
}

void mt_usb_phy_poweron(void)
{
	enable_ssusb_xtal_clock(1);

	/* 5, power domain iso disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_ISO_EN);
	/* 6, switch to USB function */
	switch_2_usb();
	/* 7, DP/DM BC1.1 path Disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);
	/* 8, dp_100k diable */
	USB_CLRMASK(U3D_USBPHYACR4, D60802_USB20_DP_100K_EN);
	/* 9, dm_100k disable */
	USB_CLRMASK(U3D_USBPHYACR4, D60802_RG_USB20_DM_100K_EN);
	/* 10, Change 100uA current switch to SSUSB */
	USB_SETMASK(U3D_USBPHYACR5, D60802_RG_USB20_HS_100U_U3_EN);
	/* 11, OTG enable */
	USB_SETMASK(U3D_USBPHYACR6, D60802_RG_USB20_OTG_VBUSCMP_EN);
	/* 12, Release force suspendm */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_SUSPENDM);

    mu3d_hal_ssusb_en();
}

void mt_usb_phy_savecurrent(void)
{
	/* 1, switch to usb function */
	switch_2_usb();
	/* 2, let syspendm=1 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_SUSPENDM);
	/* 3, force_suspendm */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_SUSPENDM);
	/* 4 wait for USBPLL stable */
	mdelay(2);
	/* 5 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_DPPULLDOWN);
	/* 6 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_DMPULLDOWN);
	/* 7 */
	__raw_writel((__raw_readl(U3D_U2PHYDTM0) & ~D60802_RG_XCVRSEL) | (0x1 << D60802_RG_XCVRSEL_OFST), U3D_U2PHYDTM0);
	/* 8 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_TERMSEL);
	/* 9 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_DATAIN);
	/* 10 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DP_PULLDOWN);
	/* 11 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DM_PULLDOWN);
	/* 12 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_XCVRSEL);
	/* 13 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_TERMSEL);
	/* 14 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DATAIN);
	/* 15, DP/DM BC1.1 path Disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);
	/* 16, OTG disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_OTG_VBUSCMP_EN);
	/* 17, Change 100uA current switch to USB2.0 */
	USB_CLRMASK(U3D_USBPHYACR5, D60802_RG_USB20_HS_100U_U3_EN);
	/* 18, wait 800us */
	mdelay(1);
	/* 19 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_SUSPENDM);
	/* 20, wait 1us */
	mdelay(1);
	USB_CLRMASK(U3D_USB30_PHYA_REG0, RG_SSUSB_VUSB10_ON);
}

void mt_usb_phy_recover(void)
{
    USB_SETMASK(U3D_USB30_PHYA_REG0, RG_SSUSB_VUSB10_ON);

	/* 4, power domain iso disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_ISO_EN);
	/* 5, switch to usb function */
	switch_2_usb();
	/* 6, force_suspendm */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_SUSPENDM);
	/* 7 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_DPPULLDOWN);
	/* 8 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_DMPULLDOWN);
	/* 9 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_XCVRSEL);
	/* 10 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_TERMSEL);
	/* 11 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_DATAIN);
	/* 12 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_DP_PULLDOWN);
	/* 13 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_DM_PULLDOWN);
	/* 14 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_XCVRSEL);
	/* 15 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_TERMSEL);
	/* 16 */
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_DATAIN);
	/* 17, DP/DM BC1.1 path Disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);
	/* 18, OTG enable */
	USB_SETMASK(U3D_USBPHYACR6, D60802_RG_USB20_OTG_VBUSCMP_EN);
	/* 19, Change 100uA current switch to SSUSB */
	USB_SETMASK(U3D_USBPHYACR5, D60802_RG_USB20_HS_100U_U3_EN);

	/*
	 * Reference clock use digital P&R, instead of analog path
	 * possibly more interference
	 * ---------REG Name---------- | -----Modification---| -----Description----
	 * DA_SSUSB_XTAL_EXT_EN[1:0]   | 2'b01-->2'b10       | 0x11290c00 bit[11:10]
	 * DA_SSUSB_XTAL_RX_PWD[9:9]   | -->1'b1             | 0x11280018 bit[9]
	 */
	USB_WRITE_FIELD32(U3D_U3PHYA_DA_REG0, E60802_RG_SSUSB_XTAL_EXT_EN_U3_OFST, E60802_RG_SSUSB_XTAL_EXT_EN_U3, 0x2);
	USB_WRITE_FIELD32(U3D_SPLLC_XTALCTL3, E60802_RG_SSUSB_XTAL_RX_PWD_OFST, E60802_RG_SSUSB_XTAL_RX_PWD, 0x01);

	/*
	 * ---------REG Name---------- | -----Modification---| -----Description----
	 * 1 RG_SSUSB_TX_EIDLE_CM[3:0] | 1100-->1110         | low-power E-idle common mode(650mV to 600mV)
	 *                             |                     | - 0x11290b18 bit [31:28]
	 * 2 RG_SSUSB_CDR_BIR_LTD0[4:0]| 5'b01000-->5'b01100 | Increase BW - 0x1128095c bit [12:8]
	 * 3 RG_XXX_CDR_BIR_LTD1[4:0]  | 5'b00010-->5'b00011 | Increase BW - 0x1128095c bit [28:24]
	 */
	USB_WRITE_FIELD32(U3D_USB30_PHYA_REG6, RG_SSUSB_TX_EIDLE_CM_OFST, RG_SSUSB_TX_EIDLE_CM, 0xE);
	USB_WRITE_FIELD32(U3D_PHYD_CDR1, RG_SSUSB_CDR_BIR_LTD0_OFST, RG_SSUSB_CDR_BIR_LTD0, 0xC);
	USB_WRITE_FIELD32(U3D_PHYD_CDR1, RG_SSUSB_CDR_BIR_LTD1_OFST, RG_SSUSB_CDR_BIR_LTD1, 0x3);

	/* 20, wait 800us */
	mdelay(1);
}

#endif

bool is_uart_cable_inserted(void)
{
	u32 tmpreg32;

    /* ROM Code Setup */ 
    /* Step 5, Power Domain ISO disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_ISO_EN);

    /* Step 6, Switch to USB function */
    USB_CLRMASK(U3D_U2PHYDTM0, D60802_FORCE_UART_EN);
    USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_UART_EN);
    USB_CLRMASK(U3D_U2PHYACR4, D60802_RG_USB20_GPIO_CTL);
    USB_CLRMASK(U3D_U2PHYACR4, D60802_USB20_GPIO_MODE);
    /* Step 7, DP/DM BC 1.1 Path Disable */
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);

    /* Step 8, Internal R bias enable */ 
	USB_SETMASK(U3D_USBPHYACR0, D60802_RG_USB20_INTR_EN);
    /* Step 9, Enable USB 480MHz PLL */ 
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_SUSPENDM);

    /* Step 10, Force SuspendM = 1 */
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_SUSPENDM);
    /* Step 11, Wait USB PLL Stable */
    mdelay(2);

    /* Step 12, OTG Setting, Power On Device Mode */
	USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_VBUSVALID);
	USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_SESSEND);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_BVALID);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_AVALID);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_IDDIG);
    
    /* Step 13, Enable force into Device Mode */
	USB_SETMASK(U3D_U2PHYDTM1, D60802_FORCE_VBUSVALID);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_FORCE_SESSEND);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_FORCE_BVALID);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_FORCE_AVALID);
	USB_SETMASK(U3D_U2PHYDTM1, D60802_FORCE_IDDIG);

    /* Step 14, Wait MAC Ready */
    mdelay(2);

    /* UART/USB Switch Detection Sequence */
    USB_SETMASK(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PHYD_RST);
	USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);

	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_DPPULLDOWN);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_DMPULLDOWN);
	__raw_writel((__raw_readl(U3D_U2PHYDTM0) & ~D60802_RG_XCVRSEL) | (0x1 << D60802_RG_XCVRSEL_OFST), U3D_U2PHYDTM0);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_RG_TERMSEL);
	USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_DATAIN);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DP_PULLDOWN);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DM_PULLDOWN);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_XCVRSEL);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_TERMSEL);
	USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_DATAIN);

	/* linestate */
	tmpreg32 = (__raw_readl (U3D_U2PHYDMON1) & D60802_USB20_LINE_STATE) >> D60802_USB20_LINE_STATE_OFST;
	print(" USB PRB0 LineState: %x", tmpreg32);

    USB_CLRMASK(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PHYD_RST);
    USB_SETMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);

	if (tmpreg32 == 0x2) {
		/* Prolific/FTDI UART cable inserted */
        print("\n[USBD] Prolific/FTDI UART cable inserted!\n");
		return true;
    }
	else {
        USB_CLRMASK(U3D_SSUSB_U2_CTRL_0P, SSUSB_U2_PORT_PHYD_RST);
        USB_SETMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);

        /* Step 1, Power Down Mode OTG Setting */
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_VBUSVALID);
        USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_SESSEND);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_BVALID);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_RG_AVALID);
        USB_SETMASK(U3D_U2PHYDTM1, D60802_RG_IDDIG);
       
        /* Step 2. Do not apply USB IP reset, Wait MAC off ready */
        mdelay(10);

        /* Step 3, Clear OTG Force Mode */
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_FORCE_VBUSVALID);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_FORCE_SESSEND);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_FORCE_BVALID);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_FORCE_AVALID);
        USB_CLRMASK(U3D_U2PHYDTM1, D60802_FORCE_IDDIG);

        /* Step 4, let syspendm=0 */
        USB_CLRMASK(U3D_U2PHYDTM0, D60802_RG_SUSPENDM);

        /* Step 5, force_suspendm */
        USB_SETMASK(U3D_U2PHYDTM0, D60802_FORCE_SUSPENDM);

        /* Step 6 */
        mdelay(2);

		/* USB cable/ No Cable inserted */
        print("\n[USBD] USB cable/ No Cable inserted!\n");
		return false;
    }
}

void Charger_Detect_Init(void)
{
    /* RG_USB20_BC11_SW_EN = 1'b1 */
    USB_SETMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);
}

void Charger_Detect_Release(void)
{
    /* RG_USB20_BC11_SW_EN = 1'b0 */
    USB_CLRMASK(U3D_USBPHYACR6, D60802_RG_USB20_BC11_SW_EN);
}
