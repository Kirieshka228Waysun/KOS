/* ============================================================================
 * KOS Bootloader - Boot Parameters Structure
 * ============================================================================
 * Data structures for passing information between boot stages
 * ============================================================================
 */

#ifndef KOS_BOOT_PARAMS_H
#define KOS_BOOT_PARAMS_H

#include "types.h"

/* Magic number to identify valid boot params structure */
#define BOOT_PARAMS_MAGIC   0x4B4F5342  /* "KOSB" */

/* Maximum size of boot params structure */
#define BOOT_PARAMS_SIZE    512

/* Boot loader version */
#define BOOTLOADER_VERSION_MAJOR    0
#define BOOTLOADER_VERSION_MINOR    1
#define BOOTLOADER_VERSION_PATCH    0

/**
 * @brief Memory map entry types
 */
typedef enum {
    MEMORY_AVAILABLE = 1,
    MEMORY_RESERVED = 2,
    MEMORY_ACPI_RECLAIMABLE = 3,
    MEMORY_ACPI_NVS = 4,
    MEMORY_BAD = 5
} memory_type_t;

/**
 * @brief Memory map entry (E820 format)
 */
typedef struct {
    uint64_t base;      /* Base address of memory region */
    uint64_t length;    /* Length of memory region in bytes */
    uint32_t type;      /* Type of memory region */
} PACKED memory_map_entry_t;

/**
 * @brief Maximum number of memory map entries
 */
#define MAX_MEMORY_MAP_ENTRIES  32

/**
 * @brief Video mode information
 */
typedef struct {
    uint32_t framebuffer_addr;  /* Physical address of framebuffer */
    uint32_t framebuffer_size;  /* Size of framebuffer in bytes */
    uint16_t width;             /* Screen width in pixels */
    uint16_t height;            /* Screen height in pixels */
    uint16_t pitch;             /* Bytes per scanline */
    uint8_t bits_per_pixel;     /* Color depth */
    uint8_t red_mask_size;      /* Size of red color mask */
    uint8_t green_mask_size;    /* Size of green color mask */
    uint8_t blue_mask_size;     /* Size of blue color mask */
    uint8_t reserved_mask_size; /* Size of reserved mask */
} PACKED video_info_t;

/**
 * @brief Boot parameters passed from bootloader to kernel
 * 
 * This structure is placed at a known physical address (0x9000) before
 * jumping to the kernel. The kernel can read this to get information
 * about the system configuration.
 */
typedef struct {
    /* Header */
    uint32_t magic;                 /* Must be BOOT_PARAMS_MAGIC */
    uint32_t size;                  /* Total size of this structure */
    uint8_t version_major;          /* Bootloader major version */
    uint8_t version_minor;          /* Bootloader minor version */
    uint8_t version_patch;          /* Bootloader patch version */
    uint8_t reserved0;              /* Reserved for alignment */
    
    /* Memory information */
    uint32_t memory_low_kb;         /* Low memory in KB (< 1MB) */
    uint32_t memory_high_kb;        /* High memory in KB (> 1MB) */
    uint32_t memory_map_entries;    /* Number of E820 entries */
    memory_map_entry_t memory_map[MAX_MEMORY_MAP_ENTRIES];
    
    /* Video information */
    video_info_t video;             /* Video mode information */
    
    /* Boot information */
    uint32_t kernel_addr;           /* Physical address where kernel is loaded */
    uint32_t kernel_size;           /* Size of kernel in bytes */
    uint32_t kernel_entry;          /* Kernel entry point address */
    uint32_t cmdline_addr;          /* Physical address of command line */
    uint32_t cmdline_size;          /* Size of command line buffer */
    
    /* Device information */
    uint32_t boot_device;           /* BIOS boot device number */
    uint32_t iso_base_lba;          /* LBA of ISO9660 volume */
    
    /* Reserved for future use */
    uint8_t reserved[128];
    
    /* Checksum (simple sum of all bytes should equal 0) */
    uint8_t checksum;
} PACKED boot_params_t;

/**
 * @brief Boot command line maximum size
 */
#define BOOT_CMDLINE_SIZE   256

/**
 * @brief Get boot params structure pointer
 * @return Pointer to boot params at fixed address
 */
static inline boot_params_t* get_boot_params(void) {
    return (boot_params_t*)0x9000;
}

/**
 * @brief Initialize boot params structure
 * @param params Pointer to boot params structure
 */
static inline void boot_params_init(boot_params_t* params) {
    if (params == NULL) {
        return;
    }
    
    /* Clear entire structure */
    uint8_t* ptr = (uint8_t*)params;
    for (size_t i = 0; i < sizeof(boot_params_t); i++) {
        ptr[i] = 0;
    }
    
    /* Set header fields */
    params->magic = BOOT_PARAMS_MAGIC;
    params->size = sizeof(boot_params_t);
    params->version_major = BOOTLOADER_VERSION_MAJOR;
    params->version_minor = BOOTLOADER_VERSION_MINOR;
    params->version_patch = BOOTLOADER_VERSION_PATCH;
}

/**
 * @brief Calculate and set checksum
 * @param params Pointer to boot params structure
 */
static inline void boot_params_set_checksum(boot_params_t* params) {
    if (params == NULL) {
        return;
    }
    
    uint8_t sum = 0;
    uint8_t* ptr = (uint8_t*)params;
    
    /* Sum all bytes except checksum field */
    for (size_t i = 0; i < sizeof(boot_params_t) - 1; i++) {
        sum += ptr[i];
    }
    
    /* Checksum makes total sum equal to 0 */
    params->checksum = -sum;
}

/**
 * @brief Verify boot params structure
 * @param params Pointer to boot params structure
 * @return true if valid, false otherwise
 */
static inline bool boot_params_verify(boot_params_t* params) {
    if (params == NULL) {
        return false;
    }
    
    /* Check magic number */
    if (params->magic != BOOT_PARAMS_MAGIC) {
        return false;
    }
    
    /* Check size */
    if (params->size != sizeof(boot_params_t)) {
        return false;
    }
    
    /* Verify checksum */
    uint8_t sum = 0;
    uint8_t* ptr = (uint8_t*)params;
    for (size_t i = 0; i < sizeof(boot_params_t); i++) {
        sum += ptr[i];
    }
    
    if (sum != 0) {
        return false;
    }
    
    return true;
}

#endif /* KOS_BOOT_PARAMS_H */
