#ifndef _LINUX_SENSORFS_H
#define _LINUX_SENSORFS_H

#define SENSORFS_GPS_FILENAME "gps"
#define SENSORFS_LUMI_FILENAME "lumi"
#define SENSORFS_PROX_FILENAME "prox"
#define SENSORFS_LINACCEL_FILENAME "linaccel"

extern struct dentry *sensorfs_mount(struct file_system_type *fs_type,
	 int flags, const char *dev_name, void *data);
extern const struct file_operations sensorfs_file_operations;
int sensorfs_fill_super(struct super_block *sb, void *data, int silent);

#endif /* _LINUX_SENSORFS_H */
