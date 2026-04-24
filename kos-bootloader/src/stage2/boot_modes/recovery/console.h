/* ============================================================================
 * KOS Bootloader - Recovery Console Header
 * ============================================================================
 * Command-line interface for recovery operations
 * ============================================================================
 */

#ifndef KOS_CONSOLE_H
#define KOS_CONSOLE_H

#include "types.h"

/**
 * @brief Main entry point for recovery console
 * Runs interactive CLI until user exits
 */
void recovery_console_main(void);

#endif /* KOS_CONSOLE_H */
