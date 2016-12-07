/* inode.c: sensorfs inode implementations
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/sensorfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/idr.h>
#include <asm/uaccess.h>
#include "internal.h"

#define SENSORFS_DEFAULT_MODE	0444
#define SENSORFS_DYNAMIC_FIRST 0xF0000000U

const char *gps_filename = "gps";
const char *lumi_filename = "lumi";
const char *prox_filename = "prox";
const char *linaccel_filename = "linaccel";

static const struct super_operations sensorfs_ops;
extern const struct inode_operations sensorfs_dir_inode_operations;

static DEFINE_IDA(sensorfs_inum_ida);
static DEFINE_SPINLOCK(sensorfs_inum_lock);
DEFINE_SPINLOCK(sensorfs_biglock); //TODO: USE ME!
static struct kmem_cache *sensorfs_inode_cache;

static struct sensorfs_dir_entry sensorfs_root = {
	.namelen = 0,
	.name = "/sensorfs",
	.parent = &sensorfs_root,
	.size = 0,
	.mode = S_IFDIR
};

int sensorfs_alloc_inum(unsigned int *inum)
{
	unsigned int i;
	int error;

retry:
	if (!ida_pre_get(&sensorfs_inum_ida, GFP_KERNEL))
		return -ENOMEM;

	spin_lock_irq(&sensorfs_inum_lock);
	error = ida_get_new(&sensorfs_inum_ida, &i);
	spin_unlock_irq(&sensorfs_inum_lock);
	if (error == -EAGAIN)
		goto retry;
	else if (error)
		return error;

	if (i > UINT_MAX - SENSORFS_DYNAMIC_FIRST) {
		spin_lock_irq(&sensorfs_inum_lock);
		ida_remove(&sensorfs_inum_ida, i);
		spin_unlock_irq(&sensorfs_inum_lock);
		return -ENOSPC;
	}
	*inum = SENSORFS_DYNAMIC_FIRST + i;
	return 0;
}

static struct sensorfs_inode *inode_to_sensorfs_inode(struct inode *inode)
{
	return container_of(inode, struct sensorfs_inode, vfs_inode);
}

struct inode *sensorfs_get_inode(struct super_block *sb,
	const struct inode *dir,
	struct sensorfs_dir_entry *de)
{
	//TODO: Complete the implementation
	//You will also have to use this function somewhere.
	//This function should construct an inode representing
	//a given sensorfs_dir_entry (de) under a parent directory
	//Should also handle the situation when dir is NULL meaning
	//return the root directory entry's inode
	return NULL;
}

static int sensorfs_delete_dentry(const struct dentry *dentry)
{
	return 1;
}


static struct sensorfs_dir_entry *sensorfs_sde_lookup(
	struct sensorfs_dir_entry *parent,
	const char *name) __attribute__((unused));
//TODO: Remove the above prototype def'n, only used to suppress
//compiler warning in this skeleton state

static struct sensorfs_dir_entry *sensorfs_sde_lookup(
	struct sensorfs_dir_entry *parent,
	const char *name)
{
	//TODO: Implement
	//Given a parent, and a name of a file, returns the
	//sensorfs_dir_entry corresponding to that name
	return NULL;
}

static void init_once(void *toinit)
{
	struct sensorfs_inode *sinode = (struct sensorfs_inode *)toinit;

	inode_init_once(&sinode->vfs_inode);
}

static void sensorfs_init_nodecache(void)
{
	sensorfs_inode_cache = kmem_cache_create("sensorfs_inode_cache",
		sizeof(struct sensorfs_inode),
		0, (SLAB_RECLAIM_ACCOUNT|
		SLAB_MEM_SPREAD|SLAB_PANIC), init_once);
}

static struct inode *sensorfs_alloc_inode(struct super_block *sb)
{
	struct sensorfs_inode *sinode =
		kmem_cache_alloc(sensorfs_inode_cache, GFP_KERNEL);
	if (!sinode)
		return NULL;

	sinode->sde = NULL;
	return &(sinode->vfs_inode);
}


static void sensorfs_destroy_inode(struct inode *inode)
{
	struct sensorfs_inode *sinode = inode_to_sensorfs_inode(inode);

	kmem_cache_free(sensorfs_inode_cache, sinode);
}

void sensorfs_create_sfile(struct sensorfs_dir_entry *parent, const char *name)
{
	//TODO: Implement
	struct sensorfs_dir_entry *ent = NULL;
	ent = kzalloc(sizeof(struct sensorfs_dir_entry), GFP_KERNEL);
	if (!ent) 
		return;
	
	ent->mode = S_IFREG;
	ent->name = name;
	ent->namelen = strlen(name);
	ent->size = 0;
	ent->next = parent->first_child;
	ent->parent = parent;
	parent->first_child = ent;
}

static struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags) __attribute__((unused));
//TODO: Remove the above prototype def'n, only used to suppress
//compiler warning in this skeleton state

static struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags)
{
	//TODO: Implement
	return NULL;
}

int sensorfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *inode;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_CACHE_SIZE;
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= RAMFS_MAGIC; //TODO: FIXME
	sb->s_op		= &sensorfs_ops;
	sb->s_time_gran		= 1;

	inode = sensorfs_get_inode(sb, NULL, &sensorfs_root);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

struct dentry *sensorfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	//TODO: Probably have to do a little thing here.
	return mount_nodev(fs_type, flags, data, sensorfs_fill_super);
}

static void sensorfs_kill_sb(struct super_block *sb)
{
	kill_litter_super(sb);
}

static int __init init_sensorfs_fs(void)
{
	sensorfs_init_nodecache();
	sensorfs_create_sfile(&sensorfs_root, gps_filename);
	sensorfs_create_sfile(&sensorfs_root, lumi_filename);
	sensorfs_create_sfile(&sensorfs_root, prox_filename);
	sensorfs_create_sfile(&sensorfs_root, linaccel_filename);
	return register_filesystem(&sensorfs_fs_type);
}
module_init(init_sensorfs_fs)

static const struct dentry_operations sensorfs_dentry_operations = {
	.d_delete	   = sensorfs_delete_dentry,
};

struct file_system_type sensorfs_fs_type = {
	.name		= "sensorfs",
	.mount		= sensorfs_mount,
	.kill_sb	= sensorfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};

static const struct super_operations sensorfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= generic_show_options,
	.alloc_inode	= sensorfs_alloc_inode,
	.destroy_inode	= sensorfs_destroy_inode,
};

