#ifndef __SPRD_FE_DAI_H
#define __SPRD_FE_DAI_H

/* dma hw request */
#define DMA_REQ_DA0_DEV_ID		(1)
#define DMA_REQ_DA1_DEV_ID		(2)
#define DMA_REQ_DA2_DEV_ID		(17)
#define DMA_REQ_DA3_DEV_ID		(18)
#define DMA_REQ_AD0_DEV_ID		(3)
#define DMA_REQ_AD1_DEV_ID		(4)
#define DMA_REQ_AD2_DEV_ID		(15)
#define DMA_REQ_AD3_DEV_ID		(16)

#define VBC_AUDPLY01_FRAGMENT	(80)
#define VBC_AUDPLY23_FRAGMENT	(80)
#define VBC_AUDRCD01_FRAGMENT	(80)
#define VBC_AUDRCD23_FRAGMENT	(80)

/* mcdt channel 5 6 7 has been used by modem */

/* FE_DAI_ID_VOIP */
#define MCDT_CHAN_VOIP MCDT_CHAN1
#define MCDT_FULL_WMK_VOIP (160)
#define MCDT_EMPTY_WMK_VOIP (320)
#define MCDT_VOIP_P_FRAGMENT (160)
#define MCDT_VOIP_C_FRAGMENT (160)

/* FE_DAI_ID_VOICE_CAPTURE */
#define MCDT_CHAN_VOICE_CAPTURE MCDT_CHAN2
#define MCDT_FULL_WMK_VOICE_CAPTURE 160
#define MCDT_VOICE_C_FRAGMENT 160

/* FE_DAI_ID_LOOP: reuse MCDT_CHAN3 playback path */
#define MCDT_CHAN_LOOP MCDT_CHAN3
#define MCDT_LOOP_P_FRAGMENT 160
#define MCDT_LOOP_C_FRAGMENT 160
#define MCDT_FULL_WMK_LOOP 160
#define MCDT_EMPTY_WMK_LOOP 160

/* FE_DAI_ID_A2DP_PCM: reuse MCDT_CHAN3 playback path */
#define MCDT_CHAN_A2DP_PCM MCDT_CHAN_LOOP
#define MCDT_EMPTY_WMK_A2DP_PCM 320
#define MCDT_A2DP_PCM_FRAGMENT 160

/* FE_DAI_ID_FAST_P: use MCDT_CHAN4 playback path */
#define MCDT_CHAN_FAST_PLAY MCDT_CHAN4
#define MCDT_EMPTY_WMK_FAST_PLAY 320
#define MCDT_FAST_PLAY_FRAGMENT 160

/* FE_DAI_ID_HIFI_FAST_P: use MCDT_CHAN10 playback path */
#define MCDT_CHAN_HIFI_FAST_PLAY MCDT_CHAN10
#define MCDT_EMPTY_WMK_HIFI_FAST_PLAY 320
#define MCDT_HIFI_FAST_PLAY_FRAGMENT 160

/* FE_DAI_ID_CAPTURE_DSP: use MCDT_CHAN4 capture path */
#define MCDT_CHAN_DSP_CAP MCDT_CHAN4
#define MCDT_FULL_WMK_DSP_CAP 320
#define MCDT_DSPCAP_FRAGMENT 80
/* FE_DAI_ID_FM_CAP_DSP: same as FE_DAI_ID_CAPTURE_DSP */
#define MCDT_CHAN_DSP_FM_CAP MCDT_CHAN4
#define MCDT_FULL_WMK_DSP_FM_CAP 320
#define MCDT_DSPFMCAP_FRAGMENT 320
/* FE_DAI_ID_BTSCO_CAP_DSP: same as FE_DAI_ID_CAPTURE_DSP */
#define MCDT_CHAN_DSP_BTSCO_CAP MCDT_CHAN4
#define MCDT_FULL_WMK_DSP_BTSCO_CAP 320
#define MCDT_DSPBTSCOCAP_FRAGMENT 320

/* FE_DAI_ID_RECOGNISE_CAPTURE */
#define MCDT_CHAN_RECOGNISE_CAPTURE MCDT_CHAN2
#define MCDT_FULL_WMK_RECOGNISE_CAPTURE 320
#define MCDT_RECOGNISE_C_FRAGMENT 320

/* FE_DAI_ID_VOICE_PCM_P */
#define MCDT_CHAN_VOICE_PCM_P MCDT_CHAN1
#define MCDT_EMPTY_WMK_VOICE_PCM_P (320)
#define MCDT_VOICE_PCM_P_FRAGMENT (160)

/* FE_DAI_ID_HIFI_P: use MCDT_CHAN4 playback path */
#define MCDT_CHAN_HIFI_PLAY MCDT_CHAN2
#define MCDT_EMPTY_WMK_HIFI_PLAY 320
#define MCDT_HIFI_PLAY_FRAGMENT 160

#endif

