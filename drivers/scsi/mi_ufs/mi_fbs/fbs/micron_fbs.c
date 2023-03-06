/*
 * micron_fbs.c
 *
 * Created on: 2021-09-17
 *
 * Authors:
 *	kook <zhangbinbin1@xiaomi.com>
 *	cuijiguang <cuijiguang@xiaomi.com>
 *	lijiaming <lijiaming3@xiaomi.com>
 */

#include "mi_fbs.h"
#include "../../mi-ufshcd.h"
#include "../../mi_ufs_common_ops.h"

#define QUERY_ATTR_IDN_FBS_REC_LRS 0x33
#define QUERY_ATTR_IDN_FBS_MAX_LRS 0X34
#define QUERY_ATTR_IDN_FBS_MIN_LRS 0X35
#define QUERY_ATTR_IDN_FBS_MAX_LRC 0X36
#define QUERY_ATTR_IDN_FBS_LRA 0X37
#define QUERY_ATTR_IDN_FBS_LEVEL_EXE 0X38
#define QUERY_ATTR_IDN_FBS_FLC_OPS 0X39
#define QUERY_ATTR_IDN_FBS_DEFRAG_OPS 0X3A
#define QUERY_ATTR_IDN_FBS_FLC_ENABLE 0X14
#define QUERY_ATTR_IDN_FBS_DEFRAG_ENABLE 0x15
#define FBS_MICRON_LRANGE_HEADER_SIZE 8
#define FBS_MICRON_LRANGE_ENTRY_SIZE 8

int micron_get_fbs_support(struct ufsfbs_dev *fbs, u8 *fbs_support_en)
{
	struct ufs_hba *hba = fbs->hba;
	struct ufs_dev_info *dev_info = &hba->dev_info;

	if (dev_info->wspecversion < 0x0310)
		return -ENOTSUPP;

	*fbs_support_en = (dev_info->d_ext_ufs_feature_sup & QUERY_FBS_SUPPORT_MASK) >> 8;
	FBS_INFO_MSG("[ufsfbs]fbs_support = %d\n", *fbs_support_en);

	return 0;
}

int micron_fbs_rec_lba_range_size(struct ufsfbs_dev *fbs, u32 *rec_lrs)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_REC_LRS, 0, 0, rec_lrs);
}

int micron_fbs_max_lba_range_size(struct ufsfbs_dev *fbs, u32 *max_lrs)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_MAX_LRS, 0, 0, max_lrs);
}

int micron_fbs_min_lba_range_size(struct ufsfbs_dev *fbs, u32 *min_lrs)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_MIN_LRS, 0, 0, min_lrs);
}

int micron_fbs_max_lba_range_count(struct ufsfbs_dev *fbs, int *lrc)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_MAX_LRC, 0, 0, lrc);
}

int micron_fbs_lba_range_alignment(struct ufsfbs_dev *fbs, int *lra)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_LRA, 0, 0, lra);
}

int micron_fbs_level_to_exe_set(struct ufsfbs_dev *fbs, int *level_exe_set)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_WRITE_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_LEVEL_EXE, 0, 0, level_exe_set);
}

int micron_fbs_level_to_exe_get(struct ufsfbs_dev *fbs, int *level_exe_get)
{
	struct ufs_hba *hba = fbs->hba;

	return mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_LEVEL_EXE, 0, 0, level_exe_get);
}

int micron_fbs_level_ops(struct ufsfbs_dev *fbs, int *level_ops)
{
	struct ufs_hba *hba = fbs->hba;
	int ret = 0, attr = -1;

	ret = mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_FLC_OPS, 0, 0, &attr);
	if (ret)
		return ret;
	if (attr == 0x0) {
		*level_ops = FBS_OPS_HOST_NA;
	} else if (attr == 0x1) {
		*level_ops = FBS_OPS_DEVICE_NA;
	} else if (attr == 0x2) {
		*level_ops = FBS_OPS_PROGRESSING;
	} else if (attr == 0x3) {
		*level_ops = FBS_OPS_SUCCESS;
	} else if (attr == 0x4) {
		*level_ops = FBS_OPS_PRE_MATURELY;
	} else if (attr == 0x5) {
		*level_ops = FBS_OPS_HOST_LBA_NA;
	} else if (attr == 0xff) {
		*level_ops = FBS_OPS_INTERNAL_ERR;
		dump_stack();
	} else {
		FBS_INFO_MSG("[ufsfbs]Micron unknown level ops %d\n", attr);
		ret = -1;
		return ret;
	}
	return 0;
}

int micron_defrag_ops(struct ufsfbs_dev *fbs, int *defrag_ops)
{
	struct ufs_hba *hba = fbs->hba;
	int ret = 0, attr = -1;

	ret = mi_ufs_query_attr(hba, UPIU_QUERY_OPCODE_READ_ATTR, (enum attr_idn)QUERY_ATTR_IDN_FBS_DEFRAG_OPS, 0, 0, &attr);
	if (ret)
		return ret;
	if (attr == 0x0) {
		*defrag_ops = FBS_OPS_HOST_NA;
	} else if (attr == 0x1) {
		*defrag_ops = FBS_OPS_DEVICE_NA;
	} else if (attr == 0x2) {
		*defrag_ops = FBS_OPS_PROGRESSING;
	} else if (attr == 0x3) {
		*defrag_ops = FBS_OPS_SUCCESS;
	} else if (attr == 0x4) {
		*defrag_ops = FBS_OPS_PRE_MATURELY;
	} else if (attr == 0x5) {
		*defrag_ops = FBS_OPS_HOST_LBA_NA;
	} else if (attr == 0xff) {
		*defrag_ops = FBS_OPS_INTERNAL_ERR;
		dump_stack();
	} else {
		FBS_INFO_MSG("[ufsfbs]Micron unknown defrag ops %d\n", attr);
		ret = -1;
		return ret;
	}
	return 0;
}

int micron_fbs_frag_level_check_enable(struct ufsfbs_dev *fbs, bool level_check_enable)
{
	struct ufs_hba *hba = fbs->hba;

	if(level_check_enable)
		return mi_ufs_query_flag(hba, UPIU_QUERY_OPCODE_SET_FLAG, (enum flag_idn)QUERY_ATTR_IDN_FBS_FLC_ENABLE, 0, 0, &level_check_enable);
	else
		return mi_ufs_query_flag(hba, UPIU_QUERY_OPCODE_CLEAR_FLAG, (enum flag_idn)QUERY_ATTR_IDN_FBS_FLC_ENABLE, 0, 0, NULL);
}

int micron_fbs_defrag_enable(struct ufsfbs_dev *fbs, bool defrag_enable)
{
	struct ufs_hba *hba = fbs->hba;

	if(defrag_enable)
		return mi_ufs_query_flag(hba, UPIU_QUERY_OPCODE_SET_FLAG, (enum flag_idn)QUERY_ATTR_IDN_FBS_DEFRAG_ENABLE, 0, 0, &defrag_enable);
	else
		return mi_ufs_query_flag(hba, UPIU_QUERY_OPCODE_CLEAR_FLAG, (enum flag_idn)QUERY_ATTR_IDN_FBS_DEFRAG_ENABLE, 0, 0, NULL);
}

int micron_fbs_lba_range_list_write(struct scsi_device *sdev, void *buf, uint8_t mode, uint8_t buf_id, int offset, int len)
{
	struct ufs_hba *hba = shost_priv(sdev->host);
	unsigned char cdb[10] = {0};
	struct scsi_sense_hdr sshdr = {};
	unsigned long flags = 0;
	int ret = 0, len_index = 1, lba_index = FBS_MICRON_LRANGE_HEADER_SIZE;
	u64 lba_value_pre, lba_value_post;
	char lba_buf[FBS_LBA_RANGE_LENGTH] = {0};
	char *lba, *raw_buf = buf;

	/*create lba range buf send for device*/
	lba_buf[0] = LUNID;  // LUNID
	lba_buf[1] = len; //lba range count
	lba_buf[2] = hba->fbs.fbs_wholefile; //calculate whole file

	while((lba = strsep(&raw_buf, ",")) != NULL) {
		ret = kstrtou64(lba, 16, &lba_value_pre);
		if (ret) {
			FBS_ERR_MSG("[ufsfbs]Invalid lba range value\n");
			ret = -ENODEV;
			return ret;
		}
		if (len_index % 2) {
			lba_value_post = lba_value_pre;
			lba_buf[lba_index] = (lba_value_pre >> 24) & 0xff;
			lba_buf[lba_index+1] = (lba_value_pre >> 16) & 0xff;
			lba_buf[lba_index+2] = (lba_value_pre >> 8) & 0xff;
			lba_buf[lba_index+3] = (lba_value_pre) & 0xff;
		} else {
			if (lba_value_pre < lba_value_post) {
				ret = -ENODEV;
				FBS_ERR_MSG("[ufsfbs]Invalid lba range length\n");
				return ret;
			}

			lba_value_pre = lba_value_pre - lba_value_post + 1;

			lba_buf[lba_index+4] = (lba_value_pre >> 16) & 0xff;
			lba_buf[lba_index+5] = (lba_value_pre >> 8) & 0xff;
			lba_buf[lba_index+6] = (lba_value_pre) & 0xff;
			lba_index += FBS_MICRON_LRANGE_ENTRY_SIZE;
		}
		len_index++;
	}
	/*create lba range buf send for device*/
	spin_lock_irqsave(hba->host->host_lock, flags);
	ret = scsi_device_get(sdev);
	if (!ret && !scsi_device_online(sdev)) {
		ret = -ENODEV;
		scsi_device_put(sdev);
	}
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	if (ret){
		FBS_ERR_MSG("[ufsfbs]Get device fail\n");
		return ret;
	}

	hba->host->eh_noresume = 1;

	cdb[0] = WRITE_BUFFER;
	cdb[1] = mode;
	cdb[2] = buf_id;
	cdb[3] = (offset >> 16) & 0xff;
	cdb[4] = (offset >> 8) & 0xff;
	cdb[5] = (offset) & 0xff;
	cdb[6] = (((len+1)*8) >> 16) & 0xff;
	cdb[7] = (((len+1)*8) >> 8) & 0xff;
	cdb[8] = ((len+1)*8) & 0xff;
	cdb[9] = 0;

	ret = scsi_execute(sdev, cdb, DMA_TO_DEVICE, lba_buf, (FBS_MICRON_LRANGE_HEADER_SIZE + len*FBS_MICRON_LRANGE_ENTRY_SIZE), NULL, &sshdr, msecs_to_jiffies(15000), 0, 0, RQF_PM, NULL);


	if (ret)
		FBS_ERR_MSG("[ufsfbs]Write Buffer failed %d\n", ret);

	/*check sense key*/

	FBS_INFO_MSG("[ufsfbs]Sense key:0x%x; asc:0x%x; ascq:0x%x\n", (int)sshdr.sense_key, (int)sshdr.asc, (int)sshdr.ascq);

	scsi_device_put(sdev);

	hba->host->eh_noresume = 0;

	return ret;
}

int micron_fbs_read_frag_level(struct scsi_device *sdev, void *buf, uint8_t mode, uint8_t buf_id, int offset, int len)
{
	uint8_t cdb[16];
	int ret = 0;
	struct ufs_hba *hba = NULL;
	struct scsi_sense_hdr sshdr = {};
	unsigned long flags = 0;

	hba = shost_priv(sdev->host);

	spin_lock_irqsave(hba->host->host_lock, flags);
	ret = scsi_device_get(sdev);
	if (!ret && !scsi_device_online(sdev)) {
		ret = -ENODEV;
		scsi_device_put(sdev);
	}
	spin_unlock_irqrestore(hba->host->host_lock, flags);

	if (ret){
		FBS_ERR_MSG("[ufsfbs]get device fail\n");
		return ret;
	}

	hba->host->eh_noresume = 1;

	// Fill in the CDB with SCSI command structure
	cdb[0] = READ_BUFFER;
	cdb[1] = mode;
	cdb[2] = buf_id;
	cdb[3] = (offset >> 16) & 0xff;
	cdb[4] = (offset >> 8) & 0xff;
	cdb[5] = (offset) & 0xff;
	cdb[6] = (((len+1)*8) >> 16) & 0xff;
	cdb[7] = (((len+1)*8) >> 8) & 0xff;
	cdb[8] = ((len+1)*8) & 0xff;
	cdb[9] = 0;	// Control

	ret = scsi_execute(sdev, cdb, DMA_FROM_DEVICE, buf, (FBS_MICRON_LRANGE_HEADER_SIZE + len*FBS_MICRON_LRANGE_ENTRY_SIZE), NULL, &sshdr, msecs_to_jiffies(15000), 0, 0, RQF_PM, NULL);
	if (ret)
		FBS_ERR_MSG("[ufsfbs]lba range frag level read error %d\n", ret);

	FBS_INFO_MSG("[ufsfbs]sense key:0x%x; asc:0x%x; ascq:0x%x\n", (int)sshdr.sense_key, (int)sshdr.asc, (int)sshdr.ascq);
	scsi_device_put(sdev);
	hba->host->eh_noresume = 0;
	return ret;
}

struct ufsfbs_ops micron_fbs_ops = {
		.get_fbs_support = micron_get_fbs_support,
		.fbs_rec_lba_range_size = micron_fbs_rec_lba_range_size,
		.fbs_max_lba_range_size = micron_fbs_max_lba_range_size,
		.fbs_min_lba_range_size = micron_fbs_min_lba_range_size,
		.fbs_max_lba_range_count = micron_fbs_max_lba_range_count,
		.fbs_lba_range_alignment = micron_fbs_lba_range_alignment,
		.fbs_level_to_exe_set = micron_fbs_level_to_exe_set,
		.fbs_level_to_exe_get = micron_fbs_level_to_exe_get,
		.fbs_level_ops = micron_fbs_level_ops,
		.fbs_defrag_ops = micron_defrag_ops,
		.fbs_frag_level_check_enable = micron_fbs_frag_level_check_enable,
		.fbs_defrag_enable = micron_fbs_defrag_enable,
		.fbs_lba_range_list_write = micron_fbs_lba_range_list_write,
		.fbs_read_frag_level = micron_fbs_read_frag_level
};
