/* internal.h: sensorfs internal definitions
 *
 * W4118 Homework 6
 */

extern const struct address_space_operations sensorfs_aops;
extern const struct inode_operations sensorfs_file_inode_operations;
extern const struct file_operations sensorfs_dir_operations;
extern struct file_system_type sensorfs_fs_type;

struct sensorfs_dir_entry {
	//TODO: You will need to add things here
	umode_t mode;
	unsigned int low_ino;
	const char *name;
	unsigned short namelen;
	char contents[8192]; //Use as a circular buffer
	loff_t size;
	struct sensorfs_dir_entry *parent, *first_child, *next;
};

struct sensorfs_inode {
	struct sensorfs_dir_entry *sde;
	struct inode vfs_inode;
};
