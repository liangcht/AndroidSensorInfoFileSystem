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
#include <linux/sensor_types.h>
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
#define SENSORFS_ROOT_INO 1
const char *gps_filename = "gps";
const char *lumi_filename = "lumi";
const char *prox_filename = "prox";
const char *linaccel_filename = "linaccel";

static const struct dentry_operations sensorfs_dentry_operations;
struct file_system_type sensorfs_fs_type;	
static const struct super_operations sensorfs_ops;
extern const struct inode_operations sensorfs_dir_inode_operations;
extern const struct file_operations sensorfs_file_operations;

static DEFINE_IDA(sensorfs_inum_ida);
static DEFINE_SPINLOCK(sensorfs_inum_lock);
DEFINE_SPINLOCK(sensorfs_biglock); //TODO: USE ME!
static struct kmem_cache *sensorfs_inode_cache;

static struct sensorfs_dir_entry sensorfs_root = {
	.namelen = 9,
	.name = "/sensorfs",
	.parent = &sensorfs_root,
	.size = 0,
	.mode = S_IFDIR | SENSORFS_DEFAULT_MODE,
	.low_ino = SENSORFS_ROOT_INO
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
	spin_lock(&sensorfs_biglock);
	if (dir == NULL) {
		if (sb->s_root != NULL){
			spin_unlock(&sensorfs_biglock);
			return sb->s_root->d_inode;
		}
	}

	struct inode *inode = new_inode_pseudo(sb);
	printk("Create new inode %d\n", inode->i_ino);	

	if(inode) {
		inode->i_ino = de->low_ino;
		inode->i_mtime = de->m_time;
		inode->i_atime = CURRENT_TIME;
		inode->i_ctime = de->c_time;
		inode_to_sensorfs_inode(inode)->sde = de;

		if (de->mode) {
			inode->i_mode = de->mode;
			//TODO: check de->uid, de->gid
		}
		if (de->size) {
			if (de->size >= 8192)
				inode->i_size = 8192;
			else
				inode->i_size = de->size;
		}
		//TODO: nlink
		if (S_ISREG(inode->i_mode)) {
			inode->i_op = &sensorfs_file_inode_operations;
			inode->i_fop = &sensorfs_file_operations;
		} else if (S_ISDIR(inode->i_mode)){
			inode->i_op = &sensorfs_dir_inode_operations;
			inode->i_fop = &sensorfs_dir_operations;
		} else {
			WARN_ON(1);
			spin_unlock(&sensorfs_biglock);
			return NULL;
		}

	}
	else {
		//TODO: pde_put(de)
	}
	spin_unlock(&sensorfs_biglock);
	return inode;
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
	struct sensorfs_dir_entry *de,
	const char *name)
{
	//TODO: Implement
	//Given a parent, and a name of a file, returns the
	//sensorfs_dir_entry corresponding to that name
	struct inode *inode;
	//TODO: lock
	spin_lock(&sensorfs_biglock);
	for (de = de->first_child; de; de = de->next) {
		if (de->namelen != strlen(name))
			continue;
		if (!memcmp(name, de->name, de->namelen)) {
			//TODO: pdeget(de)
			//TODO: unlock
			spin_unlock(&sensorfs_biglock);
			return de;
		}
	}
	spin_unlock(&sensorfs_biglock);
	return NULL;
}

void add_to_buf(struct sensorfs_dir_entry *sde, char *input)
{
	spin_lock(&sensorfs_biglock);
	char *buf = sde->contents;
	int to_write = sde->size % 8192;
	int n = 8192 - to_write;
	
	if (strlen(input) > n) {
		memcpy(buf + to_write, input, n);
		memcpy(buf, input + n, strlen(input) - n);
	} else {
		memcpy(buf + to_write, input, strlen(input));
	}
	sde->size += strlen(input);
	sde->m_time = CURRENT_TIME;
	spin_unlock(&sensorfs_biglock);
}


int syscall_only_write(struct sensor_information *si)
{
	long curr_time = get_seconds();
	struct sensorfs_dir_entry *gps_sde;
	struct sensorfs_dir_entry *lumi_sde;
	struct sensorfs_dir_entry *prox_sde;
	struct sensorfs_dir_entry *linaccel_sde;

	gps_sde = sensorfs_sde_lookup(&sensorfs_root, "gps");
	lumi_sde = sensorfs_sde_lookup(&sensorfs_root, "lumi");
	prox_sde = sensorfs_sde_lookup(&sensorfs_root, "prox");
	linaccel_sde = sensorfs_sde_lookup(&sensorfs_root, "linaccel");
	
	char temp[500];
	sprintf(temp, "TimeStamp:%ld,MicroLatitude:%d,MicroLongitude:%d\n",
		curr_time, si->microlatitude, si->microlongitude);
	add_to_buf(gps_sde, temp);
	memset(temp, 0, 500);

	sprintf(temp, "TimeStamp:%ld,Centilux:%d\n",
		 curr_time, si->centilux);
	add_to_buf(lumi_sde, temp);
	memset(temp, 0, 500);

	sprintf(temp, "TimeStamp:%ld,Centiproximity:%d\n",
		curr_time, si->centiproximity);
	add_to_buf(prox_sde, temp);
	memset(temp, 0, 500);
	
	sprintf(temp,
		"TimeStamp:%ld,CentiAccelX:%d,CentiAccelY:%d,CentiAccelZ:%d\n",
	        curr_time, si->centilinearaccelx, si->centilinearaccely,
		si->centilinearaccelz);
	add_to_buf(linaccel_sde, temp);
	memset(temp, 0, 500);
	return 0;
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
	printk("Enter creat_sfile\n");
	//TODO: investigate whether we need to keep createtime 
	struct sensorfs_dir_entry *ent = NULL;
	ent = kzalloc(sizeof(struct sensorfs_dir_entry), GFP_KERNEL);
	if (!ent) 
		return;
	if (sensorfs_alloc_inum(&ent->low_ino))
		return;
	spin_lock(&sensorfs_biglock);
	ent->mode = S_IFREG | SENSORFS_DEFAULT_MODE;
	ent->name = name;
	ent->namelen = strlen(name);
	ent->size = 0;
	ent->next = parent->first_child;
	ent->parent = parent;
	ent->m_time = CURRENT_TIME;
	ent->a_time = CURRENT_TIME;
	ent->c_time = CURRENT_TIME;
	parent->first_child = ent;
	spin_unlock(&sensorfs_biglock);
}


static struct dentry *sensorfs_lookup_de(struct sensorfs_dir_entry *de, struct inode *dir, struct dentry *dentry)
{
	struct inode *inode;
	//TODO: lock
	spin_lock(&sensorfs_biglock);
	for (de = de->first_child; de; de = de->next) {
		if (de->namelen != dentry->d_name.len)
			continue;
		if (!memcmp(dentry->d_name.name, de->name, de->namelen)) {
			//TODO: pdeget(de)
			//TODO: unlock
			spin_unlock(&sensorfs_biglock);
			inode = sensorfs_get_inode(dir->i_sb, dir, de);
			if (!inode)
				return ERR_PTR(-ENOMEM);
			
			d_set_d_op(dentry, &sensorfs_dentry_operations);
			d_add(dentry, inode);
			return NULL;
		}
	}
	//TODO: unlock
	spin_unlock(&sensorfs_biglock);
	return ERR_PTR(-ENOENT);
}

/*
static struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags) __attribute__((unused));
//TODO: Remove the above prototype def'n, only used to suppress
//compiler warning in this skeleton state
*/
struct dentry *sensorfs_lookup(struct inode *dir, struct dentry *dentry,
	unsigned int flags)
{
	//TODO: Implement
	return sensorfs_lookup_de(SDE(dir), dir, dentry);
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
	sensorfs_root.c_time = CURRENT_TIME;
	sensorfs_root.m_time = CURRENT_TIME;
	sensorfs_root.a_time = CURRENT_TIME;

	return mount_nodev(fs_type, flags, data, sensorfs_fill_super);
}

static void sensorfs_kill_sb(struct super_block *sb)
{
	kill_litter_super(sb);
}


static int __init init_sensorfs_fs(void)
{
	printk("Enter init_sensorfs_fs\n");
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
