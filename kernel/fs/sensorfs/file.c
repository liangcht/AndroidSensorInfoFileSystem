/* file.c: sensorfs file implementation
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sensorfs.h>

#include "internal.h"
extern spinlock_t sensorfs_biglock;
//TODO: Probably need a few function implemented here
//and to fill out the following structs appropriately.
int sensorfs_readdir(struct file *flip, void *dirent, filldir_t filldir)
{
	struct inode *inode = file_inode(flip);

	return sensorfs_readdir_de(SDE(inode), flip, dirent, filldir);
}

int sensorfs_readdir_de(struct sensorfs_dir_entry *de, struct file *flip, void *dirent, filldir_t filldir)
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
		next = de->next';
		//TODO: pde_put
		de = next;
	
	} while(de);
	//TODO: unlock
	spin_unlock(&sensorfs_biglock);
	ret = 1;
out:
	return ret;


}


const struct file_operations sensorfs_file_operations = {
};

const struct inode_operations sensorfs_file_inode_operations = {
};

const struct file_operations sensorfs_dir_operations = {
	.readdir = sensorfs_readdir,
	//TODO: check if there really is a default open function somewhere
	//.open = sensorfs_open
};

const struct inode_operations sensorfs_dir_inode_operations = {
};
