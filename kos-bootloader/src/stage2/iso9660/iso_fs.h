/* ============================================================================
 * KOS Bootloader - ISO9660 Filesystem Driver Header
 * ============================================================================
 * Full ISO9660 Level 1 filesystem parser
 * Supports: Volume Descriptors, Path Tables, Directory Records
 * ============================================================================
 */

#ifndef KOS_ISO_FS_H
#define KOS_ISO_FS_H

#include "types.h"

/* ============================================================================
 * ISO9660 Constants
 * ============================================================================
 */

/* Sector size */
#define ISO_SECTOR_SIZE     2048

/* Volume Descriptor types */
#define ISO_VD_BOOT         0x00
#define ISO_VD_PRIMARY      0x01
#define ISO_VD_SUPPLEMENTARY 0x02
#define ISO_VD_TERMINATOR   0xFF

/* Directory record flags */
#define ISO_DIR_HIDDEN      0x01
#define ISO_DIR_DIRECTORY   0x02
#define ISO_DIR_ASSOCIATED  0x04
#define ISO_DIR_EXTENDED    0x08
#define ISO_DIR_PROTECTED   0x10

/* Maximum path length */
#define ISO_MAX_PATH        256

/* Maximum filename length */
#define ISO_MAX_FILENAME    37

/* ============================================================================
 * Data Structures
 * ============================================================================
 */

/**
 * @brief ISO9660 file handle
 */
typedef struct {
    uint32_t lba;           /* Starting LBA of file */
    uint32_t size;          /* File size in bytes */
    uint32_t current_pos;   /* Current read position */
    char name[ISO_MAX_FILENAME];  /* Filename */
    bool is_directory;      /* True if directory */
} iso_file_t;

/**
 * @brief ISO9660 volume descriptor (Primary)
 */
typedef struct {
    uint8_t type;                   /* Volume descriptor type */
    char id[5];                     /* "CD001" */
    uint8_t version;                /* Descriptor version */
    uint8_t reserved1;              /* Reserved */
    char system_id[32];             /* System identifier */
    char volume_id[32];             /* Volume identifier */
    uint8_t reserved2[8];           /* Reserved */
    uint32_t volume_space_size_le;  /* Volume space size (little-endian) */
    uint8_t reserved3[32];          /* Reserved */
    uint32_t volume_set_size;       /* Volume set size */
    uint32_t volume_sequence_num;   /* Volume sequence number */
    uint32_t logical_block_size;    /* Logical block size */
    uint32_t path_table_size;       /* Path table size */
    uint32_t path_table_lba_le;     /* Path table LBA (little-endian) */
    uint32_t path_table_lba_be;     /* Path table LBA (big-endian) */
    uint8_t root_dir_record[34];    /* Root directory record */
    char volume_set_id[128];        /* Volume set identifier */
    char publisher_id[128];         /* Publisher identifier */
    char preparer_id[128];          /* Preparer identifier */
    char application_id[128];       /* Application identifier */
    char copyright_file_id[37];     /* Copyright file identifier */
    char abstract_file_id[37];      /* Abstract file identifier */
    char bibliographic_file_id[37]; /* Bibliographic file identifier */
    /* ... more fields follow */
} PACKED iso_volume_descriptor_t;

/**
 * @brief ISO9660 mount state
 */
typedef struct {
    bool mounted;                   /* True if filesystem is mounted */
    uint32_t root_lba;              /* Root directory LBA */
    uint32_t root_size;             /* Root directory size */
    uint32_t total_sectors;         /* Total sectors in volume */
    uint32_t logical_block_size;    /* Block size (usually 2048) */
    char volume_id[32];             /* Volume identifier */
} iso_mount_t;

/* ============================================================================
 * Function Prototypes
 * ============================================================================
 */

/**
 * @brief Mount ISO9660 filesystem
 * @param lba_start Starting LBA of volume (usually 16)
 * @return true if mount successful
 */
bool iso_mount(uint32_t lba_start);

/**
 * @brief Unmount ISO9660 filesystem
 */
void iso_unmount(void);

/**
 * @brief Check if filesystem is mounted
 * @return true if mounted
 */
bool iso_is_mounted(void);

/**
 * @brief Find file by full path
 * @param path Full path to file (e.g., "/boot/boot32.sys")
 * @return Pointer to file info, or NULL if not found
 */
iso_file_t* iso_find(const char* path);

/**
 * @brief Read data from file
 * @param file File handle
 * @param buf Buffer to read into
 * @param size Number of bytes to read
 * @return Number of bytes actually read, or -1 on error
 */
int iso_read(iso_file_t* file, void* buf, size_t size);

/**
 * @brief Seek to position in file
 * @param file File handle
 * @param offset Offset to seek to
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return New position, or -1 on error
 */
int iso_seek(iso_file_t* file, int offset, int whence);

/**
 * @brief Get file size
 * @param file File handle
 * @return File size in bytes
 */
uint32_t iso_file_size(iso_file_t* file);

/**
 * @brief Reset file position to beginning
 * @param file File handle
 */
void iso_rewind(iso_file_t* file);

/**
 * @brief List directory contents
 * @param dir Directory file handle
 * @param callback Function called for each entry
 * @param user_data User data passed to callback
 * @return Number of entries, or -1 on error
 */
typedef void (*iso_list_callback_t)(iso_file_t* file, void* user_data);
int iso_list_dir(iso_file_t* dir, iso_list_callback_t callback, void* user_data);

/**
 * @brief Get volume information
 * @return Pointer to mount structure, or NULL if not mounted
 */
iso_mount_t* iso_get_mount_info(void);

#endif /* KOS_ISO_FS_H */
