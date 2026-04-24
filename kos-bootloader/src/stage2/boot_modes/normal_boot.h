/* ============================================================================
 * KOS Bootloader - Normal Boot Mode
 * ============================================================================
 * Loads and executes /boot/boot32.sys kernel
 * ============================================================================
 */

#ifndef KOS_NORMAL_BOOT_H
#define KOS_NORMAL_BOOT_H

#include "types.h"

/**
 * @brief Execute normal boot - load and jump to kernel
 * This function never returns on success
 */
void normal_boot(void);

#endif /* KOS_NORMAL_BOOT_H */
