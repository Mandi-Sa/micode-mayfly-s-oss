/*
 * mi_fbs.h
 *
 * Created on: 2021-09-17
 *
 * Authors:
 *	kook <zhangbinbin1@xiaomi.com>
 *	cuijiguang <cuijiguang@xiaomi.com>
 *	lijiaming <lijiaming3@xiaomi.com>
 */

#ifndef DRIVERS_SCSI_UFS_MI_FBS_H_
#define DRIVERS_SCSI_UFS_MI_FBS_H_
#define UFSFBS_DEBUG	0
#if UFSFBS_DEBUG
#define FBS_INFO_MSG(msg, args...)		do {pr_info("%s:%d info: " msg "\n", __func__, __LINE__, ##args);} while(0)
#else
#define FBS_INFO_MSG(msg, args...)		do { } while(0)
#endif

#define FBS_ERR_MSG(msg, args...)		pr_err("%s:%d err: " msg "\n", \
					       __func__, __LINE__, ##args)
#define FBS_WARN_MSG(msg, args...)		pr_warn("%s:%d warn: " msg "\n", \
					       __func__, __LINE__, ##args)


#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/blktrace_api.h>
#include <linux/blkdev.h>
#include <linux/bitfield.h>
#include <linux/ktime.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <linux/types.h>
#include <asm/unaligned.h>

#include "../../../../../block/blk.h"

#define FBS_TRIGGER_WORKER_DELAY_MS_DEFAULT	2000
#define FBS_TRIGGER_WORKER_DELAY_MS_MIN		100
#define FBS_TRIGGER_WORKER_DELAY_MS_MAX		10000

#define FBS_AUTO_HIBERN8_DISABLE  (FIELD_PREP(UFSHCI_AHIBERN8_TIMER_MASK, 0) | \
				   FIELD_PREP(UFSHCI_AHIBERN8_SCALE_MASK, 3))
#define LUNID 0x0
#define FBS_LBA_RANGE_LENGTH (4*1024)
#define QUERY_FBS_SUPPORT_MASK 0x100
#define BUFFER_SZ 128

enum UFSFBS_STATE {
	FBS_NEED_INIT = 0,
	FBS_PRESENT = 1,
	FBS_FAILED = -2,
	FBS_RESET = -3,
};

enum {
	FBS_OP_DISABLE	  = 0,
	FBS_OP_ANALYZE	 = 1,
	FBS_OP_EXECUTE	= 2,
	FBS_OP_MAX
};

enum {
	FBS_NOT_REQUIRED	= 0,
	FBS_REQUIRED	        = 1,
};

enum FBS_OPS {
	FBS_OPS_HOST_NA      = 0x0,
	FBS_OPS_DEVICE_NA    = 0x1,
	FBS_OPS_PROGRESSING  = 0x2,
	FBS_OPS_SUCCESS      = 0x3,
	FBS_OPS_PRE_MATURELY = 0x4,
	FBS_OPS_HOST_LBA_NA  = 0x5,
	FBS_OPS_INTERNAL_ERR = 0xff,
};

enum {
	FBS_AH8_DISABLE = 0,
	FBS_AH8_ENABLE  = 1,
};

struct fbs_vendor_ops {
	uint8_t vendor_id[8];
	uint8_t fbs_ah8_disable_enable;
	struct ufsfbs_ops *fbs_ops;
};

struct ufsfbs_dev {
	struct ufs_hba *hba;
	unsigned int fbs_trigger;   /* default value is false */
	struct delayed_work fbs_trigger_work;
	unsigned int fbs_trigger_delay;

	bool is_auto_enabled;
	ktime_t start_time_stamp;

	atomic_t fbs_state;
	/* for sysfs */
	struct kobject kobj;
	struct mutex sysfs_lock;
	struct ufsfbs_sysfs_entry *sysfs_entries;

	struct ufsfbs_ops *fbs_ops;
	struct fbs_vendor_ops *fbs_vendor_ops;
	bool is_skhynix;
	/* for debug */
	bool fbs_debug;
	bool fbs_wholefile;
	bool first_phy_error;

	int fbs_lba_count;
	unsigned long fbs_err_cnt;
	unsigned long fbs_retry_cnt;
	bool block_suspend;

};

struct ufsfbs_ops {
	int (*get_fbs_support)(struct ufsfbs_dev *fbs, u8 *fbs_support_en);
	int (*fbs_rec_lba_range_size)(struct ufsfbs_dev *fbs, u32 *rec_lrs);
	int (*fbs_max_lba_range_size)(struct ufsfbs_dev *fbs, u32 *max_lrs);
	int (*fbs_min_lba_range_size)(struct ufsfbs_dev *fbs, u32 *min_lrs);
	int (*fbs_max_lba_range_count)(struct ufsfbs_dev *fbs, int *lrc);
	int (*fbs_lba_range_alignment)(struct ufsfbs_dev *fbs, int *lra);
	int (*fbs_level_to_exe_set)(struct ufsfbs_dev *fbs, int *level_exe_set);
	int (*fbs_level_to_exe_get)(struct ufsfbs_dev *fbs, int *level_exe_get);
	int (*fbs_level_ops)(struct ufsfbs_dev *fbs, int *level_ops);
	int (*fbs_defrag_ops)(struct ufsfbs_dev *fbs, int *defrag_ops);
	int (*fbs_frag_level_check_enable)(struct ufsfbs_dev *fbs, bool level_check_enable);
	int (*fbs_defrag_enable)(struct ufsfbs_dev *fbs, bool defrag_enable);
	int (*fbs_lba_range_list_write)(struct scsi_device *sdev, void *buf, uint8_t mode, uint8_t buf_id, int offset, int len);
	int (*fbs_read_frag_level)(struct scsi_device *sdev, void *buf, uint8_t mode, uint8_t buf_id, int offset, int len);
};

struct ufsfbs_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct ufsfbs_dev *fbs, char *buf);
	ssize_t (*store)(struct ufsfbs_dev *fbs, const char *buf, size_t count);
};
int ufsfbs_fbs_rec_lrs(struct ufsfbs_dev *fbs, u32 *rec_lrs);
int ufsfbs_fbs_max_lrs(struct ufsfbs_dev *fbs, u32 *frag_exe_level);
int ufsfbs_fbs_min_lrs(struct ufsfbs_dev *fbs, u32 *frag_exe_level);
int ufsfbs_fbs_max_lrc(struct ufsfbs_dev *fbs, int *frag_exe_level);
int ufsfbs_fbs_lra(struct ufsfbs_dev *fbs, int *frag_exe_level);
int ufsfbs_get_level_check_ops(struct ufsfbs_dev *fbs, int *op_status);
int ufsfbs_get_ops(struct ufsfbs_dev *fbs, int *op_status);
int ufsfbs_read_frag_level(struct ufsfbs_dev *fbs, char *buf, int len);
int ufsfbs_get_exe_level(struct ufsfbs_dev *fbs, int *frag_exe_level);
int ufsfbs_set_exe_level(struct ufsfbs_dev *fbs, int *frag_exe_level);
int ufsfbs_lba_list_write(struct ufsfbs_dev *fbs, const char *buf, int len);
int ufsfbs_frag_level_check_enable(struct ufsfbs_dev *fbs, bool val);
int ufsfbs_defrag_enable(struct ufsfbs_dev *fbs, bool val);
int ufsfbs_get_state(struct ufs_hba *hba);
void ufsfbs_set_state(struct ufs_hba *hba, int state);
void ufsfbs_init(struct ufs_hba *hba);
void ufsfbs_reset(struct ufs_hba *hba);
void ufsfbs_reset_host(struct ufs_hba *hba);
void ufsfbs_remove(struct ufs_hba *hba);
void ufsfbs_on_idle(struct ufs_hba *hba);
void ufsfbs_trigger_on(struct ufsfbs_dev *fbs);
void ufsfbs_trigger_off(struct ufsfbs_dev *fbs);
int ufsfbs_is_not_present(struct ufsfbs_dev *fbs);
void ufsfbs_block_enter_suspend(struct ufsfbs_dev *fbs);
void ufsfbs_allow_enter_suspend(struct ufsfbs_dev *fbs);
void ufsfbs_auto_hibern8_enable(struct ufsfbs_dev *fbs, unsigned int val);
int ufsfbs_fbs_support(struct ufsfbs_dev *fbs, u8 *fbs_support_en);
bool ufsfbs_fbs_check_phy_error(struct ufsfbs_dev *fbs);

#endif /* _MI_FBS_H_ */
