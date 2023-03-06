/*
 * mi_fbs_sysfs.h
 *
 * Created on: 2021-09-17
 *
 * Authors:
 *	kook <zhangbinbin1@xiaomi.com>
 *	cuijiguang <cuijiguang@xiaomi.com>
 *	lijiaming <lijiaming3@xiaomi.com>
 */
#ifndef DRIVERS_SCSI_UFS_MI_FBS_SYSFS_H_
#define DRIVERS_SCSI_UFS_MI_FBS_SYSFS_H_

int ufsfbs_create_sysfs(struct ufsfbs_dev *fbs);
void ufsfbs_remove_sysfs(struct ufsfbs_dev *fbs);
 
#endif /* _MI_FBS_SYSFS_H_ */
