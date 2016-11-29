/*
 *  linux/include/asm/setup.h
 *
 *  Copyright (C) 1997-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Structure passed to kernel to tell it about the
 *  hardware it's running on.  See Documentation/arm/Setup
 *  for more info.
 */
#ifndef __ASMARM_SETUP_H
#define __ASMARM_SETUP_H

#include <uapi/asm/setup.h>
#ifdef CONFIG_HUAWEI_KERNEL
/* runmode : factory , normal */
//#define ATAG_RUNMODE_FLAG  0x54410102

extern bool is_runmode_factory(void);

#endif
#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag, fn) \
static const struct tagtable __tagtable_##fn __tag = { tag, fn }

/*
 * Memory map description
 */
#define NR_BANKS	CONFIG_ARM_NR_BANKS

extern int arm_add_memory(phys_addr_t start, phys_addr_t size);
extern void early_print(const char *str, ...);
extern void dump_machine_table(void);

#endif