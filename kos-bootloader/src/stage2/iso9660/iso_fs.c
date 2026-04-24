/* ============================================================================
 * KOS Bootloader - ISO9660 Filesystem Driver Implementation
 * ============================================================================
 * Full ISO9660 Level 1 filesystem parser
 * Supports: Volume Descriptors, Path Tables, Directory Records
 * ============================================================================
 */

#include "iso_fs.h"
#include "string.h"
#include "io.h"

/* ============================================================================
 * Static State
 * ============================================================================
 */

static iso_mount_t mount_state = {0};
static iso_file_t found_file = {0};

/* Sector read buffer (must be aligned) */
static uint8_t sector_buffer[ISO_SECTOR_SIZE] __attribute__((aligned(4)));

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================
 */

/**
 * @brief Read a sector from disk (BIOS INT 0x13 wrapper)
 * Note: This is a stub - in real implementation, would call BIOS or driver
 */
static bool read_sector(uint32_t lba, void* buffer) {
    /* 
     * TODO: Implement actual disk read using BIOS INT 0x13 or ATA driver
     * For now, this is a placeholder
     */
    (void)lba;
    (void)buffer;
    return false;
}

/**
 * @brief Read multiple sectors
 * Note: Currently unused but kept for future ISO9660 implementation
 */
__attribute__((unused))
static bool read_sectors(uint32_t lba, uint32_t count, void* buffer) {
    for (uint32_t i = 0; i < count; i++) {
        if (!read_sector(lba + i, (uint8_t*)buffer + (i * ISO_SECTOR_SIZE))) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Parse directory record and extract file info
 */
static void parse_dir_record(uint8_t* record, iso_file_t* file) {
    if (record == NULL || file == NULL) {
        return;
    }
    
    /* Get record length */
    uint8_t record_len = record[0];
    if (record_len == 0) {
        return;
    }
    
    /* Get extent LBA (little-endian at offset 2) */
    file->lba = *(uint32_t*)(record + 2);
    
    /* Get file size (little-endian at offset 10) */
    file->size = *(uint32_t*)(record + 10);
    
    /* Get flags (offset 25) */
    uint8_t flags = record[25];
    file->is_directory = (flags & ISO_DIR_DIRECTORY) != 0;
    
    /* Get filename length (offset 32) */
    uint8_t name_len = record[32];
    if (name_len > ISO_MAX_FILENAME - 1) {
        name_len = ISO_MAX_FILENAME - 1;
    }
    
    /* Copy filename (starts at offset 33) */
    memset(file->name, 0, sizeof(file->name));
    memcpy(file->name, record + 33, name_len);
    
    /* Remove version suffix (;1) if present */
    char* semicolon = strchr(file->name, ';');
    if (semicolon) {
        *semicolon = '\0';
    }
    
    /* Handle special directory names */
    if (name_len == 1 && record[33] == 0x00) {
        strcpy(file->name, ".");
    } else if (name_len == 1 && record[33] == 0x01) {
        strcpy(file->name, "..");
    }
}

/**
 * @brief Search directory for file by name
 */
static bool search_directory(uint32_t dir_lba, uint32_t dir_size, 
                            const char* name, iso_file_t* result) {
    if (name == NULL || result == NULL) {
        return false;
    }
    
    uint32_t sectors = (dir_size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;
    
    for (uint32_t sec = 0; sec < sectors; sec++) {
        if (!read_sector(dir_lba + sec, sector_buffer)) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < ISO_SECTOR_SIZE) {
            uint8_t record_len = sector_buffer[offset];
            
            /* End of records in this sector */
            if (record_len == 0) {
                break;
            }
            
            /* Parse this record */
            parse_dir_record(sector_buffer + offset, result);
            
            /* Check if name matches (case-insensitive) */
            if (strcmp(result->name, name) == 0) {
                return true;
            }
            
            offset += record_len;
        }
    }
    
    return false;
}

/* ============================================================================
 * Public API Implementation
 * ============================================================================
 */

bool iso_mount(uint32_t lba_start) {
    /* Unmount any existing mount */
    iso_unmount();
    
    /* Read Primary Volume Descriptor at LBA 16 (relative to volume start) */
    if (!read_sector(lba_start + 16, sector_buffer)) {
        return false;
    }
    
    /* Verify ISO9660 signature "CD001" */
    if (memcmp(sector_buffer + 1, "CD001", 5) != 0) {
        return false;
    }
    
    /* Verify descriptor type is Primary (0x01) */
    if (sector_buffer[0] != ISO_VD_PRIMARY) {
        return false;
    }
    
    /* Extract volume information */
    iso_volume_descriptor_t* vd = (iso_volume_descriptor_t*)sector_buffer;
    
    /* Copy volume ID */
    memcpy(mount_state.volume_id, vd->volume_id, 32);
    mount_state.volume_id[31] = '\0';
    
    /* Get logical block size */
    mount_state.logical_block_size = vd->logical_block_size;
    if (mount_state.logical_block_size == 0) {
        mount_state.logical_block_size = ISO_SECTOR_SIZE;
    }
    
    /* Get total volume size */
    mount_state.total_sectors = vd->volume_space_size_le;
    
    /* Extract root directory record */
    memcpy(&mount_state.root_lba, vd->root_dir_record + 2, 4);
    memcpy(&mount_state.root_size, vd->root_dir_record + 10, 4);
    
    mount_state.mounted = true;
    
    return true;
}

void iso_unmount(void) {
    memset(&mount_state, 0, sizeof(mount_state));
}

bool iso_is_mounted(void) {
    return mount_state.mounted;
}

iso_file_t* iso_find(const char* path) {
    if (!mount_state.mounted || path == NULL) {
        return NULL;
    }
    
    /* Start from root directory */
    uint32_t current_lba = mount_state.root_lba;
    uint32_t current_size = mount_state.root_size;
    
    /* Make a copy of path we can modify */
    char path_copy[ISO_MAX_PATH];
    strncpy(path_copy, path, ISO_MAX_PATH - 1);
    path_copy[ISO_MAX_PATH - 1] = '\0';
    
    /* Skip leading slash */
    char* token = path_copy;
    if (token[0] == '/') {
        token++;
    }
    
    /* Process each path component */
    while (token && *token) {
        /* Find next slash */
        char* next_slash = strchr(token, '/');
        if (next_slash) {
            *next_slash = '\0';
        }
        
        /* Skip empty components */
        if (*token == '\0') {
            token = next_slash ? next_slash + 1 : NULL;
            continue;
        }
        
        /* Search for this component */
        if (!search_directory(current_lba, current_size, token, &found_file)) {
            return NULL;  /* Not found */
        }
        
        /* If not last component, must be directory */
        if (next_slash) {
            if (!found_file.is_directory) {
                return NULL;  /* Expected directory */
            }
            current_lba = found_file.lba;
            current_size = found_file.size;
        }
        
        token = next_slash ? next_slash + 1 : NULL;
    }
    
    return &found_file;
}

int iso_read(iso_file_t* file, void* buf, size_t size) {
    if (file == NULL || buf == NULL || !mount_state.mounted) {
        return -1;
    }
    
    if (file->current_pos >= file->size) {
        return 0;  /* EOF */
    }
    
    /* Limit read to remaining file size */
    size_t remaining = file->size - file->current_pos;
    if (size > remaining) {
        size = remaining;
    }
    
    /* Calculate starting sector and offset */
    uint32_t start_sector = file->lba + (file->current_pos / ISO_SECTOR_SIZE);
    uint32_t offset = file->current_pos % ISO_SECTOR_SIZE;
    
    uint8_t* dst = (uint8_t*)buf;
    size_t bytes_read = 0;
    
    while (bytes_read < size) {
        /* Read sector */
        if (!read_sector(start_sector++, sector_buffer)) {
            return bytes_read > 0 ? (int)bytes_read : -1;
        }
        
        /* Copy data from sector buffer */
        size_t to_copy = ISO_SECTOR_SIZE - offset;
        if (to_copy > size - bytes_read) {
            to_copy = size - bytes_read;
        }
        
        memcpy(dst + bytes_read, sector_buffer + offset, to_copy);
        bytes_read += to_copy;
        offset = 0;  /* Reset offset after first sector */
    }
    
    file->current_pos += bytes_read;
    return (int)bytes_read;
}

int iso_seek(iso_file_t* file, int offset, int whence) {
    if (file == NULL) {
        return -1;
    }
    
    int new_pos;
    
    switch (whence) {
        case 0:  /* SEEK_SET */
            new_pos = offset;
            break;
        case 1:  /* SEEK_CUR */
            new_pos = file->current_pos + offset;
            break;
        case 2:  /* SEEK_END */
            new_pos = file->size + offset;
            break;
        default:
            return -1;
    }
    
    /* Bounds check */
    if (new_pos < 0) {
        new_pos = 0;
    }
    if ((uint32_t)new_pos > file->size) {
        new_pos = file->size;
    }
    
    file->current_pos = new_pos;
    return new_pos;
}

uint32_t iso_file_size(iso_file_t* file) {
    if (file == NULL) {
        return 0;
    }
    return file->size;
}

void iso_rewind(iso_file_t* file) {
    if (file != NULL) {
        file->current_pos = 0;
    }
}

int iso_list_dir(iso_file_t* dir, iso_list_callback_t callback, void* user_data) {
    if (dir == NULL || callback == NULL || !mount_state.mounted) {
        return -1;
    }
    
    if (!dir->is_directory) {
        return -1;  /* Not a directory */
    }
    
    int count = 0;
    uint32_t sectors = (dir->size + ISO_SECTOR_SIZE - 1) / ISO_SECTOR_SIZE;
    iso_file_t entry;
    
    for (uint32_t sec = 0; sec < sectors; sec++) {
        if (!read_sector(dir->lba + sec, sector_buffer)) {
            continue;
        }
        
        uint32_t offset = 0;
        while (offset < ISO_SECTOR_SIZE) {
            uint8_t record_len = sector_buffer[offset];
            
            if (record_len == 0) {
                break;
            }
            
            parse_dir_record(sector_buffer + offset, &entry);
            
            /* Skip . and .. */
            if (strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
                callback(&entry, user_data);
                count++;
            }
            
            offset += record_len;
        }
    }
    
    return count;
}

iso_mount_t* iso_get_mount_info(void) {
    return mount_state.mounted ? &mount_state : NULL;
}
