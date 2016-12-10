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
	//TODO: investigate why FILE_STR len prints out 3 times when cat once!!
	spin_lock(&sensorfs_biglock);
	struct sensorfs_dir_entry *de = SDE(file_inode(file));
	int to_write = (int)de->size % 8192;
	char *file_str = kzalloc(8193, GFP_KERNEL);
	size_t ret;
	if (file_str == NULL){
		spin_unlock(&sensorfs_biglock);
		return -ENOMEM;
	}
	int i;
debug_info:
	if (de->size >= 8192) {
		i = to_write;
		while (i < 8192) {
			if (unlikely(de->contents[i] == 0))
				printk("DEBUG: %d th char is 0\n", i);
			i++;
		}
		memcpy(file_str, 
		       de->contents + to_write, 
		       8192 - to_write);
		memcpy(file_str + (8192 - to_write), 
		       de->contents,
		       to_write);
		printk("FILE_STR len %d\n", strlen(file_str));
	
	}
	else {
		memcpy(file_str, de->contents, to_write);

	}

	spin_unlock(&sensorfs_biglock);
	ret = simple_read_from_buffer(buf, count, ppos, 
				       file_str, 
				       strlen(file_str));

	kfree(file_str);
	return ret;
}
//TODO: Probably need a few function implemented here
//and to fill out the following structs appropriately.
int sensorfs_readdir_de(struct sensorfs_dir_entry *de, struct file *flip, 
			void *dirent, filldir_t filldir)
{
	unsigned int ino;
	int i;
	struct inode *inode = file_inode(flip);
	int ret = 0;

	ino = inode->i_ino;
	i = flip->f_pos;

	//TODO: need to design locking machanism
	spin_lock(&sensorfs_biglock);
	de = de->first_child;
	for (;;) {
		if(!de) {
			ret = 1;
			//TODO: unlock;
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
		//TODO: pde_get(de) ? ref count?
		spin_unlock(&sensorfs_biglock);
		if (filldir(dirent, de->name, de->namelen,flip->f_pos, 
			    de->low_ino, de->mode >> 12) < 0) {
			//TODO: pde_put
			goto out;
		}
		//TODO: lock
		spin_lock(&sensorfs_biglock);
		flip->f_pos++;
		next = de->next;
		//TODO: pde_put
		de = next;
	
	} while(de);
	//TODO: unlock
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
//	.open = simple_open,
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
