/* file.c: sensorfs file implementation
 *
 * W4118 Homework 6
 */

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sensorfs.h>

#include "internal.h"

//TODO: Probably need a few function implemented here
//and to fill out the following structs appropriately.

const struct file_operations sensorfs_file_operations = {
};

const struct inode_operations sensorfs_file_inode_operations = {
};

const struct file_operations sensorfs_dir_operations = {
};

const struct inode_operations sensorfs_dir_inode_operations = {
};
