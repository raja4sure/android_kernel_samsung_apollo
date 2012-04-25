/* linux/drivers/media/video/samsung/s3c_fimc4x_regs.c
 *
 * Register interface file for Samsung Camera Interface (FIMC) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/videodev2.h>
#include <linux/videodev2_samsung.h>
#include <linux/io.h>
#include <mach/map.h>
#include <plat/regs-fimc.h>
#include <plat/fimc.h>

#include "fimc.h"

/* struct fimc_limit: Limits for FIMC */
struct fimc_limit fimc_limits[FIMC_DEVICES] = {
	{
		.pre_dst_w      = 2048,
		.bypass_w       = 2592,
		.trg_h_no_rot   = 2048,
		.trg_h_rot      = 2048,
		.real_w_no_rot  = 2048,
		.real_h_rot     = 2048,
	}, {
		.pre_dst_w      = 854,
		.bypass_w       = 2592,
		.trg_h_no_rot   = 2592,
		.trg_h_rot      = 720,
		.real_w_no_rot  = 2592,
		.real_h_rot     = 720,
	}, {
		.pre_dst_w      = 320,
		.bypass_w       = 800,
		.trg_h_no_rot   = 320,
		.trg_h_rot      = 320,
		.real_w_no_rot  = 320,
		.real_h_rot     = 320,	
	},
};

/** Set source parameters - YUV format, width and height. */
int fimc_hwset_camera_source(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	struct fimc_capinfo *cap = ctrl->cap;
	u32 cfg = 0;

	/* for now, we support only ITU601 8 bit mode */
	cfg |= S3C_CISRCFMT_ITU601_8BIT;

	if (cap->fmt.pixelformat == V4L2_PIX_FMT_RGB32 ||
                cap->fmt.pixelformat == V4L2_PIX_FMT_RGB565)
                cfg |= CAM_ORDER422_8BIT_CRYCBY;
        else
                cfg |= cam->order422;

	if (cam->type == CAM_TYPE_ITU)
		cfg |= cam->fmt;
	cfg |= S3C_CISRCFMT_SOURCEHSIZE(cam->width);
	cfg |= S3C_CISRCFMT_SOURCEVSIZE(cam->height);

	writel(cfg, ctrl->regs + S3C_CISRCFMT);

	return 0;
}

int fimc_hwset_enable_irq(struct fimc_control *ctrl, int overflow, int level)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_IRQ_OVFEN | S3C_CIGCTRL_IRQ_LEVEL);
	cfg |= S3C_CIGCTRL_IRQ_ENABLE;

	if (overflow)
		cfg |= S3C_CIGCTRL_IRQ_OVFEN;

	if (level)
		cfg |= S3C_CIGCTRL_IRQ_LEVEL;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_enable_irq_frmEnd(struct fimc_control *ctrl)
{
        u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

        cfg |= S3C_CIGCTRL_ENB_IRQ_FRM_END | S3C_CIGCTRL_DIS_IRQ_FRM_START;

        writel(cfg, ctrl->regs + S3C_CIGCTRL);

        return 0;
}



int fimc_hwset_disable_irq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_IRQ_OVFEN | S3C_CIGCTRL_IRQ_ENABLE);
	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_clear_irq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg |= S3C_CIGCTRL_IRQ_CLR;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_reset(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	cfg = readl(ctrl->regs + S3C_CISRCFMT);
	cfg |= S3C_CISRCFMT_ITU601_8BIT;
	writel(cfg, ctrl->regs + S3C_CISRCFMT);

	/* s/w reset */
	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg |= (S3C_CIGCTRL_SWRST);
	writel(cfg, ctrl->regs + S3C_CIGCTRL);
	mdelay(1);

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_SWRST;
	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	/* in case of ITU656, CISRCFMT[31] should be 0 */
	if ((ctrl->cap != NULL) && (ctrl->cam->fmt == ITU_656_YCBCR422_8BIT)) {
		cfg = readl(ctrl->regs + S3C_CISRCFMT);
		cfg &= ~S3C_CISRCFMT_ITU601_8BIT;
		writel(cfg, ctrl->regs + S3C_CISRCFMT);
	}

	return 0;
}

int fimc_hwget_overflow_state(struct fimc_control *ctrl)
{
	u32 cfg, status, flag;

	status = readl(ctrl->regs + S3C_CISTATUS);
	flag = S3C_CISTATUS_OVFIY | S3C_CISTATUS_OVFICB | S3C_CISTATUS_OVFICR;

	if (status & flag) {
		cfg = readl(ctrl->regs + S3C_CIWDOFST);
		cfg |= (S3C_CIWDOFST_CLROVFIY | S3C_CIWDOFST_CLROVFICB |
			S3C_CIWDOFST_CLROVFICR);
		writel(cfg, ctrl->regs + S3C_CIWDOFST);

		cfg = readl(ctrl->regs + S3C_CIWDOFST);
		cfg &= ~(S3C_CIWDOFST_CLROVFIY | S3C_CIWDOFST_CLROVFICB |
			S3C_CIWDOFST_CLROVFICR);
		writel(cfg, ctrl->regs + S3C_CIWDOFST);

		return 1;
	}

	return 0;
}

int fimc_hwset_camera_offset(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	struct v4l2_rect *rect = NULL;
	u32 cfg, h1, h2, v1, v2;

	if (!cam) {
		dev_err(ctrl->dev, "%s: no active camera\n", \
			__func__);
		return -ENODEV;
	}

	/* First check and then assign to avoid possible null-pointer dereferencing */
	rect = &cam->window;

	h1 = rect->left;
	h2 = cam->width - rect->width - rect->left;
	v1 = rect->top;
	v2 = cam->height - rect->height - rect->top;

	cfg = readl(ctrl->regs + S3C_CIWDOFST);
	cfg &= ~(S3C_CIWDOFST_WINHOROFST_MASK | S3C_CIWDOFST_WINVEROFST_MASK);
	cfg |= S3C_CIWDOFST_WINHOROFST(h1);
	cfg |= S3C_CIWDOFST_WINVEROFST(v1);
	cfg |= S3C_CIWDOFST_WINOFSEN;
	writel(cfg, ctrl->regs + S3C_CIWDOFST);

	cfg = 0;
	cfg |= S3C_CIWDOFST2_WINHOROFST2(h2);
	cfg |= S3C_CIWDOFST2_WINVEROFST2(v2);
	writel(cfg, ctrl->regs + S3C_CIWDOFST2);

	return 0;
}

int fimc_hwset_camera_polarity(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg;

	if (!cam) {
		dev_err(ctrl->dev, "%s: no active camera\n",
			__func__);
		return -ENODEV;
	}

	cfg = readl(ctrl->regs + S3C_CIGCTRL);

	cfg &= ~(S3C_CIGCTRL_INVPOLPCLK | S3C_CIGCTRL_INVPOLVSYNC |
		 S3C_CIGCTRL_INVPOLHREF | S3C_CIGCTRL_INVPOLHSYNC);

	if (cam->inv_pclk)
		cfg |= S3C_CIGCTRL_INVPOLPCLK;

	if (cam->inv_vsync)
		cfg |= S3C_CIGCTRL_INVPOLVSYNC;

	if (cam->inv_href)
		cfg |= S3C_CIGCTRL_INVPOLHREF;

	if (cam->inv_hsync)
		cfg |= S3C_CIGCTRL_INVPOLHSYNC;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_camera_type(struct fimc_control *ctrl)
{
	struct s3c_platform_camera *cam = ctrl->cam;
	u32 cfg;

	if (!cam) {
		dev_err(ctrl->dev, "%s: no active camera\n",
			__func__);
		return -ENODEV;
	}

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~(S3C_CIGCTRL_TESTPATTERN_MASK | S3C_CIGCTRL_SELCAM_ITU_MASK |
		S3C_CIGCTRL_SELCAM_MIPI_MASK | S3C_CIGCTRL_SELCAM_FIMC_MASK);

	/* Interface selection */
	if (cam->type == CAM_TYPE_MIPI) {
		cfg |= S3C_CIGCTRL_SELCAM_FIMC_MIPI;

		/* FIXME: temporary only A support */
		cfg |= S3C_CIGCTRL_SELCAM_MIPI_A;

		/* FIXME: temporary hardcoded value used */
		writel(cam->fmt | (0x1 << 8), ctrl->regs + S3C_CSIIMGFMT);
	} else if (cam->type == CAM_TYPE_ITU) {
		if (cam->id == CAMERA_PAR_A)
			cfg |= S3C_CIGCTRL_SELCAM_ITU_A;
		else
			cfg |= S3C_CIGCTRL_SELCAM_ITU_B;
		/* switch to ITU interface */
		cfg |= S3C_CIGCTRL_SELCAM_FIMC_ITU;
	} else {
		dev_err(ctrl->dev, "%s: invalid camera bus type selected\n",
			__func__);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_output_size(struct fimc_control *ctrl, int width, int height)
{
	u32 cfg = readl(ctrl->regs + S3C_CITRGFMT);

	cfg &= ~(S3C_CITRGFMT_TARGETH_MASK | S3C_CITRGFMT_TARGETV_MASK);

	cfg |= S3C_CITRGFMT_TARGETHSIZE(width);
	cfg |= S3C_CITRGFMT_TARGETVSIZE(height);

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_output_colorspace(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg;

	if (pixelformat == V4L2_PIX_FMT_YUV444) {
		cfg = readl(ctrl->regs + S3C_CIEXTEN);
		cfg |= S3C_CIEXTEN_YUV444_OUT;
		writel(cfg, ctrl->regs + S3C_CIEXTEN);
		
		return 0;
	} else {
		cfg = readl(ctrl->regs + S3C_CIEXTEN);
		cfg &= ~S3C_CIEXTEN_YUV444_OUT;
		writel(cfg, ctrl->regs + S3C_CIEXTEN);
	}

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_OUTFORMAT_MASK;

	switch (pixelformat) {
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB32:
		cfg |= S3C_CITRGFMT_OUTFORMAT_RGB;
		break;

	case V4L2_PIX_FMT_YUYV:		/* fall through */
	case V4L2_PIX_FMT_UYVY:		/* fall through */
	case V4L2_PIX_FMT_VYUY:		/* fall through */
	case V4L2_PIX_FMT_YVYU:		/* fall through */
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR422_1PLANE;
		break;

	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_YUV422P:
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR422;
		break;

	case V4L2_PIX_FMT_YUV420:	/* fall through */
	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:	/* fall through */
	case V4L2_PIX_FMT_NV21:		/* fall through */
		cfg |= S3C_CITRGFMT_OUTFORMAT_YCBCR420;
		break;

	default:
		dev_err(ctrl->dev, "%s: invalid pixel format\n", __func__);
		break;
	}

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

/* FIXME */
int fimc_hwset_output_rot_flip(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_FLIP_MASK;
	cfg &= ~S3C_CITRGFMT_OUTROT90_CLOCKWISE;

	val = fimc_mapping_rot_flip(rot, flip);

	if (val & FIMC_ROT)
		cfg |= S3C_CITRGFMT_OUTROT90_CLOCKWISE;

	if (val & FIMC_XFLIP)
		cfg |= S3C_CITRGFMT_FLIP_X_MIRROR;

	if (val & FIMC_YFLIP)
		cfg |= S3C_CITRGFMT_FLIP_Y_MIRROR;

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_output_area(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg = S3C_CITAREA_TARGET_AREA(width * height);
	writel(cfg, ctrl->regs + S3C_CITAREA);

	return 0;
}

int fimc_hwset_enable_lastirq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIOCTRL);

	cfg |= S3C_CIOCTRL_LASTIRQ_ENABLE;
	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_disable_lastirq(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIOCTRL);

	cfg &= ~S3C_CIOCTRL_LASTIRQ_ENABLE;
	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_prescaler(struct fimc_control *ctrl)
{
	struct fimc_scaler *sc = &ctrl->sc;
	u32 cfg = 0, shfactor;

	shfactor = 10 - (sc->hfactor + sc->vfactor);

	cfg |= S3C_CISCPRERATIO_SHFACTOR(shfactor);
	cfg |= S3C_CISCPRERATIO_PREHORRATIO(sc->pre_hratio);
	cfg |= S3C_CISCPRERATIO_PREVERRATIO(sc->pre_vratio);

	writel(cfg, ctrl->regs + S3C_CISCPRERATIO);

	cfg = 0;
	cfg |= S3C_CISCPREDST_PREDSTWIDTH(sc->pre_dst_width);
	cfg |= S3C_CISCPREDST_PREDSTHEIGHT(sc->pre_dst_height);

	writel(cfg, ctrl->regs + S3C_CISCPREDST);

	return 0;
}

int fimc_hwset_output_address(struct fimc_control *ctrl,
				struct fimc_buf_set *bs, int id)
{
	writel(bs->base[FIMC_ADDR_Y], ctrl->regs + S3C_CIOYSA(id));
	writel(bs->base[FIMC_ADDR_CB], ctrl->regs + S3C_CIOCBSA(id));
	writel(bs->base[FIMC_ADDR_CR], ctrl->regs + S3C_CIOCRSA(id));

	return 0;
}

int fimc_hwset_output_yuv(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_CIOCTRL);
	cfg &= ~(S3C_CIOCTRL_ORDER2P_MASK | S3C_CIOCTRL_ORDER422_MASK |
		S3C_CIOCTRL_YCBCR_PLANE_MASK);

	switch (pixelformat) {
	/* 1 plane formats */
	case V4L2_PIX_FMT_YUYV:
		cfg |= S3C_CIOCTRL_ORDER422_YCBYCR;
		break;

	case V4L2_PIX_FMT_UYVY:
		cfg |= S3C_CIOCTRL_ORDER422_CBYCRY;
		break;

	case V4L2_PIX_FMT_VYUY:
		cfg |= S3C_CIOCTRL_ORDER422_CRYCBY;
		break;

	case V4L2_PIX_FMT_YVYU:
		cfg |= S3C_CIOCTRL_ORDER422_YCRYCB;
		break;

	/* 2 plane formats */
	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:	/* fall through */
	case V4L2_PIX_FMT_NV16:
		cfg |= S3C_CIOCTRL_ORDER2P_LSB_CBCR;
		cfg |= S3C_CIOCTRL_YCBCR_2PLANE;
		break;

	case V4L2_PIX_FMT_NV21:		/* fall through */
	case V4L2_PIX_FMT_NV61:
		cfg |= S3C_CIOCTRL_ORDER2P_LSB_CRCB;    //valid only for CIOCTRL1
		cfg |= S3C_CIOCTRL_YCBCR_2PLANE;
		break;

	/* 3 plane formats */
	case V4L2_PIX_FMT_YUV422P:	/* fall through */
	case V4L2_PIX_FMT_YUV420:
		cfg |= S3C_CIOCTRL_YCBCR_3PLANE;
		break;
	}

	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_output_scan(struct fimc_control *ctrl, struct v4l2_pix_format *fmt)
{
	u32 cfg;

	cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_SCAN_MASK;

	if (fmt->field == V4L2_FIELD_INTERLACED ||
		fmt->field == V4L2_FIELD_INTERLACED_TB)
		cfg |= S3C_CISCCTRL_INTERLACE;
	else
		cfg |= S3C_CISCCTRL_PROGRESSIVE;

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	cfg = readl(ctrl->regs + S3C_CIOCTRL);
	cfg &= ~S3C_CIOCTRL_WEAVE_MASK;

	if ((ctrl->cap) && (fmt->field == V4L2_FIELD_INTERLACED_TB))
		cfg |= S3C_CIOCTRL_WEAVE_OUT;

	writel(cfg, ctrl->regs + S3C_CIOCTRL);

	return 0;
}

int fimc_hwset_input_rot(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_CITRGFMT);
	cfg &= ~S3C_CITRGFMT_INROT90_CLOCKWISE;

	val = fimc_mapping_rot_flip(rot, flip);

	if (val & FIMC_ROT)
		cfg |= S3C_CITRGFMT_INROT90_CLOCKWISE;

	writel(cfg, ctrl->regs + S3C_CITRGFMT);

	return 0;
}

int fimc_hwset_scaler(struct fimc_control *ctrl)
{
	struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev);
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~(S3C_CISCCTRL_SCALERBYPASS |
		S3C_CISCCTRL_SCALEUP_H | S3C_CISCCTRL_SCALEUP_V |
		S3C_CISCCTRL_MAIN_V_RATIO_MASK |
		S3C_CISCCTRL_MAIN_H_RATIO_MASK);
	cfg |= (S3C_CISCCTRL_CSCR2Y_WIDE | S3C_CISCCTRL_CSCY2R_WIDE);

	if (ctrl->sc.bypass)
		cfg |= S3C_CISCCTRL_SCALERBYPASS;

	if (ctrl->sc.scaleup_h)
		cfg |= S3C_CISCCTRL_SCALEUP_H;

	if (ctrl->sc.scaleup_v)
		cfg |= S3C_CISCCTRL_SCALEUP_V;

    if (pdata->hw_ver == 0x50 ){
		u32 cfg1 = readl(ctrl->regs + S3C_CIEXTEN);
        cfg |= S3C_CISCCTRL_MAINHORRATIO(((ctrl->sc.main_hratio&0x7fc0)>>6));
		cfg |= S3C_CISCCTRL_MAINVERRATIO(((ctrl->sc.main_vratio&0x7fc0)>>6));

        cfg1 &= ~S3C_CIEXTEN_MAINHRATIO_EXT_MASK;
		cfg1 &= ~S3C_CIEXTEN_MAINVRATIO_EXT_MASK;
		cfg1 |= (((ctrl->sc.main_hratio&0x3f)<<10) | (ctrl->sc.main_vratio&0x3f));
		writel(cfg1, ctrl->regs + S3C_CIEXTEN);
	} else {
		cfg |= S3C_CISCCTRL_MAINHORRATIO(ctrl->sc.main_hratio);
		cfg |= S3C_CISCCTRL_MAINVERRATIO(ctrl->sc.main_vratio);
	}
	

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_enable_lcdfifo(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg |= S3C_CISCCTRL_LCDPATHEN_FIFO;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_disable_lcdfifo(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~S3C_CISCCTRL_LCDPATHEN_FIFO;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwget_frame_count(struct fimc_control *ctrl)
{
	return S3C_CISTATUS_GET_FRAME_COUNT(readl(ctrl->regs + S3C_CISTATUS));
}

int fimc_hwget_frame_end(struct fimc_control *ctrl)
{
	unsigned long timeo = jiffies;
	u32 cfg;

	timeo += 20;    /* waiting for 100ms */
	while (time_before(jiffies, timeo)) {
		cfg = readl(ctrl->regs + S3C_CISTATUS);

		if (S3C_CISTATUS_GET_FRAME_END(cfg)) {
			cfg &= ~S3C_CISTATUS_FRAMEEND;
			writel(cfg, ctrl->regs + S3C_CISTATUS);
			break;
		}
		//cond_resched();
		msleep_interruptible(2);
	}

	return 0;
}

int fimc_hwget_last_frame_end(struct fimc_control *ctrl)
{
	unsigned long timeo = jiffies;
	u32 cfg;

	timeo += 20;    /* waiting for 100ms */
	while (time_before(jiffies, timeo)) {
		cfg = readl(ctrl->regs + S3C_CISTATUS);

		if (S3C_CISTATUS_GET_LAST_CAPTURE_END(cfg)) {
			cfg &= ~S3C_CISTATUS_LASTCAPTUREEND;
			writel(cfg, ctrl->regs + S3C_CISTATUS);
			break;
		}
		//cond_resched();
		msleep_interruptible(2);
	}

	return 0;
}

int fimc_hwset_start_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg |= S3C_CISCCTRL_SCALERSTART;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_stop_scaler(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);

	cfg &= ~S3C_CISCCTRL_SCALERSTART;
	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_input_rgb(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_INRGB_FMT_RGB_MASK;

	if (pixelformat == V4L2_PIX_FMT_RGB32)
		cfg |= S3C_CISCCTRL_INRGB_FMT_RGB888;
	else if (pixelformat == V4L2_PIX_FMT_RGB565)
		cfg |= S3C_CISCCTRL_INRGB_FMT_RGB565;

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_intput_field(struct fimc_control *ctrl, enum v4l2_field field)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_FIELD_MASK;

	if (field == V4L2_FIELD_NONE)
		cfg |= S3C_MSCTRL_FIELD_NORMAL;
	else if (field == V4L2_FIELD_INTERLACED_TB)
		cfg |= S3C_MSCTRL_FIELD_WEAVE;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_output_rgb(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_OUTRGB_FMT_RGB_MASK;

	if (pixelformat == V4L2_PIX_FMT_RGB32)
		cfg |= S3C_CISCCTRL_OUTRGB_FMT_RGB888;
	else if (pixelformat == V4L2_PIX_FMT_RGB565)
		cfg |= S3C_CISCCTRL_OUTRGB_FMT_RGB565;

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_ext_rgb(struct fimc_control *ctrl, int enable)
{
	u32 cfg = readl(ctrl->regs + S3C_CISCCTRL);
	cfg &= ~S3C_CISCCTRL_EXTRGB_EXTENSION;

	if (enable)
		cfg |= S3C_CISCCTRL_EXTRGB_EXTENSION;

	writel(cfg, ctrl->regs + S3C_CISCCTRL);

	return 0;
}

int fimc_hwset_enable_capture(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIIMGCPT);
	cfg &= ~S3C_CIIMGCPT_IMGCPTEN_SC;
	cfg |= S3C_CIIMGCPT_IMGCPTEN;

	if (!ctrl->sc.bypass)
		cfg |= S3C_CIIMGCPT_IMGCPTEN_SC;

	writel(cfg, ctrl->regs + S3C_CIIMGCPT);

	return 0;
}

int fimc_hwset_disable_capture(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIIMGCPT);

	cfg &= ~(S3C_CIIMGCPT_IMGCPTEN_SC | S3C_CIIMGCPT_IMGCPTEN);

	writel(cfg, ctrl->regs + S3C_CIIMGCPT);

	return 0;
}

int fimc_hwset_input_address(struct fimc_control *ctrl, dma_addr_t *base)
{
	writel(base[FIMC_ADDR_Y], ctrl->regs + S3C_CIIYSA0);
	writel(base[FIMC_ADDR_CB], ctrl->regs + S3C_CIICBSA0);
	writel(base[FIMC_ADDR_CR], ctrl->regs + S3C_CIICRSA0);

	return 0;
}

int fimc_hwset_enable_autoload(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg |= S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_disable_autoload(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg &= ~S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_real_input_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);
	cfg &= ~(S3C_CIREAL_ISIZE_HEIGHT_MASK | S3C_CIREAL_ISIZE_WIDTH_MASK);

	cfg |= S3C_CIREAL_ISIZE_WIDTH(width);
	cfg |= S3C_CIREAL_ISIZE_HEIGHT(height);

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_addr_change_enable(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg &= ~S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_addr_change_disable(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_CIREAL_ISIZE);

	cfg |= S3C_CIREAL_ISIZE_ADDR_CH_DISABLE;

	writel(cfg, ctrl->regs + S3C_CIREAL_ISIZE);

	return 0;
}

int fimc_hwset_input_burst_cnt(struct fimc_control *ctrl, u32 cnt)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_BURST_CNT_MASK;

	if (cnt > 4)
		cnt = 4;
	else if (cnt == 0)
		cnt = 4;

	cfg |= S3C_MSCTRL_SUCCESSIVE_COUNT(cnt);
	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_colorspace(struct fimc_control *ctrl, u32 pixelformat)
{
    struct s3c_platform_fimc *pdata = to_fimc_plat(ctrl->dev); 
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_INFORMAT_RGB;

	/* Color format setting */
	switch (pixelformat) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:
		cfg |= S3C_MSCTRL_INFORMAT_YCBCR420;
		break;
	case V4L2_PIX_FMT_NV21:
        if (pdata->hw_ver == 0x50)
		  cfg |= S3C_MSCTRL_INFORMAT_YCBCR420;
//		cfg |= S3C_MSCTRL_2PLANE_LSB_CRCB; 
		break;
	case V4L2_PIX_FMT_YUYV:
		cfg |= S3C_MSCTRL_INFORMAT_YCBCR422_1PLANE;
		break;		
	case V4L2_PIX_FMT_NV16:		/* fall through */
	case V4L2_PIX_FMT_NV61:		/* fall through */
		cfg |= S3C_MSCTRL_INFORMAT_YCBCR422;
		break;		
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB32:
		cfg |= S3C_MSCTRL_INFORMAT_RGB;		
		break;
	default: 
		dev_err(ctrl->dev, "%s: Invalid pixelformt : %d\n", 
				__FUNCTION__, pixelformat);
		return -EINVAL;
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_yuv(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~(S3C_MSCTRL_2PLANE_SHIFT_MASK | S3C_MSCTRL_C_INT_IN_2PLANE | \
						S3C_MSCTRL_ORDER422_YCBYCR);

	switch (pixelformat) {
	case V4L2_PIX_FMT_YUYV:		/* fall through */
		cfg |= S3C_MSCTRL_ORDER422_YCBYCR;
		break;
    case V4L2_PIX_FMT_UYVY:	
		cfg |= S3C_MSCTRL_ORDER422_CBYCRY;
		break;
	case V4L2_PIX_FMT_YVYU:
		cfg |= S3C_MSCTRL_ORDER422_YCRYCB;
		break;
	case V4L2_PIX_FMT_VYUY:		
		cfg |= S3C_MSCTRL_ORDER422_CRYCBY;
		break; 
	case V4L2_PIX_FMT_NV16:		/* fall through */
		cfg |= S3C_MSCTRL_2PLANE_LSB_CBCR;
		cfg |= S3C_MSCTRL_C_INT_IN_2PLANE;
		break;
	case V4L2_PIX_FMT_NV61:		/* fall through */
	case V4L2_PIX_FMT_NV21:
		cfg |= S3C_MSCTRL_2PLANE_LSB_CRCB;
		cfg |= S3C_MSCTRL_C_INT_IN_2PLANE;
		break;
	case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:
		cfg |= S3C_MSCTRL_2PLANE_LSB_CBCR;
		cfg |= S3C_MSCTRL_C_INT_IN_2PLANE;
		break;
	case V4L2_PIX_FMT_YUV420:
		cfg |= S3C_MSCTRL_INFORMAT_YCBCR420;
		cfg |= S3C_MSCTRL_C_INT_IN_3PLANE;
		break;
	case V4L2_PIX_FMT_RGB565:	/* fall through */
	case V4L2_PIX_FMT_RGB32:
		break;
	default: 
		dev_err(ctrl->dev, "%s: Invalid pixelformt : %d\n", 
				__FUNCTION__, pixelformat);
	}

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_flip(struct fimc_control *ctrl, u32 rot, u32 flip)
{
	u32 cfg, val;

	cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~(S3C_MSCTRL_FLIP_X_MIRROR | S3C_MSCTRL_FLIP_Y_MIRROR);
	val = fimc_mapping_rot_flip(rot, flip);

	if (val & FIMC_XFLIP)
		cfg |= S3C_MSCTRL_FLIP_X_MIRROR;

	if (val & FIMC_YFLIP)
		cfg |= S3C_MSCTRL_FLIP_Y_MIRROR;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_input_source(struct fimc_control *ctrl, enum fimc_input path)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_INPUT_MASK;

	if (path == FIMC_SRC_MSDMA)
		cfg |= S3C_MSCTRL_INPUT_MEMORY;
	else if (path == FIMC_SRC_CAM)
		cfg |= S3C_MSCTRL_INPUT_EXTCAM;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;

}

int fimc_hwset_start_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg |= S3C_MSCTRL_ENVID;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}

int fimc_hwset_stop_input_dma(struct fimc_control *ctrl)
{
	u32 cfg = readl(ctrl->regs + S3C_MSCTRL);
	cfg &= ~S3C_MSCTRL_ENVID;

	writel(cfg, ctrl->regs + S3C_MSCTRL);

	return 0;
}


static void fimc_wait_stop_processing(struct fimc_control *ctrl)
{
	fimc_hwget_frame_end(ctrl);
	fimc_hwget_last_frame_end(ctrl);
}


void fimc_hwset_stop_processing(struct fimc_control *ctrl)
{               
	fimc_wait_stop_processing(ctrl);

        fimc_hwset_stop_scaler(ctrl);
        fimc_hwset_disable_capture(ctrl);
        fimc_hwset_stop_input_dma(ctrl);

	/* We need to wait for sometime after processing is stopped.
 	 * This is required for obtaining clean buffer for DMA processing. */
	fimc_wait_stop_processing(ctrl);
}  


/* FIXME */
int fimc_hwset_output_offset(struct fimc_control *ctrl, u32 pixelformat,
				struct v4l2_rect *bounds,
				struct v4l2_rect *crop)
{
	u32 cfg_y = 0, cfg_cb = 0, cfg_cr = 0;

   //  If fimc hw_ver is 4.5 or 5.0, The Offset values are based on pixel unit


	dev_dbg(ctrl->dev, "%s: left: %d, top: %d, width: %d, height: %d\n", \
		__func__, crop->left, crop->top, crop->width, crop->height);

	switch (pixelformat) {
	/* 1 plane 16/32 bits per pixel*/
	case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_YUYV:	/* fall through */
	case V4L2_PIX_FMT_UYVY:	/* fall through */
	case V4L2_PIX_FMT_VYUY:	/* fall through */
	case V4L2_PIX_FMT_YVYU:	/* fall through */
	case V4L2_PIX_FMT_RGB565:
		cfg_y |= S3C_CIOYOFF_HORIZONTAL(crop->left);
		cfg_y |= S3C_CIOYOFF_VERTICAL(crop->top);
		break;	

	/* 2 planes, 16/12 bits per pixel */
	case V4L2_PIX_FMT_NV16:	/* fall through */
	case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_NV12:		/* fall through */
	case V4L2_PIX_FMT_NV12T:	/* fall through */		
	case V4L2_PIX_FMT_NV21:
		cfg_y |= S3C_CIOYOFF_HORIZONTAL(crop->left);
		cfg_y |= S3C_CIOYOFF_VERTICAL(crop->top);
		cfg_cb |= S3C_CIOCBOFF_HORIZONTAL(crop->left);
		cfg_cb |= S3C_CIOCBOFF_VERTICAL(crop->top);
		break;
	

	/* 3 planes, 12/16 bits per pixel */
	case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUV420: 
		cfg_y |= S3C_CIOYOFF_HORIZONTAL(crop->left);
		cfg_y |= S3C_CIOYOFF_VERTICAL(crop->top);
		cfg_cb |= S3C_CIOCBOFF_HORIZONTAL(crop->left);
	    cfg_cb |= S3C_CIOCBOFF_VERTICAL(crop->top);
	    cfg_cr |= S3C_CIOCROFF_HORIZONTAL(crop->left);
	    cfg_cr |= S3C_CIOCROFF_VERTICAL(crop->top);
		break;	

	default:
		break;
	}

	writel(cfg_y, ctrl->regs + S3C_CIOYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIOCBOFF);
	writel(cfg_cr, ctrl->regs + S3C_CIOCROFF);

	return 0;
}

int fimc_hwset_input_offset(struct fimc_control *ctrl, u32 pixelformat,
				struct v4l2_rect *bounds,
				struct v4l2_rect *crop)
{
	u32 cfg_y = 0, cfg_cb = 0, cfg_cr = 0;
    
    //  If fimc hw_ver is 4.5 or 5.0, The Offset values are based on pixel unit
	

	if (crop->left || crop->top || \
		(bounds->width != crop->width) || (bounds->height != crop->height)) {
		switch (pixelformat) {
		case V4L2_PIX_FMT_YUYV:		/* fall through */
		case V4L2_PIX_FMT_RGB565:	/* fall through */
        case V4L2_PIX_FMT_RGB32: 
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
			break;			
		
	    case V4L2_PIX_FMT_NV12:		/* fall through */
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV12T:		
		case V4L2_PIX_FMT_NV16:		/* fall through */
		case V4L2_PIX_FMT_NV61:		/* fall through */         
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
			cfg_cb |= S3C_CIICBOFF_HORIZONTAL(crop->left);
			cfg_cb |= S3C_CIICBOFF_VERTICAL(crop->top);
			break;
		
			
		case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YUV422P:			
			cfg_y |= S3C_CIIYOFF_HORIZONTAL(crop->left);
			cfg_y |= S3C_CIIYOFF_VERTICAL(crop->top);
			cfg_cb |= S3C_CIICBOFF_HORIZONTAL(crop->left);
			cfg_cb |= S3C_CIICBOFF_VERTICAL(crop->top);
			cfg_cr |= S3C_CIICROFF_HORIZONTAL(crop->left);
			cfg_cr |= S3C_CIICROFF_VERTICAL(crop->top);
			break;
		default: 
			dev_err(ctrl->dev, "%s: Invalid pixelformt : %d\n", 
					__FUNCTION__, pixelformat);
		}
	}

	writel(cfg_y, ctrl->regs + S3C_CIIYOFF);
	writel(cfg_cb, ctrl->regs + S3C_CIICBOFF);
	writel(cfg_cr, ctrl->regs + S3C_CIICROFF);

	return 0;
}

int fimc_hwset_org_input_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg |= S3C_ORGISIZE_HORIZONTAL(width);
	cfg |= S3C_ORGISIZE_VERTICAL(height);

	writel(cfg, ctrl->regs + S3C_ORGISIZE);

	return 0;
}

int fimc_hwset_org_output_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = 0;

	cfg |= S3C_ORGOSIZE_HORIZONTAL(width);
	cfg |= S3C_ORGOSIZE_VERTICAL(height);

	writel(cfg, ctrl->regs + S3C_ORGOSIZE);

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_CSC_MASK;

	if (width >= FIMC_HD_WIDTH)
		cfg |= S3C_CIGCTRL_CSC_ITU709;
	else 
		cfg |= S3C_CIGCTRL_CSC_ITU601;

	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	return 0;
}

int fimc_hwset_ext_output_size(struct fimc_control *ctrl, u32 width, u32 height)
{
	u32 cfg = readl(ctrl->regs + S3C_CIEXTEN);

	cfg &= ~S3C_CIEXTEN_TARGETH_EXT_MASK;
	cfg &= ~S3C_CIEXTEN_TARGETV_EXT_MASK;
	cfg |= S3C_CIEXTEN_TARGETH_EXT(width);
	cfg |= S3C_CIEXTEN_TARGETV_EXT(height);

	writel(cfg, ctrl->regs + S3C_CIEXTEN);

	return 0;
}

int fimc_hwset_input_addr_style(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CIDMAPARAM);
	cfg &= ~S3C_CIDMAPARAM_R_MODE_MASK;
	
	if (pixelformat == V4L2_PIX_FMT_NV12T)
		cfg |= S3C_CIDMAPARAM_R_MODE_64X32;
	else
		cfg |= S3C_CIDMAPARAM_R_MODE_LINEAR;

	writel(cfg, ctrl->regs + S3C_CIDMAPARAM);

	return 0;	
}

int fimc_hwset_output_addr_style(struct fimc_control *ctrl, u32 pixelformat)
{
	u32 cfg = readl(ctrl->regs + S3C_CIDMAPARAM);
	cfg &= ~S3C_CIDMAPARAM_W_MODE_MASK;
	
	if (pixelformat == V4L2_PIX_FMT_NV12T)
		cfg |= S3C_CIDMAPARAM_W_MODE_64X32;
	else
		cfg |= S3C_CIDMAPARAM_W_MODE_LINEAR;

	writel(cfg, ctrl->regs + S3C_CIDMAPARAM);

	return 0;	
}

static void fimc_reset_cfg(struct fimc_control *ctrl)
{
	int i;
	u32 cfg[][2] = {
		{ 0x018, 0x00000000 }, { 0x01c, 0x00000000 },
		{ 0x020, 0x00000000 }, { 0x024, 0x00000000 },
		{ 0x028, 0x00000000 }, { 0x02c, 0x00000000 },
		{ 0x030, 0x00000000 }, { 0x034, 0x00000000 },
		{ 0x038, 0x00000000 }, { 0x03c, 0x00000000 },
		{ 0x040, 0x00000000 }, { 0x044, 0x00000000 },
		{ 0x048, 0x00000000 }, { 0x04c, 0x00000000 },
		{ 0x050, 0x00000000 }, { 0x054, 0x00000000 },
		{ 0x058, 0x18000000 }, { 0x05c, 0x00000000 },
		{ 0x0c0, 0x00000000 }, { 0x0c4, 0xffffffff },
		{ 0x0d0, 0x00100080 }, { 0x0d4, 0x00000000 },
		{ 0x0d8, 0x00000000 }, { 0x0dc, 0x00000000 },
		{ 0x0f8, 0x00000000 }, { 0x0fc, 0x04000000 },
		{ 0x168, 0x00000000 }, { 0x16c, 0x00000000 },
		{ 0x170, 0x00000000 }, { 0x174, 0x00000000 },
		{ 0x178, 0x00000000 }, { 0x17c, 0x00000000 },
		{ 0x180, 0x00000000 }, { 0x184, 0x00000000 },
		{ 0x188, 0x00000000 }, { 0x18c, 0x00000000 },
		{ 0x194, 0x0000001e },
	};

	for (i = 0; i < sizeof(cfg) / 8; i++)
		writel(cfg[i][1], ctrl->regs + cfg[i][0]);
}

void fimc_reset(struct fimc_control *ctrl)
{
	u32 cfg = 0;

	dev_dbg(ctrl->dev, "%s: called\n", __func__);

	cfg = readl(ctrl->regs + S3C_CISRCFMT);
	cfg |= S3C_CISRCFMT_ITU601_8BIT;
	writel(cfg, ctrl->regs + S3C_CISRCFMT);

	/* s/w reset */
	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg |= (S3C_CIGCTRL_SWRST);
	writel(cfg, ctrl->regs + S3C_CIGCTRL);
	mdelay(1);

	cfg = readl(ctrl->regs + S3C_CIGCTRL);
	cfg &= ~S3C_CIGCTRL_SWRST;
	writel(cfg, ctrl->regs + S3C_CIGCTRL);

	/* in case of ITU656, CISRCFMT[31] should be 0 */
	if ((ctrl->cap != NULL) && (ctrl->cam->fmt == ITU_656_YCBCR422_8BIT)) {
		cfg = readl(ctrl->regs + S3C_CISRCFMT);
		cfg &= ~S3C_CISRCFMT_ITU601_8BIT;
		writel(cfg, ctrl->regs + S3C_CISRCFMT);
	}

	fimc_reset_cfg(ctrl);
}

static unsigned int save_reg[3][42];

void fimc_save_regs(struct fimc_control *ctrl)
{
        save_reg[ctrl->id][0] = readl(ctrl->regs + S3C_CISRCFMT);
        save_reg[ctrl->id][1] = readl(ctrl->regs + S3C_CIWDOFST);
        save_reg[ctrl->id][2] = readl(ctrl->regs + S3C_CIGCTRL);
        save_reg[ctrl->id][3] = readl(ctrl->regs + S3C_CIWDOFST2);
        save_reg[ctrl->id][4] = readl(ctrl->regs + S3C_CIOYSA1);
        save_reg[ctrl->id][5] = readl(ctrl->regs + S3C_CIOYSA2);
        save_reg[ctrl->id][6] = readl(ctrl->regs + S3C_CIOYSA3);
        save_reg[ctrl->id][7] = readl(ctrl->regs + S3C_CIOYSA4);
        save_reg[ctrl->id][8] = readl(ctrl->regs + S3C_CIOCBSA1);
        save_reg[ctrl->id][9] = readl(ctrl->regs + S3C_CIOCBSA2);
        save_reg[ctrl->id][10] = readl(ctrl->regs + S3C_CIOCBSA3);
        save_reg[ctrl->id][11] = readl(ctrl->regs + S3C_CIOCBSA4);
        save_reg[ctrl->id][12] = readl(ctrl->regs + S3C_CIOCRSA1);
        save_reg[ctrl->id][13] = readl(ctrl->regs + S3C_CIOCRSA2);
        save_reg[ctrl->id][14] = readl(ctrl->regs + S3C_CIOCRSA3);
        save_reg[ctrl->id][15] = readl(ctrl->regs + S3C_CIOCRSA4);
        save_reg[ctrl->id][16] = readl(ctrl->regs + S3C_CITRGFMT);
        save_reg[ctrl->id][17] = readl(ctrl->regs + S3C_CIOCTRL);
        save_reg[ctrl->id][18] = readl(ctrl->regs + S3C_CISCPRERATIO);
        save_reg[ctrl->id][19] = readl(ctrl->regs + S3C_CISCPREDST);
        save_reg[ctrl->id][20] = readl(ctrl->regs + S3C_CISCCTRL);
        save_reg[ctrl->id][21] = readl(ctrl->regs + S3C_CITAREA);
        save_reg[ctrl->id][22] = readl(ctrl->regs + S3C_CISTATUS);
        save_reg[ctrl->id][23] = readl(ctrl->regs + S3C_CIIMGCPT);
        save_reg[ctrl->id][24] = readl(ctrl->regs + S3C_CICPTSEQ);
        save_reg[ctrl->id][25] = readl(ctrl->regs + S3C_CIIMGEFF);
        save_reg[ctrl->id][26] = readl(ctrl->regs + S3C_CIIYSA0);
        save_reg[ctrl->id][27] = readl(ctrl->regs + S3C_CIICBSA0);
        save_reg[ctrl->id][28] = readl(ctrl->regs + S3C_CIICRSA0);
        save_reg[ctrl->id][29] = readl(ctrl->regs + S3C_CIREAL_ISIZE);
        save_reg[ctrl->id][30] = readl(ctrl->regs + S3C_MSCTRL  );
        save_reg[ctrl->id][31] = readl(ctrl->regs + S3C_CIOYOFF);
        save_reg[ctrl->id][32] = readl(ctrl->regs + S3C_CIOCBOFF);
        save_reg[ctrl->id][33] = readl(ctrl->regs + S3C_CIOCROFF);
        save_reg[ctrl->id][34] = readl(ctrl->regs + S3C_CIIYOFF);
        save_reg[ctrl->id][35] = readl(ctrl->regs + S3C_CIICBOFF);
        save_reg[ctrl->id][36] = readl(ctrl->regs + S3C_CIICROFF);
        save_reg[ctrl->id][37] = readl(ctrl->regs + S3C_ORGISIZE);
        save_reg[ctrl->id][38] = readl(ctrl->regs + S3C_ORGOSIZE);
        save_reg[ctrl->id][39] = readl(ctrl->regs + S3C_CIEXTEN);
        save_reg[ctrl->id][40] = readl(ctrl->regs + S3C_CIDMAPARAM);
        save_reg[ctrl->id][41] = readl(ctrl->regs + S3C_CSIIMGFMT);
}

void fimc_print_regs(struct fimc_control *ctrl)
{
        printk("\n %x reg = 0x%x\n", S3C_CISRCFMT, readl(ctrl->regs + S3C_CISRCFMT));
        printk("\n %x reg = 0x%x\n", S3C_CIWDOFST, readl(ctrl->regs + S3C_CIWDOFST));
        printk("\n %x reg = 0x%x\n", S3C_CIGCTRL, readl(ctrl->regs + S3C_CIGCTRL));
        printk("\n %x reg = 0x%x\n", S3C_CIWDOFST2, readl(ctrl->regs + S3C_CIWDOFST2));
        printk("\n %x reg = 0x%x\n", S3C_CIOYSA1, readl(ctrl->regs + S3C_CIOYSA1));
        printk("\n %x reg = 0x%x\n", S3C_CIOYSA2, readl(ctrl->regs + S3C_CIOYSA2));
        printk("\n %x reg = 0x%x\n", S3C_CIOYSA3, readl(ctrl->regs + S3C_CIOYSA3));
        printk("\n %x reg = 0x%x\n", S3C_CIOYSA4, readl(ctrl->regs + S3C_CIOYSA4));
        printk("\n %x reg = 0x%x\n", S3C_CIOCBSA1, readl(ctrl->regs + S3C_CIOCBSA1));
        printk("\n %x reg = 0x%x\n", S3C_CIOCBSA2, readl(ctrl->regs + S3C_CIOCBSA2));
        printk("\n %x reg = 0x%x\n", S3C_CIOCBSA3, readl(ctrl->regs + S3C_CIOCBSA3));
        printk("\n %x reg = 0x%x\n", S3C_CIOCBSA4, readl(ctrl->regs + S3C_CIOCBSA4));
        printk("\n %x reg = 0x%x\n", S3C_CIOCRSA1, readl(ctrl->regs + S3C_CIOCRSA1));
        printk("\n %x reg = 0x%x\n", S3C_CIOCRSA2, readl(ctrl->regs + S3C_CIOCRSA2));
        printk("\n %x reg = 0x%x\n", S3C_CIOCRSA3, readl(ctrl->regs + S3C_CIOCRSA3));
        printk("\n %x reg = 0x%x\n", S3C_CIOCRSA4, readl(ctrl->regs + S3C_CIOCRSA4));
        printk("\n %x reg = 0x%x\n", S3C_CITRGFMT, readl(ctrl->regs + S3C_CITRGFMT));
        printk("\n %x reg = 0x%x\n", S3C_CIOCTRL, readl(ctrl->regs + S3C_CIOCTRL));
        printk("\n %x reg = 0x%x\n", S3C_CISCPRERATIO, readl(ctrl->regs + S3C_CISCPRERATIO));
        printk("\n %x reg = 0x%x\n", S3C_CISCPREDST, readl(ctrl->regs + S3C_CISCPREDST));
        printk("\n %x reg = 0x%x\n", S3C_CISCCTRL, readl(ctrl->regs + S3C_CISCCTRL));
        printk("\n %x reg = 0x%x\n", S3C_CITAREA, readl(ctrl->regs + S3C_CITAREA));
        printk("\n %x reg = 0x%x\n", S3C_CISTATUS, readl(ctrl->regs + S3C_CISTATUS));
        printk("\n %x reg = 0x%x\n", S3C_CIIMGCPT, readl(ctrl->regs + S3C_CIIMGCPT));
        printk("\n %x reg = 0x%x\n", S3C_CICPTSEQ, readl(ctrl->regs + S3C_CICPTSEQ));
        printk("\n %x reg = 0x%x\n", S3C_CIIMGEFF, readl(ctrl->regs + S3C_CIIMGEFF));
        printk("\n %x reg = 0x%x\n", S3C_CIIYSA0, readl(ctrl->regs + S3C_CIIYSA0));
        printk("\n %x reg = 0x%x\n", S3C_CIICBSA0, readl(ctrl->regs + S3C_CIICBSA0));
        printk("\n %x reg = 0x%x\n", S3C_CIICRSA0, readl(ctrl->regs + S3C_CIICRSA0));
        printk("\n %x reg = 0x%x\n", S3C_CIREAL_ISIZE, readl(ctrl->regs + S3C_CIREAL_ISIZE));
        printk("\n %x reg = 0x%x\n", S3C_MSCTRL, readl(ctrl->regs + S3C_MSCTRL));
        printk("\n %x reg = 0x%x\n", S3C_CIOYOFF, readl(ctrl->regs + S3C_CIOYOFF));
        printk("\n %x reg = 0x%x\n", S3C_CIOCBOFF, readl(ctrl->regs + S3C_CIOCBOFF));
        printk("\n %x reg = 0x%x\n", S3C_CIOCROFF, readl(ctrl->regs + S3C_CIOCROFF));
        printk("\n %x reg = 0x%x\n", S3C_CIIYOFF, readl(ctrl->regs + S3C_CIIYOFF));
        printk("\n %x reg = 0x%x\n", S3C_CIICBOFF, readl(ctrl->regs + S3C_CIICBOFF));
        printk("\n %x reg = 0x%x\n", S3C_CIICROFF, readl(ctrl->regs + S3C_CIICROFF));
        printk("\n %x reg = 0x%x\n", S3C_ORGISIZE, readl(ctrl->regs + S3C_ORGISIZE));
        printk("\n %x reg = 0x%x\n", S3C_ORGOSIZE, readl(ctrl->regs + S3C_ORGOSIZE));
        printk("\n %x reg = 0x%x\n", S3C_CIEXTEN, readl(ctrl->regs + S3C_CIEXTEN));
        printk("\n %x reg = 0x%x\n", S3C_CIDMAPARAM, readl(ctrl->regs + S3C_CIDMAPARAM));
        printk("\n %x reg = 0x%x\n", S3C_CSIIMGFMT, readl(ctrl->regs + S3C_CSIIMGFMT));
}

void fimc_load_regs(struct fimc_control *ctrl)
{
        writel(save_reg[ctrl->id][0], ctrl->regs + S3C_CISRCFMT);
        writel(save_reg[ctrl->id][1], ctrl->regs + S3C_CIWDOFST);
        writel(save_reg[ctrl->id][2], ctrl->regs + S3C_CIGCTRL);
        writel(save_reg[ctrl->id][3], ctrl->regs + S3C_CIWDOFST2);
        writel(save_reg[ctrl->id][4], ctrl->regs + S3C_CIOYSA1);
        writel(save_reg[ctrl->id][5], ctrl->regs + S3C_CIOYSA2);
        writel(save_reg[ctrl->id][6], ctrl->regs + S3C_CIOYSA3);
        writel(save_reg[ctrl->id][7], ctrl->regs + S3C_CIOYSA4);
        writel(save_reg[ctrl->id][8], ctrl->regs + S3C_CIOCBSA1);
        writel(save_reg[ctrl->id][9], ctrl->regs + S3C_CIOCBSA2);
        writel(save_reg[ctrl->id][10], ctrl->regs + S3C_CIOCBSA3);
        writel(save_reg[ctrl->id][11], ctrl->regs + S3C_CIOCBSA4);
        writel(save_reg[ctrl->id][12], ctrl->regs + S3C_CIOCRSA1);
        writel(save_reg[ctrl->id][13], ctrl->regs + S3C_CIOCRSA2);
        writel(save_reg[ctrl->id][14], ctrl->regs + S3C_CIOCRSA3);
        writel(save_reg[ctrl->id][15], ctrl->regs + S3C_CIOCRSA4);
        writel(save_reg[ctrl->id][16], ctrl->regs + S3C_CITRGFMT);
        writel(save_reg[ctrl->id][17], ctrl->regs + S3C_CIOCTRL);
        writel(save_reg[ctrl->id][18], ctrl->regs + S3C_CISCPRERATIO);
        writel(save_reg[ctrl->id][19], ctrl->regs + S3C_CISCPREDST);
        writel(save_reg[ctrl->id][20], ctrl->regs + S3C_CISCCTRL);
        writel(save_reg[ctrl->id][21], ctrl->regs + S3C_CITAREA);
        writel(save_reg[ctrl->id][22], ctrl->regs + S3C_CISTATUS);
        writel(save_reg[ctrl->id][24], ctrl->regs + S3C_CICPTSEQ);
        writel(save_reg[ctrl->id][25], ctrl->regs + S3C_CIIMGEFF);
        writel(save_reg[ctrl->id][26], ctrl->regs + S3C_CIIYSA0);
        writel(save_reg[ctrl->id][27], ctrl->regs + S3C_CIICBSA0);
        writel(save_reg[ctrl->id][28], ctrl->regs + S3C_CIICRSA0);
        writel(save_reg[ctrl->id][29], ctrl->regs + S3C_CIREAL_ISIZE);
        writel(save_reg[ctrl->id][30], ctrl->regs + S3C_MSCTRL  );
        writel(save_reg[ctrl->id][31], ctrl->regs + S3C_CIOYOFF);
        writel(save_reg[ctrl->id][32], ctrl->regs + S3C_CIOCBOFF);
        writel(save_reg[ctrl->id][33], ctrl->regs + S3C_CIOCROFF);
        writel(save_reg[ctrl->id][34], ctrl->regs + S3C_CIIYOFF);
        writel(save_reg[ctrl->id][35], ctrl->regs + S3C_CIICBOFF);
        writel(save_reg[ctrl->id][36], ctrl->regs + S3C_CIICROFF);
        writel(save_reg[ctrl->id][37], ctrl->regs + S3C_ORGISIZE);
        writel(save_reg[ctrl->id][38], ctrl->regs + S3C_ORGOSIZE);
        writel(save_reg[ctrl->id][39], ctrl->regs + S3C_CIEXTEN);
        writel(save_reg[ctrl->id][40], ctrl->regs + S3C_CIDMAPARAM);
        writel(save_reg[ctrl->id][41], ctrl->regs + S3C_CSIIMGFMT);
        writel(save_reg[ctrl->id][23], ctrl->regs + S3C_CIIMGCPT);
}
