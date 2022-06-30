/* #include "mm_dvfs_reg.h" */
#include "mmsys_dvfs_comm.h"
#include "mm_devfreq_common.h"
#include "mm_dvfs_coffe.h"
#include "mm_dvfs_table.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "mmsys-dvfs: vdsp_dvfs %d %s : "\
	fmt, __LINE__, __func__

/* userspace  interface*/
static int ip_hw_dvfs_en(struct devfreq *devfreq, unsigned int dvfs_eb)
{
/*    u32 dfs_en_reg;
    mutex_lock(&mmsys_glob_reg_lock);
    dfs_en_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL);
    if (dvfs_eb)
        DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
                    dfs_en_reg | BIT_VDSP_DFS_EN);
    else
        DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_EN_CTRL,
                    dfs_en_reg & (~BIT_VDSP_DFS_EN));
    mutex_unlock(&mmsys_glob_reg_lock);
*/
	pr_info("dvfs ops: %s, dvfs_eb=%d\n", __func__, dvfs_eb);
	return MM_DVFS_SUCCESS;
}

static int ip_auto_tune_en(struct devfreq *devfreq, unsigned long dvfs_eb)
{
	pr_info("dvfs ops: %s\n", __func__);
	/* vdsp has no this funcation */
	return MM_DVFS_SUCCESS;
}

/*work-idle dvfs map  ops*/
static int get_ip_dvfs_table(struct devfreq *devfreq,
			     struct ip_dvfs_map_cfg *dvfs_table)
{
	int i = 0;

	pr_info("dvfs ops: %s\n", __func__);
	for (i = 0; i < 8; i++) {
		dvfs_table[i].map_index = vdsp_dvfs_config_table[i].map_index;
		dvfs_table[i].clk_freq = vdsp_dvfs_config_table[i].clk_freq;
		dvfs_table[i].clk = vdsp_dvfs_config_table[i].clk;
		dvfs_table[i].volt_value = vdsp_dvfs_config_table[i].volt_value;
		dvfs_table[i].volt = vdsp_dvfs_config_table[i].volt;
		dvfs_table[i].fdiv_denom = vdsp_dvfs_config_table[i].fdiv_denom;
		dvfs_table[i].fdiv_num = vdsp_dvfs_config_table[i].fdiv_num;
		dvfs_table[i].axi_index = vdsp_dvfs_config_table[i].axi_index;
		dvfs_table[i].mtx_index = vdsp_dvfs_config_table[i].mtx_index;
		dvfs_table[i].reg_add = vdsp_dvfs_config_table[i].reg_add;
	}
	return MM_DVFS_SUCCESS;
}

static int set_ip_dvfs_table(struct devfreq *devfreq,
			     struct ip_dvfs_map_cfg *dvfs_table)
{
	memcpy(vdsp_dvfs_config_table, dvfs_table, sizeof(struct ip_dvfs_map_cfg));
	pr_info("dvfs ops: %s\n", __func__);
	return MM_DVFS_SUCCESS;
}

static void get_ip_index_from_table(struct ip_dvfs_map_cfg *dvfs_cfg,
				    unsigned long work_freq,
				    unsigned int *index)
{
	unsigned long set_clk = 0;
	u32 i;

	*index = 0;
	for (i = 0; i < 8; i++) {
		set_clk = vdsp_dvfs_config_table[i].clk_freq;
		if (work_freq == set_clk || work_freq < set_clk) {
			*index = i;
			break;
		}
		if (i == 7)
			*index = 7;
	}
	pr_info("index=%d work_freq %ld", *index, work_freq);
}

static void set_change_flag(unsigned long work_freq)
{
	u32 i =0, clk_reg = 0, ip_clk_sel = 0;
	unsigned long current_ip_clk = 0;

	clk_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_VDSP_DVFS_CGM_CFG_DBG);
	ip_clk_sel = (clk_reg >> SHFT_BITS_CGM_VDSP_SEL_DVFS)
		     & MASK_BITS_CGM_VDSP_SEL_DVFS;

	for (i = 0; i < 8; i++) {
		if (ip_clk_sel == vdsp_dvfs_config_table[i].clk) {
			current_ip_clk = vdsp_dvfs_config_table[i].clk_freq;
			break;
		}
	}

	if (current_ip_clk < work_freq)
		clk_vote_table[VDSP_MTX_DATA_CLK].ip_change_flag = 1;/*hww ? VDSP_MTX_DATA_CLK*/
	else
		clk_vote_table[VDSP_MTX_DATA_CLK].ip_change_flag = 0;
}

static void updata_clk_vote_table(unsigned int *mtx_index,
				  unsigned long work_freq)
{
	u32 i =0, arb_clk = 0;
	clk_vote_table[VDSP_MTX_DATA_CLK].clk_value = work_freq;
	arb_clk = clk_vote_table[0].clk_value;
	pr_info("clk_vote_table[0].clk_value:%ud",
		clk_vote_table[0].clk_value);
	for (i = 1; i < VOTE_MAX_CLK; i++) {
		if (arb_clk < clk_vote_table[i].clk_value)
			arb_clk = clk_vote_table[i].clk_value;
		pr_info("clk_vote_table[%d].clk_value:%ud",
			i, clk_vote_table[i].clk_value);
	}

	for (i = 0; i < 8; i++) {
		if (arb_clk <= mtx_data_dvfs_config_table[i].clk_freq) {
			*mtx_index = mtx_data_dvfs_config_table[i].map_index;
			break;
		}
		*mtx_index = 7;
	}
	pr_info("mtx_index=%d work_freq %ld", *mtx_index, work_freq);
}

static int set_vdsp_work_freq(unsigned int index)
{
	u32 index_cfg_reg = 0;

	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_CFG, index_cfg_reg);
	pr_info("dvfs ops: %s, index=%d,\n", __func__, index);
	return MM_DVFS_SUCCESS;
}

static void change_vdspmtx_index(unsigned int *vdspmtx_index,
				 unsigned long work_freq)
{
	u32 i =0;
	for (i = 0; i < 8; i++) {
		if (work_freq <= vdsp_mtx_dvfs_config_table[i].clk_freq) {
			*vdspmtx_index = vdsp_mtx_dvfs_config_table[i].map_index;
			break;
		}
		*vdspmtx_index = 7;
	}
	pr_info("vdspmtx_index=%d work_freq %ld", *vdspmtx_index, work_freq);
}

static int set_work_freq(struct devfreq *devfreq, unsigned long work_freq)
{
	u32 index = 0, mtx_index = 0, vdspmtx_index = 0;

	get_ip_index_from_table(vdsp_dvfs_config_table, work_freq, &index);
	set_change_flag(work_freq);
	updata_clk_vote_table(&mtx_index, work_freq / 2);
	change_vdspmtx_index(&vdspmtx_index, work_freq / 2);

	if (clk_vote_table[VDSP_MTX_DATA_CLK].ip_change_flag) {
		set_mtx_data_work_freq(mtx_index);
		set_vdsp_mtx_work_freq(vdspmtx_index);
		set_vdsp_work_freq(index);
	} else {
		set_vdsp_work_freq(index);
		set_vdsp_mtx_work_freq(vdspmtx_index);
		set_mtx_data_work_freq(mtx_index);
	}
	return MM_DVFS_SUCCESS;
}

static int set_idle_freq(struct devfreq *devfreq, unsigned long idle_freq)
{
#if 1
    //dvfs_test_case(0);
    //pr_info("dvfs ops: %s, index_cfg=%d\n", __func__, 0);
#else
    u32 index_cfg_reg = 0;
    u32 index = 0;

    get_ip_index_from_table(vdsp_dvfs_config_table, idle_freq, &index);

    index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_IDLE_CFG);
    index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
    DVFS_REG_WR(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_IDLE_CFG, index_cfg_reg);

    pr_info("dvfs ops: %s, work_freq=%lu, index=%d,\n", __func__, idle_freq,
            index);
#endif
	return MM_DVFS_SUCCESS;
}

/* get ip current volt ,clk & map index*/
static int get_ip_status(struct devfreq *devfreq,
			 struct ip_dvfs_status *ip_status)
{
	u32 volt_reg;
	u32 clk_reg;
	u32 i;
	volt_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DVFS_VOLTAGE_DBG1);
	clk_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_VDSP_DVFS_CGM_CFG_DBG);

	ip_status->current_ip_clk =
		(clk_reg >> SHFT_BITS_CGM_VDSP_SEL_DVFS)
		& MASK_BITS_CGM_VDSP_SEL_DVFS;
	ip_status->current_sys_volt =
		((volt_reg >> SHFT_BITS_VDSP_VOLTAGE)
		& MASK_BITS_VDSP_VOLTAGE);
	for (i = 0; i < 8; i++) {
		if (ip_status->current_ip_clk == vdsp_dvfs_config_table[i].clk) {
			ip_status->current_ip_clk = vdsp_dvfs_config_table[i].clk_freq;
			break;
		}
	}
	pr_info("dvfs ops: %s v = %d c = %d\n", __func__, volt_reg, volt_reg);
	return MM_DVFS_SUCCESS;
}

/*coffe setting ops*/
static void set_ip_gfree_wait_delay(unsigned int wind_para)
{
/* hww ??*/
	DVFS_REG_WR(REG_MM_DVFS_AHB_MM_GFREE_WAIT_DELAY_CFG0,
		BITS_VDSP_GFREE_WAIT_DELAY(wind_para));

	pr_info("dvfs ops: %s\n", __func__);
}

static void set_ip_freq_upd_en_byp(unsigned int on)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS);

	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp | BIT_REG_VDSP_FREQ_UPD_EN_BYP);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPDATE_BYPASS,
			reg_temp & (~BIT_REG_VDSP_FREQ_UPD_EN_BYP));
	mutex_unlock(&mmsys_glob_reg_lock);
	pr_info("dvfs ops: %s\n", __func__);
}

static void set_ip_freq_upd_delay_en(unsigned int on)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0);
	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0,
			reg_temp | BIT_VDSP_FREQ_UPD_DELAY_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0,
			reg_temp & (~(BIT_VDSP_FREQ_UPD_DELAY_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s\n", __func__);
}

static void set_ip_freq_upd_hdsk_en(unsigned int on)
{
	u32 reg_temp = 0;

	mutex_lock(&mmsys_glob_reg_lock);
	reg_temp = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0);

	if (on)
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0,
			reg_temp | BIT_VDSP_FREQ_UPD_HDSK_EN);
	else
		DVFS_REG_WR(REG_MM_DVFS_AHB_MM_FREQ_UPD_TYPE_CFG0,
			reg_temp & (~(BIT_VDSP_FREQ_UPD_HDSK_EN)));
	mutex_unlock(&mmsys_glob_reg_lock);

	pr_info("dvfs ops: %s\n", __func__);
}

static void set_ip_dvfs_swtrig_en(unsigned int en) {
/*    u32 dfs_swtrig_en;

    mutex_lock(&mmsys_glob_reg_lock);

    dfs_swtrig_en = DVFS_REG_RD(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG);
    if (en)
        DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
                    dfs_swtrig_en | BIT_VDSP_DFS_SW_TRIG);
    else
        DVFS_REG_WR(REG_MM_DVFS_AHB_MM_DFS_SW_TRIG_CFG,
                    dfs_swtrig_en & (~BIT_VDSP_DFS_SW_TRIG));
    mutex_unlock(&mmsys_glob_reg_lock); */
    pr_info("dvfs ops: %s\n", __func__);
}

/*work-idle dvfs index ops*/
static void set_ip_dvfs_work_index(struct devfreq *devfreq,
				   unsigned int index)
{
	u32 index_cfg_reg = 0;
	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_CFG, index_cfg_reg);
	pr_info("dvfs ops: %s, index_cfg=%d\n", __func__, index_cfg_reg);
}

/*work-idle dvfs index ops*/
static void get_ip_dvfs_work_index(struct devfreq *devfreq,
				   unsigned int *index)
{
	*index = DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_CFG);
	pr_info("dvfs ops: %s, work_index=%d\n", __func__, *index);
}

static void vdsp_dvfs_map_cfg(void)
{
	u32 map_cfg_reg = 0;
	u32 i = 0;

	for (i = 0; i < 8; i++) {
		map_cfg_reg = 0x0;
		map_cfg_reg =
			(map_cfg_reg & (~0x1ff)) |
			BITS_VDSP_VOL_INDEX0(vdsp_dvfs_config_table[i].volt) |
			BITS_CGM_VDSP_M_DIV_INDEX0(0x1) |
			BITS_CGM_VDSP_SEL_INDEX0(vdsp_dvfs_config_table[i].clk);

		/* DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_INDEX0_MAP, map_cfg_reg); */
		//pr_info("dvfs ops: %s vdsp map_cfg_reg=%d-- %d\n", __func__, i,
		//        map_cfg_reg);
		DVFS_REG_WR((vdsp_dvfs_config_table[i].reg_add), map_cfg_reg);
	}
}

static void set_ip_dvfs_idle_index(struct devfreq *devfreq,
				   unsigned int index)
{
	u32 index_cfg_reg = 0;
	index_cfg_reg = DVFS_REG_RD(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_IDLE_CFG);
	index_cfg_reg = (index_cfg_reg & (~0x7)) | index;
	DVFS_REG_WR(REG_MM_DVFS_AHB_VDSP_DVFS_INDEX_IDLE_CFG, index_cfg_reg);
}

static int ip_dvfs_init(struct devfreq *devfreq)
{
	struct module_dvfs *vdsp;
	vdsp = dev_get_drvdata(devfreq->dev.parent);

	if (!vdsp) {
		pr_err("undefined vdsp_dvfs\n");
		return -EINVAL;
	}
	vdsp_dvfs_map_cfg();
	devfreq->max_freq = vdsp_dvfs_config_table[7].clk_freq;
	devfreq->min_freq = vdsp_dvfs_config_table[0].clk_freq;
	set_ip_freq_upd_en_byp(VDSP_FREQ_UPD_EN_BYP);
	set_ip_freq_upd_delay_en(VDSP_FREQ_UPD_DELAY_EN);
	set_ip_freq_upd_hdsk_en(VDSP_FREQ_UPD_HDSK_EN);
	set_ip_gfree_wait_delay(VDSP_GFREE_WAIT_DELAY);
	//set_ip_dvfs_swtrig_en(VDSP_SW_TRIG_EN);
	set_ip_dvfs_work_index(devfreq, VDSP_WORK_INDEX_DEF);
	set_ip_dvfs_idle_index(devfreq, VDSP_IDLE_INDEX_DEF);
	//ip_hw_dvfs_en(devfreq, VDSP_DFS_EN);
	vdsp->dvfs_enable = VDSP_DFS_EN;
	vdsp->freq = vdsp_dvfs_config_table[VDSP_WORK_INDEX_DEF].clk_freq;
	pr_info("vdsp dvfs init param: HDSK_EN %d WORK_INDEX_DEF %d IDLE_INDEX_DEF %d\n",
		VDSP_FREQ_UPD_HDSK_EN, VDSP_WORK_INDEX_DEF, VDSP_IDLE_INDEX_DEF);
	return MM_DVFS_SUCCESS;
}

static int updata_target_freq(struct devfreq *devfreq, unsigned long volt,
			      unsigned long freq, unsigned int eb_sysgov)
{
	if (eb_sysgov == TRUE)
		mmsys_adjust_target_freq(DVFS_VDSP, volt, freq, eb_sysgov);
	else
		set_work_freq(devfreq, freq);

	pr_info("dvfs ops: %s\n", __func__);
	return MM_DVFS_SUCCESS;
}

/*debug interface*/
static int set_fix_dvfs_value(struct devfreq *devfreq, unsigned long fix_volt)
{
	mmsys_set_fix_dvfs_value(fix_volt);

	pr_info("dvfs ops: %s\n,fix_volt=%lu", __func__, fix_volt);
	return MM_DVFS_SUCCESS;
}

static void power_on_nb(struct devfreq *devfreq)
{
	ip_dvfs_init(devfreq);
	pr_info("dvfs ops: vdsp %s\n", __func__);
}

static void power_off_nb(struct devfreq *devfreq)
{
	struct module_dvfs *vdsp;
	vdsp = dev_get_drvdata(devfreq->dev.parent);
	if (vdsp != NULL) {
		pr_info("vdsp_dvfs power off \n");
		vdsp->dvfs_enable = 0;
	}
}

static int top_current_volt(struct devfreq *devfreq, unsigned int *top_volt)
{
	unsigned int ret;

	ret = top_mm_dvfs_current_volt(devfreq);
	*top_volt = ret;
	pr_info("dvfs ops: %s\n", __func__);
	return MM_DVFS_SUCCESS;
}

static int mm_current_volt(struct devfreq *devfreq, unsigned int *mm_volt)
{
	unsigned int ret;

	ret = mm_dvfs_read_current_volt(devfreq);
	*mm_volt = ret;
	pr_info("dvfs ops: %s\n", __func__);
	return MM_DVFS_SUCCESS;
}

static struct ip_dvfs_ops vdsp_dvfs_ops = {

	.name = "VDSP_DVFS_OPS",
	.available = 1,

	.ip_dvfs_init = ip_dvfs_init,
	.ip_hw_dvfs_en = ip_hw_dvfs_en,
	.ip_auto_tune_en = ip_auto_tune_en,
	.set_work_freq = set_work_freq,
	.set_idle_freq = set_idle_freq,
	.get_ip_dvfs_table = get_ip_dvfs_table,
	.set_ip_dvfs_table = set_ip_dvfs_table,
	.get_ip_status = get_ip_status,
	.set_ip_dvfs_work_index = set_ip_dvfs_work_index,
	.get_ip_dvfs_work_index = get_ip_dvfs_work_index,
	.set_ip_dvfs_idle_index = set_ip_dvfs_idle_index,
	.get_ip_work_index_from_table = get_ip_index_from_table,
	.get_ip_idle_index_from_table = get_ip_index_from_table,

	.set_ip_gfree_wait_delay = set_ip_gfree_wait_delay,
	.set_ip_freq_upd_en_byp = set_ip_freq_upd_en_byp,
	.set_ip_freq_upd_delay_en = set_ip_freq_upd_delay_en,
	.set_ip_freq_upd_hdsk_en = set_ip_freq_upd_hdsk_en,
	.set_ip_dvfs_swtrig_en = set_ip_dvfs_swtrig_en,

	.set_fix_dvfs_value = set_fix_dvfs_value,
	.updata_target_freq = updata_target_freq,
	.power_on_nb = power_on_nb,
	.power_off_nb = power_off_nb,

	.top_current_volt = top_current_volt,
	.mm_current_volt = mm_current_volt,
	.event_handler = NULL,
};

struct ip_dvfs_ops *get_vdsp_dvfs_ops(void)
{
	return &vdsp_dvfs_ops;
}
EXPORT_SYMBOL(get_vdsp_dvfs_ops);


