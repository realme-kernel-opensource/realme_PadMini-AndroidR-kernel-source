/*
 * sound/soc/sprd/platform
 *
 * Copyright (C) 2015 SpreadTrum Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY ork FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __SPRD_PLATFORM_PCM_ROUTING_H
#define __SPRD_PLATFORM_PCM_ROUTING_H

/* FE dai id*/
enum {
	FE_DAI_ID_NORMAL_AP01 = 0,
	FE_DAI_ID_NORMAL_AP23,
	FE_DAI_ID_CAPTURE_DSP,
	FE_DAI_ID_FAST_P,
	FE_DAI_ID_OFFLOAD,
	FE_DAI_ID_VOICE,
	FE_DAI_ID_VOIP,
	FE_DAI_ID_FM,
	FE_DAI_ID_FM_CAPTURE_AP,
	FE_DAI_ID_VOICE_CAPTURE,
	FE_DAI_ID_LOOP,
	FE_DAI_ID_A2DP_OFFLOAD,
	FE_DAI_ID_A2DP_PCM,
	FE_DAI_ID_FM_CAP_DSP,
	FE_DAI_ID_BTSCO_CAP_DSP,
	FE_DAI_ID_FM_DSP,
	FE_DAI_ID_DUMP,
	FE_DAI_ID_BTSCO_CAP_AP,
	FE_DAI_ID_VOICE_PCM_P,
	FE_DAI_ID_CODEC_TEST,
	FE_DAI_ID_HFP,
	FE_DAI_ID_RECOGNISE_CAPTURE,
	FE_DAI_ID_HIFI_P,
	FE_DAI_ID_HIFI_FAST_P,
	FE_DAI_ID_MAX
};

/* BE dais id*/
enum {
	/* codec */
	BE_DAI_ID_NORMAL_AP01_CODEC = 0,
	BE_DAI_ID_NORMAL_AP23_CODEC,
	BE_DAI_ID_CAPTURE_DSP_CODEC,
	BE_DAI_ID_FAST_P_CODEC,
	BE_DAI_ID_OFFLOAD_CODEC,
	BE_DAI_ID_VOICE_CODEC,
	BE_DAI_ID_VOIP_CODEC,
	BE_DAI_ID_FM_CODEC,
	BE_DAI_ID_LOOP_CODEC,
	BE_DAI_ID_FM_DSP_CODEC,
	/* usb */
	BE_DAI_ID_NORMAL_AP01_USB,
	BE_DAI_ID_NORMAL_AP23_USB,
	BE_DAI_ID_CAPTURE_DSP_USB,
	BE_DAI_ID_FAST_P_USB,
	BE_DAI_ID_OFFLOAD_USB,
	BE_DAI_ID_VOICE_USB,
	BE_DAI_ID_VOIP_USB,
	BE_DAI_ID_FM_USB,
	BE_DAI_ID_LOOP_USB,
	BE_DAI_ID_FM_DSP_USB,
	/* bt */
	BE_DAI_ID_OFFLOAD_A2DP,
	BE_DAI_ID_PCM_A2DP,
	BE_DAI_ID_VOICE_BT,
	BE_DAI_ID_VOIP_BT,
	BE_DAI_ID_LOOP_BT,
	BE_DAI_ID_CAPTURE_BT,
	BE_DAI_ID_CAPTURE_DSP_BTSCO,
	BE_DAI_ID_FAST_P_BTSCO,
	BE_DAI_ID_NORMAL_AP01_P_BTSCO,
	/* hifi */
	BE_DAI_ID_NORMAL_AP01_P_HIFI,
	BE_DAI_ID_NORMAL_AP23_HIFI,
	BE_DAI_ID_FAST_P_HIFI,
	BE_DAI_ID_OFFLOAD_HIFI,
	BE_DAI_ID_VOICE_HIFI,
	BE_DAI_ID_VOIP_HIFI,
	BE_DAI_ID_FM_HIFI,
	BE_DAI_ID_LOOP_HIFI,
	BE_DAI_ID_FM_DSP_HIFI,

	/* common */
	BE_DAI_ID_VOICE_CAPTURE,
	BE_DAI_ID_FM_CAPTURE,
	BE_DAI_ID_FM_CAPTURE_DSP,
	BE_DAI_ID_DUMP,
	BE_DAI_ID_DUMMY_VBC_DAI_NOTBE,
	BE_DAI_ID_HFP,
	BE_DAI_ID_RECOGNISE_CAPTURE,
	BE_DAI_ID_VOICE_PCM_P,

	/* smart pa */
	BE_DAI_ID_NORMAL_AP01_P_SMTPA,
	BE_DAI_ID_NORMAL_AP23_SMTPA,
	BE_DAI_ID_FAST_P_SMTPA,
	BE_DAI_ID_OFFLOAD_SMTPA,
	BE_DAI_ID_VOICE_SMTPA,
	BE_DAI_ID_VOIP_SMTPA,
	BE_DAI_ID_FM_SMTPA,
	BE_DAI_ID_LOOP_SMTPA,
	BE_DAI_ID_FM_DSP_SMTPA,
	BE_DAI_ID_HIFI_P,
	BE_DAI_ID_HIFI_FAST_P,
	BE_DAI_ID_MAX
};
#endif
