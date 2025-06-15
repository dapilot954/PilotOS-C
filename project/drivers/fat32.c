#include "print.h"
#include <stdint.h>
#include <stdbool.h>

#pragma pack(push, 1)  // Prevent structure padding

typedef struct {
    uint8_t jumpBoot[3];
    uint8_t OEMName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t numFATs;
    uint16_t rootEntryCount;
    uint16_t totalSectors16;
    uint8_t media;
    uint16_t FATSize16;
    uint16_t sectorsPerTrack;
    uint16_t numberOfHeads;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;

    // Extended Boot Record
    uint32_t FATSize32;
    uint16_t extFlags;
    uint16_t fsVersion;
    uint32_t rootCluster;
    uint16_t fsInfo;
    uint16_t backupBootSector;
    uint8_t reserved[12];
    uint8_t driveNumber;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeID;
    uint8_t volumeLabel[11];
    uint8_t fileSystemType[8];
} FAT32_BPB;

typedef struct {
    uint8_t name[11];
    uint8_t attr;
    uint8_t ntres;
    uint8_t crtTimeTenth;
    uint16_t crtTime;
    uint16_t crtDate;
    uint16_t lstAccDate;
    uint16_t firstClusterHigh;
    uint16_t wrtTime;
    uint16_t wrtDate;
    uint16_t firstClusterLow;
    uint32_t fileSize;
} FAT32_DirectoryEntry;

typedef struct {
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t zero;
    uint16_t name3[2];
} FAT32_LFNEntry;

#pragma pack(pop)

typedef struct {
    uint32_t cluster;
    uint8_t sectorBuffer[512];
    uint8_t sectorIndex;
    uint8_t entryIndex;
    bool end;
    char lfnBuffer[256];
} FAT32_DIR;

#define FAT_ENTRY_EOC 0x0FFFFFFF

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
unsigned int strlen(const char* str) {
    unsigned int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void* memset(void* dest, int value, unsigned int count) {
    unsigned char* ptr = dest;
    while (count--) {
        *ptr++ = (unsigned char)value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned int count) {
    unsigned char* dst8 = (unsigned char*)dest;
    const unsigned char* src8 = (const unsigned char*)src;
    while (count--) {
        *dst8++ = *src8++;
    }
    return dest;
}

char* strrchr(const char* str, int ch) {
    const char* last = 0;
    while (*str) {
        if (*str == (char)ch) {
            last = str;
        }
        str++;
    }
    return (char*)last;
}




FAT32_BPB bpb;
char lfn_buffer[256];

void fat32_init() {
    uint8_t sector[512];
    ata_pio_read(0, sector);
    bpb = *(FAT32_BPB*)sector;
}

uint32_t fat_start_sector() {
    return bpb.reservedSectorCount;
}

uint32_t cluster_to_sector(uint32_t cluster) {
    return bpb.reservedSectorCount + (bpb.numFATs * bpb.FATSize32) +
           ((cluster - 2) * bpb.sectorsPerCluster);
}

bool is_lfn_entry(FAT32_DirectoryEntry* entry) {
    return (entry->attr & 0x0F) == 0x0F;
}

void append_lfn_part(FAT32_LFNEntry* lfn) {
    int pos = ((lfn->order & 0x1F) - 1) * 13;

    // name1
    for (int i = 0; i < 5; i++) {
        if (lfn->name1[i] == 0xFFFF || lfn->name1[i] == 0x0000) break;
        lfn_buffer[pos++] = (char)lfn->name1[i];
    }

    // name2
    for (int i = 0; i < 6; i++) {
        if (lfn->name2[i] == 0xFFFF || lfn->name2[i] == 0x0000) break;
        lfn_buffer[pos++] = (char)lfn->name2[i];
    }

    // name3
    for (int i = 0; i < 2; i++) {
        if (lfn->name3[i] == 0xFFFF || lfn->name3[i] == 0x0000) break;
        lfn_buffer[pos++] = (char)lfn->name3[i];
    }

    lfn_buffer[pos] = '\0';
}

void print_short_name(uint8_t* name) {
    char filename[13];
    int k = 0;

    // Copy name (first 8 characters)
    for (int i = 0; i < 8 && name[i] != ' '; i++) {
        filename[k++] = name[i];
    }

    // Check if extension is not all spaces
    if (name[8] != ' ') {
        filename[k++] = '.';
        for (int i = 8; i < 11 && name[i] != ' '; i++) {
            filename[k++] = name[i];
        }
    }

    filename[k] = '\0';
    print(filename); print("\n");
}

bool fat32_is_dir(const FAT32_DirectoryEntry* entry) {
    return (entry->attr & 0x10) != 0;
}

bool fat32_opendir(FAT32_DIR* dir, uint32_t cluster) {
    dir->cluster = cluster;
    dir->sectorIndex = 0;
    dir->entryIndex = 0;
    dir->end = false;
    dir->lfnBuffer[0] = '\0';

    ata_pio_read(cluster_to_sector(cluster), dir->sectorBuffer);
    return true;
}

bool fat32_readdir(FAT32_DIR* dir, char* name_out, FAT32_DirectoryEntry* entry_out) {
    if (dir->end) return false;

    while (1) {
        FAT32_DirectoryEntry* entries = (FAT32_DirectoryEntry*)dir->sectorBuffer;

        if (dir->entryIndex >= 512 / sizeof(FAT32_DirectoryEntry)) {
            dir->entryIndex = 0;
            dir->sectorIndex++;

            if (dir->sectorIndex >= bpb.sectorsPerCluster) {
                dir->end = true;
                return false;
            }

            ata_pio_read(cluster_to_sector(dir->cluster) + dir->sectorIndex, dir->sectorBuffer);
            continue;
        }

        FAT32_DirectoryEntry* entry = &entries[dir->entryIndex++];

        if (entry->name[0] == 0x00) {
            dir->end = true;
            return false;
        }

        if (entry->name[0] == 0xE5) continue; // Deleted

        if (is_lfn_entry(entry)) {
            append_lfn_part((FAT32_LFNEntry*)entry);
            continue;
        }

        // Normal entry
        *entry_out = *entry;
        if (lfn_buffer[0] != '\0') {
            for (int i = 0; lfn_buffer[i]; i++) name_out[i] = lfn_buffer[i];
            name_out[strlen(lfn_buffer)] = '\0';
            lfn_buffer[0] = '\0';
        } else {
            // Fallback: convert short name
            char* n = name_out;
            for (int i = 0; i < 8 && entry->name[i] != ' '; i++) *n++ = entry->name[i];
            if (entry->name[8] != ' ') {
                *n++ = '.';
                for (int i = 8; i < 11 && entry->name[i] != ' '; i++) *n++ = entry->name[i];
            }
            *n = '\0';
        }

        return true;
    }
}

void fat32_closedir(FAT32_DIR* dir) {
    // No cleanup needed in this implementation
    (void)dir;
}


uint32_t get_entry_cluster(const FAT32_DirectoryEntry* entry) {
    return ((uint32_t)entry->firstClusterHigh << 16) | entry->firstClusterLow;
}

uint32_t resolve_path_to_cluster(const char* path) {
    if (path[0] == '/' || path[0] == '\\') path++; // skip initial slash
    if (path[0] == '\0') return bpb.rootCluster;

    FAT32_DIR dir;
    char segment[256];
    int segIndex = 0;

    uint32_t currentCluster = bpb.rootCluster;

    while (1) {
        segIndex = 0;
        while (*path && *path != '/' && *path != '\\') {
            segment[segIndex++] = *path++;
        }
        segment[segIndex] = '\0';
        if (*path == '/' || *path == '\\') path++;

        fat32_opendir(&dir, currentCluster);
        char name[256];
        FAT32_DirectoryEntry entry;
        bool found = false;

        while (fat32_readdir(&dir, name, &entry)) {
            if (!fat32_is_dir(&entry)) continue;
            if (strcmp(name, segment) == 0) {
                currentCluster = get_entry_cluster(&entry);
                found = true;
                break;
            }
        }

        fat32_closedir(&dir);

        if (!found) return 0; // not found
        if (*path == '\0') return currentCluster;
    }
}


void fat32_list_root_dir() {
    uint8_t sector[512];
    uint32_t cluster = bpb.rootCluster;

    while (1) {
        for (uint8_t i = 0; i < bpb.sectorsPerCluster; i++) {
            ata_pio_read(cluster_to_sector(cluster) + i, sector);

            FAT32_DirectoryEntry* entry = (FAT32_DirectoryEntry*)sector;
            lfn_buffer[0] = '\0';

            for (int j = 0; j < 512 / sizeof(FAT32_DirectoryEntry); j++) {
                if (entry[j].name[0] == 0x00) return; // No more entries
                if (entry[j].name[0] == 0xE5) continue; // Deleted entry

                if (is_lfn_entry(&entry[j])) {
                    append_lfn_part((FAT32_LFNEntry*)&entry[j]);
                    continue;
                }

                // Normal 8.3 entry
                if (lfn_buffer[0] != '\0') {
                    print(lfn_buffer);
                    print("\n");
                    lfn_buffer[0] = '\0';  // reset for next
                } else {
                    print_short_name(entry[j].name);
                }
            }
        }

        // NOTE: currently not following FAT cluster chains
        break;
    }
}

void fat32_list_directory(const char* path) {
    FAT32_DIR dir;
    FAT32_DirectoryEntry entry;
    char name[256];

    uint32_t cluster = resolve_path_to_cluster(path);
    if (cluster == 0) {
        print("Directory not found\n");
        return;
    }

    if (!fat32_opendir(&dir, cluster)) {
        print("Failed to open directory\n");
        return;
    }

    print("Directory listing:\n");

    while (fat32_readdir(&dir, name, &entry)) {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue; // skip special entries
        }

        print(name);
        if (fat32_is_dir(&entry)) {
            print(" [DIR]");
        }
        print("\n");
    }

    fat32_closedir(&dir);
}

bool fat32_dir_exists(const char* path) {
    return resolve_path_to_cluster(path) != 0;
}

uint32_t fat32_find_free_cluster() {
    uint32_t fat_start = fat_start_sector();
    uint32_t entries = bpb.FATSize32 * 512 / 4;
    uint8_t sector[512];

    for (uint32_t i = 2; i < entries; i++) {
        uint32_t offset = i * 4;
        uint32_t sector_num = fat_start + (offset / 512);
        uint32_t sector_offset = offset % 512;

        ata_pio_read(sector_num, sector);
        uint32_t entry = *(uint32_t*)&sector[sector_offset] & 0x0FFFFFFF;
        if (entry == 0x00000000) {
            // Mark it as EOC
            *(uint32_t*)&sector[sector_offset] = FAT_ENTRY_EOC;
            ata_pio_write(sector_num, sector);
            return i;
        }
    }
    return 0; // No free cluster found
}


// Helper: Write FAT entry
void fat32_set_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t offset = cluster * 4;
    uint32_t sector_num = fat_start_sector() + (offset / 512);
    uint32_t sector_offset = offset % 512;
    uint8_t sector[512];

    ata_pio_read(sector_num, sector);
    *(uint32_t*)&sector[sector_offset] = value;
    ata_pio_write(sector_num, sector);
}

// Create directory
bool fat32_create_dir(const char* path) {
    // Split into parent path and new dir name
    char parent[256], name[256];
    strncpy(parent, path, sizeof(parent));
    char* last = strrchr(parent, '/');
    if (!last) return false;
    strncpy(name, last + 1, sizeof(name));
    *last = '\0';
    if (strlen(parent) == 0) strcpy(parent, "/");

    uint32_t parent_cluster = resolve_path_to_cluster(parent);
    if (parent_cluster == 0) return false;

    // Ensure directory with same name does not exist
    FAT32_DIR dir;
    FAT32_DirectoryEntry entry;
    char entry_name[256];
    fat32_opendir(&dir, parent_cluster);
    while (fat32_readdir(&dir, entry_name, &entry)) {
        if (strcmp(entry_name, name) == 0) return false; // Already exists
    }

    // Find free cluster
    uint32_t new_cluster = fat32_find_free_cluster();
    if (new_cluster == 0) return false;

    // Initialize the new cluster with . and ..
    FAT32_DirectoryEntry entries[16] = {0};
    memset(entries, 0, sizeof(entries));

    // . entry
    memcpy(entries[0].name, ".          ", 11);
    entries[0].attr = 0x10;
    entries[0].firstClusterLow = new_cluster & 0xFFFF;
    entries[0].firstClusterHigh = (new_cluster >> 16) & 0xFFFF;

    // .. entry
    memcpy(entries[1].name, "..         ", 11);
    entries[1].attr = 0x10;
    entries[1].firstClusterLow = parent_cluster & 0xFFFF;
    entries[1].firstClusterHigh = (parent_cluster >> 16) & 0xFFFF;

    // Write new directory entries
    ata_pio_write(cluster_to_sector(new_cluster), (uint8_t*)entries);

    // Add entry to parent directory
    fat32_opendir(&dir, parent_cluster);
    for (int s = 0; s < bpb.sectorsPerCluster; s++) {
        ata_pio_read(cluster_to_sector(parent_cluster) + s, dir.sectorBuffer);
        FAT32_DirectoryEntry* ents = (FAT32_DirectoryEntry*)dir.sectorBuffer;
        for (int i = 0; i < 512 / sizeof(FAT32_DirectoryEntry); i++) {
            if (ents[i].name[0] == 0x00 || ents[i].name[0] == 0xE5) {
                // Add entry
                memset(&ents[i], 0, sizeof(FAT32_DirectoryEntry));
                int len = strlen(name);
                memset(ents[i].name, ' ', 11);
                for (int j = 0; j < len && j < 11; j++) ents[i].name[j] = name[j];
                ents[i].attr = 0x10;
                ents[i].firstClusterLow = new_cluster & 0xFFFF;
                ents[i].firstClusterHigh = (new_cluster >> 16) & 0xFFFF;
                ata_pio_write(cluster_to_sector(parent_cluster) + s, dir.sectorBuffer);
                return true;
            }
        }
    }

    return false;
}

// Delete empty directory
bool fat32_delete_dir(const char* path) {
    uint32_t cluster = resolve_path_to_cluster(path);
    if (cluster == 0) return false;

    // Check if empty
    FAT32_DIR dir;
    FAT32_DirectoryEntry entry;
    char name[256];
    fat32_opendir(&dir, cluster);
    while (fat32_readdir(&dir, name, &entry)) {
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        return false; // Not empty
    }

    // Clear FAT entry
    fat32_set_fat_entry(cluster, 0x00000000);

    // Remove from parent directory
    char parent[256], leaf[256];
    strncpy(parent, path, sizeof(parent));
    char* last = strrchr(parent, '/');
    if (!last) return false;
    strncpy(leaf, last + 1, sizeof(leaf));
    *last = '\0';
    if (strlen(parent) == 0) strcpy(parent, "/");

    uint32_t parent_cluster = resolve_path_to_cluster(parent);
    if (parent_cluster == 0) return false;

    for (int s = 0; s < bpb.sectorsPerCluster; s++) {
        uint8_t buffer[512];
        ata_pio_read(cluster_to_sector(parent_cluster) + s, buffer);
        FAT32_DirectoryEntry* ents = (FAT32_DirectoryEntry*)buffer;
        for (int i = 0; i < 512 / sizeof(FAT32_DirectoryEntry); i++) {
            if (ents[i].name[0] == 0x00) break;
            if (ents[i].name[0] == 0xE5) continue;
            char short_name[12];
            strncpy(short_name, (char*)ents[i].name, 11);
            short_name[11] = '\0';
            for (int k = 10; k >= 0 && short_name[k] == ' '; k--) short_name[k] = '\0';
            if (strcmp(short_name, leaf) == 0) {
                ents[i].name[0] = 0xE5; // Mark deleted
                ata_pio_write(cluster_to_sector(parent_cluster) + s, buffer);
                return true;
            }
        }
    }

    return false;
}
