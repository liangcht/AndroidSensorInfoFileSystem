/* file.c: sensorfs file implementation
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sensorfs.h>
#include <linux/slab.h>
#include "internal.h"
extern spinlock_t sensorfs_biglock;

ssize_t sensorfs_read_file(struct file *file, char __user *buf, size_t count,
			   loff_t *ppos)
{
	struct sensorfs_dir_entry *de;
	int to_write;
	char *file_str;
	size_t ret;
	int i;
	spin_lock(&sensorfs_biglock);
	de = SDE(file_inode(file));
	to_write = (int)de->size % 8192;
	file_str = kzalloc(8193, GFP_KERNEL);

	if (file_str == NULL) {
		spin_unlock(&sensorfs_biglock);
		return -ENOMEM;
	}
	if (de->size >= 8192) {
		i = to_write;
		while (i < 8192) {
			BUG_ON(de->contents[i] == 0);
			i++;
		}
		memcpy(file_str,
		       de->contents + to_write,
		       8192 - to_write);
		memcpy(file_str + (8192 - to_write),
		       de->contents,
		       to_write);
	} else {
		memcpy(file_str, de->contents, to_write);

	}

	spin_unlock(&sensorfs_biglock);
	ret = simple_read_from_buffer(buf, count, ppos,
				       file_str,
				       strlen(file_str));

	kfree(file_str);
	return ret;
}

int sensorfs_readdir_de(struct sensorfs_dir_entry *de, struct file *flip,
			void *dirent, filldir_t filldir)
{
	unsigned int ino;
	int i;
	struct inode *inode = file_inode(flip);
	int ret = 0;

	ino = inode->i_ino;
	i = flip->f_pos;

	spin_lock(&sensorfs_biglock);
	de = de->first_child;
	for (;;) {
		if (!de) {
			ret = 1;
			spin_unlock(&sensorfs_biglock);
			goto out;
		}
		if (!i)
			break;
		de = de->next;
		i--;
	}
	do {
		struct sensorfs_dir_entry *next;
		spin_unlock(&sensorfs_biglock);
		if (filldir(dirent, de->name, de->namelen, flip->f_pos,
			    de->low_ino, de->mode >> 12) < 0) {
			goto out;
		}
		spin_lock(&sensorfs_biglock);
		flip->f_pos++;
		next = de->next;
		de = next;
	} while (de);
	spin_unlock(&sensorfs_biglock);
	ret = 1;
out:
	return ret;


}

int sensorfs_readdir(struct file *flip, void *dirent, filldir_t filldir)
{
	struct inode *inode = file_inode(flip);

	return sensorfs_readdir_de(SDE(inode), flip, dirent, filldir);
}


const struct file_operations sensorfs_file_operations = {
	.read = sensorfs_read_file
};

const struct inode_operations sensorfs_file_inode_operations = {
};

const struct file_operations sensorfs_dir_operations = {
	.readdir = sensorfs_readdir,
	.open = dcache_dir_open,
	.read = generic_read_dir,
	.release = dcache_dir_close,
	.llseek = dcache_dir_lseek
};

const struct inode_operations sensorfs_dir_inode_operations = {
	.lookup = sensorfs_lookup
};
