/*
 * mi_fbs_sysfs.c
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

void ufsfbs_remove_sysfs(struct ufsfbs_dev *fbs)
{
	int ret;

	ret = kobject_uevent(&fbs->kobj, KOBJ_REMOVE);
	FBS_INFO_MSG("[ufsfbs]Kobject removed (%d)", ret);
	kobject_del(&fbs->kobj);
}

static ssize_t ufsfbs_sysfs_show_fbs_support(struct ufsfbs_dev *fbs, char *buf)
{
	int ret;
	u8 fbs_support_en;

	ret = ufsfbs_fbs_support(fbs, &fbs_support_en);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get support val failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", fbs_support_en);
}


static ssize_t ufsfbs_sysfs_show_fbs_rec_lrs(struct ufsfbs_dev *fbs, char *buf)
{
	int ret;
	u32 rec_lrs;

	ret = ufsfbs_fbs_rec_lrs(fbs, &rec_lrs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get recommanded LBA range size failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", rec_lrs);
}

static ssize_t ufsfbs_sysfs_show_fbs_max_lrs(struct ufsfbs_dev *fbs, char *buf)
{
	int ret;
	u32 max_lrs;

	ret = ufsfbs_fbs_max_lrs(fbs, &max_lrs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get max LBA range size failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", max_lrs);
}

static ssize_t ufsfbs_sysfs_show_fbs_min_lrs(struct ufsfbs_dev *fbs, char *buf)
{
	int ret;
	u32 min_lrs;

	ret = ufsfbs_fbs_min_lrs(fbs, &min_lrs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get min LBA range size failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", min_lrs);
}

static ssize_t ufsfbs_sysfs_show_fbs_max_lrc(struct ufsfbs_dev *fbs, char *buf)
{
	int ret, max_lrc;

	ret = ufsfbs_fbs_max_lrc(fbs, &max_lrc);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get max LBA range count failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", max_lrc);
}

static ssize_t ufsfbs_sysfs_show_fbs_lra(struct ufsfbs_dev *fbs, char *buf)
{
	int ret, lra;

	ret = ufsfbs_fbs_lra(fbs, &lra);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get LBA range alignment failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", lra);
}

static ssize_t ufsfbs_sysfs_show_fbs_flc_ops(struct ufsfbs_dev *fbs, char *buf)
{
	int ret, level_ops;

	ufsfbs_block_enter_suspend(fbs);

	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 0);
	}
	ret = ufsfbs_get_level_check_ops(fbs, &level_ops);
	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 1);
	}

	ufsfbs_allow_enter_suspend(fbs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get level check opstatus failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", level_ops);
}

static ssize_t ufsfbs_sysfs_show_fbs_defrag_ops(struct ufsfbs_dev *fbs, char *buf)
{
	int ret, defrag_ops;

	ufsfbs_block_enter_suspend(fbs);

	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 0);
	}
	ret = ufsfbs_get_ops(fbs, &defrag_ops);
	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 1);
	}

	ufsfbs_allow_enter_suspend(fbs);

	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get defrag opstatus failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", defrag_ops);
}

static ssize_t ufsfbs_sysfs_show_fbs_get_lr_frag_level(struct ufsfbs_dev *fbs,
						  char *buf)
{
	int i, ret, count = 0;
	int read_len = 1024;
	char *fbs_read_buffer;

	fbs_read_buffer = kzalloc(read_len, GFP_KERNEL);
	if (!fbs_read_buffer) {
		ret = -ENOMEM;
		return ret;
	}
	ufsfbs_block_enter_suspend(fbs);

	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 0);
	}
	FBS_INFO_MSG("[ufsfbs]Get frag level start power mode: %d\n", fbs->hba->curr_dev_pwr_mode);
	ret = ufsfbs_read_frag_level(fbs, fbs_read_buffer, read_len);
	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 1);
	}

	ufsfbs_allow_enter_suspend(fbs);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get lba range level failed");
		goto out;
	}

	for(i = 0; i < (fbs->fbs_lba_count + 1) * 8; i++) {
		count += snprintf(buf + count, PAGE_SIZE - count, "%02x  ", fbs_read_buffer[i]);
		if(!((i + 1 ) % 8))
			count += snprintf(buf + count, PAGE_SIZE - count, "\n");
	}
out:
	kfree(fbs_read_buffer);
	return count;
}

static ssize_t ufsfbs_sysfs_show_fbs_wholefile_enable(struct ufsfbs_dev *fbs,
						  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "[ufsfbs]whole file flag: %d\n", fbs->fbs_wholefile);
}

static ssize_t ufsfbs_sysfs_store_fbs_wholefile_enable(struct ufsfbs_dev *fbs,
						   const char *buf,
						   size_t count)
{
	bool val;

	if (kstrtobool(buf, &val)) {
		FBS_ERR_MSG("[ufsfbs]Convert bool type fail from char * type");
		return -EINVAL;
	}

	fbs->fbs_wholefile = val;

	return count;
}

static ssize_t ufsfbs_sysfs_show_fbs_err_cnt(struct ufsfbs_dev *fbs, char *buf)
{
	FBS_INFO_MSG("[ufsfbs]fbs_err_cnt:%d", fbs->fbs_err_cnt);

	return snprintf(buf, PAGE_SIZE, "%d\n", fbs->fbs_err_cnt);
}

static ssize_t ufsfbs_sysfs_store_fbs_err_cnt(struct ufsfbs_dev *fbs, const char *buf,
					size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0)
		return -EINVAL;

	fbs->fbs_err_cnt += val;

	FBS_INFO_MSG("[ufsfbs]fbs_err_cnt:%d", fbs->fbs_err_cnt);

	return count;
}

static ssize_t ufsfbs_sysfs_show_fbs_retry_cnt(struct ufsfbs_dev *fbs, char *buf)
{
	FBS_INFO_MSG("[ufsfbs]fbs_retry_cnt:%d", fbs->fbs_retry_cnt);

	return snprintf(buf, PAGE_SIZE, "%d\n", fbs->fbs_retry_cnt);
}

static ssize_t ufsfbs_sysfs_store_fbs_retry_cnt(struct ufsfbs_dev *fbs, const char *buf,
					size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0)
		return -EINVAL;

	fbs->fbs_retry_cnt += val;

	FBS_INFO_MSG("[ufsfbs]fbs_retry_cnt:%d", fbs->fbs_retry_cnt);

	return count;
}

static ssize_t ufsfbs_sysfs_show_fbs_exe_level(struct ufsfbs_dev *fbs,
						  char *buf)
{
	int frag_exe_level, ret;

	ret = ufsfbs_get_exe_level(fbs, &frag_exe_level);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get execute level failed");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", frag_exe_level);
}

static ssize_t ufsfbs_sysfs_store_fbs_exe_level(struct ufsfbs_dev *fbs,
						   const char *buf,
						   size_t count)
{
	unsigned long val;
	int ret = 0, defrag_ops;

	if (kstrtoul(buf, 0, &val)) {
		return -EINVAL;
	}

	if (val < 0 || val > 10) {
		FBS_ERR_MSG("[ufsfbs]fbs_exe_level set error, illegal value");
		return -EINVAL;
	}
	ret = ufsfbs_get_ops(fbs, &defrag_ops);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get defrag opstatus failed");
		return -EINVAL;
	}
	if (defrag_ops == FBS_OPS_HOST_NA  || defrag_ops == FBS_OPS_DEVICE_NA) {
		ret = ufsfbs_set_exe_level(fbs, (int *)&val);
		if (ret) {
			FBS_ERR_MSG("[ufsfbs]Get execute level failed");
			return -EINVAL;
		}
		FBS_INFO_MSG("[ufsfbs]fbs_set_exe_level:%d", val);
	} else {
		FBS_ERR_MSG("[ufsfbs]fbs_exe_level set error, illegal defrag ops value");
		return -EINVAL;
	}

	return count;
}

static ssize_t ufsfbs_sysfs_show_debug(struct ufsfbs_dev *fbs, char *buf)
{
	FBS_INFO_MSG("[ufsfbs]Debug:%d", fbs->fbs_debug);

	return snprintf(buf, PAGE_SIZE, "%d\n", fbs->fbs_debug);
}

static ssize_t ufsfbs_sysfs_store_debug(struct ufsfbs_dev *fbs, const char *buf,
					size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val != 0 && val != 1)
		return -EINVAL;

	fbs->fbs_debug = val ? true : false;

	FBS_INFO_MSG("[ufsfbs]Debug:%d", fbs->fbs_debug);

	return count;
}

static ssize_t ufsfbs_sysfs_show_block_suspend(struct ufsfbs_dev *fbs,
					       char *buf)
{
	FBS_INFO_MSG("[ufsfbs]Block suspend:%d", fbs->block_suspend);

	return snprintf(buf, PAGE_SIZE, "%d\n", fbs->block_suspend);
}

static ssize_t ufsfbs_sysfs_store_block_suspend(struct ufsfbs_dev *fbs,
						const char *buf, size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val != 0 && val != 1)
		return -EINVAL;

	FBS_INFO_MSG("[ufsfbs]fbs_block_suspend %lu", val);

	if (val == fbs->block_suspend)
		return count;

	if (val)
		ufsfbs_block_enter_suspend(fbs);
	else
		ufsfbs_allow_enter_suspend(fbs);

	fbs->block_suspend = val ? true : false;

	return count;
}

static ssize_t ufsfbs_sysfs_show_auto_hibern8_enable(struct ufsfbs_dev *fbs,
						     char *buf)
{
	FBS_INFO_MSG("[ufsfbs]HCI auto hibern8 %d", fbs->is_auto_enabled);

	return snprintf(buf, PAGE_SIZE, "%d\n", fbs->is_auto_enabled);
}

static ssize_t ufsfbs_sysfs_store_auto_hibern8_enable(struct ufsfbs_dev *fbs,
						      const char *buf,
						      size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val != 0 && val != 1)
		return -EINVAL;

	ufsfbs_auto_hibern8_enable(fbs, val);

	return count;
}

static ssize_t ufsfbs_sysfs_store_fbs_flc_enable(struct ufsfbs_dev *fbs,
						      const char *buf,
						      size_t count)
{
	bool val;
	int ret;

	if (kstrtobool(buf, &val))
		return -EINVAL;

	ufsfbs_block_enter_suspend(fbs);

	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 0);
	}
	FBS_INFO_MSG("[ufsfbs]flc_enable start power mode: %d\n", fbs->hba->curr_dev_pwr_mode);
	ret = ufsfbs_frag_level_check_enable(fbs, val);
	if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
		ufsfbs_auto_hibern8_enable(fbs, 1);
	}
	ufsfbs_allow_enter_suspend(fbs);

	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Enable frag level check failed");
		return -EINVAL;
	}

	return count;
}

static ssize_t ufsfbs_sysfs_store_fbs_defrag_enable(struct ufsfbs_dev *fbs,
						      const char *buf,
						      size_t count)
{
	bool val;
	int ret = 0, defrag_ops = 0;

	if (kstrtobool(buf, &val)) {
		return -EINVAL;
	}

	ret = ufsfbs_get_ops(fbs, &defrag_ops);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get defrag ops failed");
		return -EINVAL;
	}
	if (fbs->is_skhynix || (defrag_ops == FBS_OPS_PROGRESSING && !val) || val) {
		ufsfbs_block_enter_suspend(fbs);

		if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
			ufsfbs_auto_hibern8_enable(fbs, 0);
		}
		FBS_INFO_MSG("[ufsfbs]Defrag_enable start power mode:%d", fbs->hba->curr_dev_pwr_mode);
		ret = ufsfbs_defrag_enable(fbs, val);
		if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
			ufsfbs_auto_hibern8_enable(fbs, 1);
		}
		ufsfbs_allow_enter_suspend(fbs);

		if (ret) {
			FBS_ERR_MSG("[ufsfbs]Enable level check failed");
			return -EINVAL;
		}
	} else {
		FBS_ERR_MSG("[ufsfbs]Invalid degrag_ops(%d), or enable value(%d) \n", defrag_ops, val);
	}

	return  count;
}

static ssize_t ufsfbs_sysfs_store_fbs_send_lr_list(struct ufsfbs_dev *fbs,
						      const char *buf,
						      size_t count)
{
	int ret = 0, len = 0,  level_ops = 0, defrag_ops = 0;
	char *arg;

	if(!buf) {
		FBS_ERR_MSG("[ufsfbs]Invalid fbs write buf input, Please try again");
		return -EINVAL;
	}
	arg = strstr(buf, ",");
	if(arg == NULL || buf[strlen(buf) - 1] == ',') {
		FBS_ERR_MSG("[ufsfbs]Invalid lba range, please input lba range separated by ','");
		return -EINVAL;
	}
	while (arg != NULL) {
		len++;
		arg +=1;
		arg = strstr(arg, ",");
	}
	if (len%2) {
		len++;
		FBS_INFO_MSG("[ufsfbs]Valid lba range count");
	} else {
		FBS_ERR_MSG("[ufsfbs]Invalid lba range count, please input right lba range count");
		return -EINVAL;
	}

	ret = ufsfbs_get_level_check_ops(fbs, &level_ops);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get level_ops fail");
		return -EINVAL;
	}

	ret = ufsfbs_get_ops(fbs, &defrag_ops);
	if (ret) {
		FBS_ERR_MSG("[ufsfbs]Get defrag_ops fail");
		return -EINVAL;
	}
	if(level_ops == FBS_OPS_SUCCESS){
		FBS_INFO_MSG("[ufsfbs]level_ops:%d, abnormal, clear flag again", level_ops);
		ufsfbs_frag_level_check_enable(fbs, 0);
	}
	if(level_ops == FBS_OPS_HOST_NA && defrag_ops == FBS_OPS_HOST_NA) {
		ufsfbs_block_enter_suspend(fbs);

		if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
			ufsfbs_auto_hibern8_enable(fbs, 0);
		}
		FBS_INFO_MSG("[ufsfbs]Send fbs lba range begin, WRITE BUFF power mode:%d", fbs->hba->curr_dev_pwr_mode);
		ret = ufsfbs_lba_list_write(fbs, buf, len/2);
		if (fbs->fbs_vendor_ops->fbs_ah8_disable_enable) {
			ufsfbs_auto_hibern8_enable(fbs, 1);
		}

		ufsfbs_allow_enter_suspend(fbs);
		if (ret) {
			FBS_ERR_MSG("[ufsfbs]Send lba range failed");
			return -EINVAL;
		}
	} else {
		FBS_ERR_MSG("[ufsfbs]Invalid defrag or level check ops");
	}

	return count;
}

/* SYSFS DEFINE */
#define define_sysfs_ro(_name) __ATTR(_name, 0444,			\
				      ufsfbs_sysfs_show_##_name, NULL)
#define define_sysfs_wo(_name) __ATTR(_name, 0200,			\
				       NULL, ufsfbs_sysfs_store_##_name)
#define define_sysfs_rw(_name) __ATTR(_name, 0644,			\
				      ufsfbs_sysfs_show_##_name,	\
				      ufsfbs_sysfs_store_##_name)

static struct ufsfbs_sysfs_entry ufsfbs_sysfs_entries[] = {
	define_sysfs_ro(fbs_rec_lrs),
	define_sysfs_ro(fbs_max_lrs),
	define_sysfs_ro(fbs_min_lrs),
	define_sysfs_ro(fbs_max_lrc),
	define_sysfs_ro(fbs_lra),
	define_sysfs_ro(fbs_flc_ops),
	define_sysfs_ro(fbs_defrag_ops),
	define_sysfs_ro(fbs_get_lr_frag_level),
	define_sysfs_ro(fbs_support),

	define_sysfs_wo(fbs_flc_enable),
	define_sysfs_wo(fbs_defrag_enable),
	define_sysfs_wo(fbs_send_lr_list),

	define_sysfs_rw(fbs_exe_level),
	define_sysfs_rw(fbs_wholefile_enable),
	define_sysfs_rw(fbs_err_cnt),
	define_sysfs_rw(fbs_retry_cnt),
	/* debug */
	define_sysfs_rw(debug),
	/* Attribute (RAW) */
	define_sysfs_rw(block_suspend),
	define_sysfs_rw(auto_hibern8_enable),
	__ATTR_NULL
};

static ssize_t ufsfbs_attr_show(struct kobject *kobj, struct attribute *attr,
				char *page)
{
	struct ufsfbs_sysfs_entry *entry;
	struct ufsfbs_dev *fbs;
	ssize_t error;

	entry = container_of(attr, struct ufsfbs_sysfs_entry, attr);
	if (!entry->show)
		return -EIO;

	fbs = container_of(kobj, struct ufsfbs_dev, kobj);
	if (ufsfbs_is_not_present(fbs))
		return -ENODEV;

	mutex_lock(&fbs->sysfs_lock);
	error = entry->show(fbs, page);
	mutex_unlock(&fbs->sysfs_lock);

	return error;
}

static ssize_t ufsfbs_attr_store(struct kobject *kobj, struct attribute *attr,
				 const char *page, size_t length)
{
	struct ufsfbs_sysfs_entry *entry;
	struct ufsfbs_dev *fbs;
	ssize_t error;

	entry = container_of(attr, struct ufsfbs_sysfs_entry, attr);
	if (!entry->store)
		return -EIO;

	fbs = container_of(kobj, struct ufsfbs_dev, kobj);
	if (ufsfbs_is_not_present(fbs))
		return -ENODEV;

	mutex_lock(&fbs->sysfs_lock);
	error = entry->store(fbs, page, length);
	mutex_unlock(&fbs->sysfs_lock);

	return error;
}

static const struct sysfs_ops ufsfbs_sysfs_ops = {
	.show = ufsfbs_attr_show,
	.store = ufsfbs_attr_store,
};

static struct kobj_type ufsfbs_ktype = {
	.sysfs_ops = &ufsfbs_sysfs_ops,
	.release = NULL,
};

 int ufsfbs_create_sysfs(struct ufsfbs_dev *fbs)
{
	struct device *dev = fbs->hba->dev;
	struct ufsfbs_sysfs_entry *entry;
	int err;

	fbs->sysfs_entries = ufsfbs_sysfs_entries;

	kobject_init(&fbs->kobj, &ufsfbs_ktype);
	mutex_init(&fbs->sysfs_lock);

	FBS_INFO_MSG("[ufsfbs]Creates sysfs %p dev->kobj %p",
		 &fbs->kobj, &dev->kobj);

	err = kobject_add(&fbs->kobj, kobject_get(&dev->kobj), "ufsfbs");
	if (!err) {
		for (entry = fbs->sysfs_entries; entry->attr.name != NULL;
		     entry++) {
			FBS_INFO_MSG("[ufsfbs]Sysfs attr creates: %s",
				 entry->attr.name);
			err = sysfs_create_file(&fbs->kobj, &entry->attr);
			if (err) {
				FBS_ERR_MSG("[ufsfbs]Create entry(%s) failed",
					entry->attr.name);
				goto kobj_del;
			}
		}
		kobject_uevent(&fbs->kobj, KOBJ_ADD);
	} else {
		FBS_ERR_MSG("[ufsfbs]Kobject_add failed");
	}

	return err;
kobj_del:
	err = kobject_uevent(&fbs->kobj, KOBJ_REMOVE);
	FBS_INFO_MSG("Kobject removed (%d)", err);
	kobject_del(&fbs->kobj);
	return -EINVAL;
}
