/*
 * Kernel API extracted from Linux kernel headers.
 */

#ifndef __KERNEL_API
#define __KERNEL_API

#define DECLARE_IMV(type, name) extern __typeof__(type) name##__imv
#define DEFINE_IMV(type, name)  __typeof__(type) name##__imv

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

/**
 * imv_read - read immediate variable
 * @name: immediate value name
 *
 * Reads the value of @name.
 */
#define imv_read(name)			_imv_read(name)

/**
 * _imv_read - Read immediate value with standard memory load.
 * @name: immediate value name
 *
 * Force a data read of the immediate value instead of the immediate value
 * based mechanism. Useful for __init and __exit section data read.
 */
#define _imv_read(name)		(name##__imv)

#endif
