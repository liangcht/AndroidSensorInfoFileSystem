/* internal.h: sensorfs internal definitions
 *
 * W4118 Homework 6
 */
#include <linux/kernel.h>

extern const struct address_space_operations sensorfs_aops;
extern const struct inode_operations sensorfs_file_inode_operations;
extern const struct file_operations sensorfs_dir_operations;
extern struct file_system_type sensorfs_fs_type;
struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags);
struct sensorfs_inode {
	struct sensorfs_dir_entry *sde;
	struct inode vfs_inode;
};

static struct sensorfs_inode *inode_to_sensorfs_inode(struct inode *inode)
{
	return container_of(inode, struct sensorfs_inode, vfs_inode);
}

static inline struct sensorfs_dir_entry *SDE(struct inode *inode)
{
	struct sensorfs_inode *temp = inode_to_sensorfs_inode(inode);
	return temp->sde;
}


struct sensorfs_dir_entry {
	struct timespec m_time;
	struct timespec a_time;
	struct timespec c_time;
	uid_t uid;
	gid_t gid;
	umode_t mode;
	unsigned int low_ino;
	const char *name;
	unsigned short namelen;
	char contents[8192];
	loff_t size;
	struct sensorfs_dir_entry *parent, *first_child, *next;
};


