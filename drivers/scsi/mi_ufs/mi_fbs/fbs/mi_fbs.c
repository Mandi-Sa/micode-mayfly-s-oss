/*
 * mi_fbs.c
 *
 * Created on: 2021-09-17
 *
 * Authors:
 *	kook <zhangbinbin1@xiaomi.com>
 *	cuijiguang <cuijiguang@xiaomi.com>
 *	lijiaming <lijiaming3@xiaomi.com>
 */
#include <linux/pm_runtime.h>
#include "mi_fbs.h"
#include "mi_fbs_sysfs.h"
#include "../../mi-ufshcd.h"
#include "../ufs/ufs-qcom.h"

#define FBS_READ_LBA_RANGE_MODE 0x1d
#define FBS_READ_LBA_RANGE_BUF_ID 0x00
#define FBS_READ_LBA_RANGE_BUF_OFFSET 0x00
#define FBS_WRITE_LBA_RANGE_MODE 0x1d
#define FBS_WRITE_LBA_RANGE_BUF_ID 0x00
#define FBS_WRITE_LBA_RANGE_BUF_OFFSET 0x00

struct standard_inquiry {
	uint8_t vendor_id[8];
	uint8_t product_id[16];
	uint8_t product_rev[4];
};

#define FBS_DEBUG(fbs, msg, args...)					\
	do { if (fbs->fbs_debug)					\
		pr_err("%40s:%3d [%01d%02d%02d] " msg "\n",		\
		       __func__, __LINE__,				\
			   fbs->fbs_trigger,				\
		       atomic_read(&fbs->hba->dev->power.usage_count),\
			   fbs->hba->clk_gating.active_reqs, ##args);	\
	} while (0)

extern struct ufsfbs_ops wdc_fbs_ops;
extern struct ufsfbs_ops micron_fbs_ops;
extern struct ufsfbs_ops samsung_fbs_ops;
extern struct ufsfbs_ops hynix_fbs_ops;

struct fbs_vendor_ops fbs_ops_arry[] = {
		{"WDC", FBS_AH8_ENABLE, &wdc_fbs_ops},
		{"SAMSUNG", FBS_AH8_ENABLE, &samsung_fbs_ops},
		{"MICRON", FBS_AH8_ENABLE, &micron_fbs_ops},
		{"SKhynix", FBS_AH8_ENABLE, &hynix_fbs_ops},
};

int ufsfbs_init_ops(struct ufsfbs_dev *fbs)
{
	struct standard_inquiry stdinq = {};
	int ret = -1;
	int i = 0;
	struct ufs_hba *hba = fbs->hba;

	memcpy(&stdinq, hba->sdev_ufs_device->inquiry + 8, sizeof(stdinq));

	for (i = 0; i < sizeof(fbs_ops_arry)/sizeof(fbs_ops_arry[0]); i++) {
		if (strncmp((char *)stdinq.vendor_id, (char *)fbs_ops_arry[i].vendor_id, strlen((char *)fbs_ops_arry[i].vendor_id)) == 0) {
			hba->fbs.fbs_ops = fbs_ops_arry[i].fbs_ops;
			fbs->fbs_vendor_ops = &fbs_ops_arry[i];
			if(!strncmp((char *)stdinq.vendor_id, "SKhynix", strlen("SKhynix"))){
				fbs->is_skhynix = true;
				FBS_INFO_MSG("[ufsfbs]is SKhynix:%d", fbs->is_skhynix);
			}
			return 0;
		}
	}
	return ret;
}

int ufsfbs_is_not_present(struct ufsfbs_dev *fbs)
{
	enum UFSFBS_STATE cur_state = ufsfbs_get_state(fbs->hba);

	if (cur_state != FBS_PRESENT) {
		FBS_INFO_MSG("[ufsfbs]fbs_state != fbs_PRESENT (%d)", cur_state);
		return -ENODEV;
	}
	return 0;
}

inline int ufsfbs_get_state(struct ufs_hba *hba)
{
	return atomic_read(&hba->fbs.fbs_state);
}

inline void ufsfbs_set_state(struct ufs_hba *hba, int state)
{
	atomic_set(&hba->fbs.fbs_state, state);
}

int ufsfbs_frag_level_check_enable(struct ufsfbs_dev *fbs, bool val)
{
	int ret = 0;

	pm_runtime_get_sync(fbs->hba->dev);
	ret = fbs->fbs_ops->fbs_frag_level_check_enable(fbs, val);
	pm_runtime_put_sync(fbs->hba->dev);
	return ret;
}

int ufsfbs_defrag_enable(struct ufsfbs_dev *fbs, bool val)
{
	int ret = 0;

	pm_runtime_get_sync(fbs->hba->dev);
	ret = fbs->fbs_ops->fbs_defrag_enable(fbs, val);
	pm_runtime_put_sync(fbs->hba->dev);
	return ret;
}

/*
 * Lock status: fbs_sysfs lock was held when called.
 */
void ufsfbs_auto_hibern8_enable(struct ufsfbs_dev *fbs,
				       unsigned int val)
{
	struct ufs_hba *hba = fbs->hba;
	unsigned long flags;
	u32 reg;

	val = !!val;

	/* Update auto hibern8 timer value if supported */
	if (!ufshcd_is_auto_hibern8_supported(hba))
		return;

	pm_runtime_get_sync(hba->dev);
	ufshcd_hold(hba, false);
	down_write(&hba->clk_scaling_lock);
	ufshcd_scsi_block_requests(hba);
	/* wait for all the outstanding requests to finish */
	ufshcd_wait_for_doorbell_clr(hba, U64_MAX);
	spin_lock_irqsave(hba->host->host_lock, flags);

	reg = ufshcd_readl(hba, REG_AUTO_HIBERNATE_IDLE_TIMER);
	FBS_INFO_MSG("[ufsfbs]ahit-reg:0x%X", reg);

	if (val ^ (reg == hba->ahit)) {
		ufshcd_writel(hba, val ? hba->ahit : FBS_AUTO_HIBERN8_DISABLE,
			      REG_AUTO_HIBERNATE_IDLE_TIMER);
		/* Make sure the timer gets applied before further operations */
		mb();

		FBS_INFO_MSG("[ufsfbs][Before]is_auto_enabled:%d", fbs->is_auto_enabled);
		fbs->is_auto_enabled = val;

		reg = ufshcd_readl(hba, REG_AUTO_HIBERNATE_IDLE_TIMER);
		FBS_INFO_MSG("[ufsfbs][After]is_auto_enabled:%d,ahit-reg:0x%X",
			 fbs->is_auto_enabled, reg);
	} else {
		FBS_INFO_MSG("[ufsfbs]is_auto_enabled:%d. so it does not changed",
			 fbs->is_auto_enabled);
	}

	spin_unlock_irqrestore(hba->host->host_lock, flags);
	ufshcd_scsi_unblock_requests(hba);
	up_write(&hba->clk_scaling_lock);
	ufshcd_release(hba);
	pm_runtime_put_sync(hba->dev);
}

void ufsfbs_block_enter_suspend(struct ufsfbs_dev *fbs)
{
	struct ufs_hba *hba = fbs->hba;
	unsigned long flags;

	if (unlikely(fbs->block_suspend))
		return;

	fbs->block_suspend = true;

	pm_runtime_get_sync(hba->dev);
	ufshcd_hold(hba, false);

	spin_lock_irqsave(hba->host->host_lock, flags);
	FBS_DEBUG(fbs,
		  "[ufsfbs]power.usage_count:%d,clk_gating.active_reqs:%d",
		  atomic_read(&hba->dev->power.usage_count),
		  hba->clk_gating.active_reqs);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
}

void ufsfbs_allow_enter_suspend(struct ufsfbs_dev *fbs)
{
	struct ufs_hba *hba = fbs->hba;
	unsigned long flags;

	if (unlikely(!fbs->block_suspend))
		return;

	fbs->block_suspend = false;

	ufshcd_release(hba);
	pm_runtime_mark_last_busy(hba->dev);
	pm_runtime_put_noidle(hba->dev);

	spin_lock_irqsave(hba->host->host_lock, flags);
	FBS_DEBUG(fbs,
		  "[ufsfbs]power.usage_count:%d,clk_gating.active_reqs:%d",
		  atomic_read(&hba->dev->power.usage_count),
		  hba->clk_gating.active_reqs);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
}

int ufsfbs_fbs_support(struct ufsfbs_dev *fbs, u8 *fbs_support_en)
{

	int ret = 0;

	ret = fbs->fbs_ops->get_fbs_support(fbs, fbs_support_en);

	return ret;
}

int ufsfbs_fbs_rec_lrs(struct ufsfbs_dev *fbs, u32 *rec_lrs)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_rec_lba_range_size(fbs, rec_lrs);

	return ret;
}

int ufsfbs_fbs_max_lrs(struct ufsfbs_dev *fbs, u32 *max_lrs)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_max_lba_range_size(fbs, max_lrs);

	return ret;
}

int ufsfbs_fbs_min_lrs(struct ufsfbs_dev *fbs, u32 *min_lrs)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_min_lba_range_size(fbs, min_lrs);

	return ret;
}

int ufsfbs_fbs_max_lrc(struct ufsfbs_dev *fbs, int *frag_exe_level)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_max_lba_range_count(fbs, frag_exe_level);

	return ret;
}

int ufsfbs_fbs_lra(struct ufsfbs_dev *fbs, int *frag_exe_level)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_lba_range_alignment(fbs, frag_exe_level);

	return ret;
}
int ufsfbs_get_exe_level(struct ufsfbs_dev *fbs, int *frag_exe_level)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_level_to_exe_get(fbs, frag_exe_level);

	return ret;
}

int ufsfbs_set_exe_level(struct ufsfbs_dev *fbs, int *frag_exe_level)
{

	int ret = 0;

	ret = fbs->fbs_ops->fbs_level_to_exe_set(fbs, frag_exe_level);

	return ret;
}

int ufsfbs_get_level_check_ops(struct ufsfbs_dev *fbs, int *op_status)
{

	int ret;

	ret = fbs->fbs_ops->fbs_level_ops(fbs, op_status);
	if (ret)
		FBS_ERR_MSG("[ufsfbs]Get level check ops failed:%d", ret);
	else
		FBS_INFO_MSG("[ufsfbs]Level check ops:%d", *op_status);

	return ret;
}

int ufsfbs_get_ops(struct ufsfbs_dev *fbs, int *op_status)
{

	int ret;

	ret = fbs->fbs_ops->fbs_defrag_ops(fbs, op_status);
	if (ret)
		FBS_ERR_MSG("[ufsfbs]Get defrag ops failed:%d", ret);
	else
		FBS_INFO_MSG("[ufsfbs]Defrag ops:%d", *op_status);

	return ret;
}

int ufsfbs_read_frag_level(struct ufsfbs_dev *fbs, char *buf, int len)
{
	int ret;
	uint8_t mode = FBS_READ_LBA_RANGE_MODE;
	uint8_t bufid = FBS_READ_LBA_RANGE_BUF_ID;
	int offset = FBS_READ_LBA_RANGE_BUF_OFFSET;
	struct scsi_device *sdev = fbs->hba->sdev_ufs_device;
	FBS_INFO_MSG("[ufsfbs]Lba range count:%d", fbs->fbs_lba_count);
	if(!fbs->fbs_lba_count)
		FBS_ERR_MSG("[ufsfbs]Invaild lba range count:%d",fbs->fbs_lba_count);
	len = fbs->fbs_lba_count;

	pm_runtime_get_sync(fbs->hba->dev);
	ret = fbs->fbs_ops->fbs_read_frag_level(sdev, buf, mode, bufid, offset, len);
	pm_runtime_put_sync(fbs->hba->dev);
	if (ret)
		FBS_ERR_MSG("[ufsfbs]Read frag level fail, ret:%d",ret);

	return ret;
}

int ufsfbs_lba_list_write(struct ufsfbs_dev *fbs, const char *buf, int len)
{
	int ret, buf_len = 0;
	uint8_t mode = FBS_WRITE_LBA_RANGE_MODE;
	uint8_t bufid = FBS_WRITE_LBA_RANGE_BUF_ID;
	int offset = FBS_WRITE_LBA_RANGE_BUF_OFFSET;
	struct scsi_device *sdev = fbs->hba->sdev_ufs_device;
	char *buf_ptr;

	buf_len = strlen(buf);
	fbs->fbs_lba_count = len;
	buf_ptr = kzalloc(buf_len + 1, GFP_KERNEL);
	if (!buf_ptr) {
		FBS_ERR_MSG("[ufsfbs]Alloc buffer fail");
		ret = -ENOMEM;
		return ret;
	}
	memcpy(buf_ptr, buf, buf_len + 1);

	pm_runtime_get_sync(fbs->hba->dev);
	ret = fbs->fbs_ops->fbs_lba_range_list_write(sdev, buf_ptr, mode, bufid, offset, len);
	pm_runtime_put_sync(fbs->hba->dev);
	if (ret) {
		FBS_INFO_MSG("[ufsfbs]Write lba range buffer fail");
		goto out;
	}
out:
	kfree(buf_ptr);
	return ret;
}

void ufsfbs_init(struct ufs_hba *hba)
{
	struct ufsfbs_dev *fbs;
	int ret = 0;
	fbs = &hba->fbs;
	fbs->hba = hba;

	fbs->fbs_trigger = false;
	fbs->is_skhynix = false;

	ret = ufsfbs_init_ops(fbs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get fbs ops fail");
		return;
	} else {
		FBS_INFO_MSG("[ufsfbs]Init ops succeed");
	}

	fbs->fbs_trigger_delay = FBS_TRIGGER_WORKER_DELAY_MS_DEFAULT;

	fbs->fbs_debug = false;
	fbs->block_suspend = false;
	fbs->start_time_stamp = 0;
	fbs->first_phy_error = true;

	/* If HCI supports auto hibern8, UFS Driver use it default */
	if (ufshcd_is_auto_hibern8_supported(fbs->hba))
		fbs->is_auto_enabled = true;
	else
		fbs->is_auto_enabled = false;

	ret = ufsfbs_create_sysfs(fbs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Sysfs init fail,disabled fbs driver");
		kfree(fbs);
		ufsfbs_set_state(hba, FBS_FAILED);
		return;
	}

	FBS_INFO_MSG("[ufsfbs]Create sysfs finished");

	ufsfbs_set_state(hba, FBS_PRESENT);
}

