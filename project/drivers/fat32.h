#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdbool.h>

#pragma pack(push, 1)

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

// Global Variables
extern FAT32_BPB bpb;
extern char lfn_buffer[256];

// FAT32 functions
void fat32_init(void);
uint32_t fat_start_sector(void);
uint32_t cluster_to_sector(uint32_t cluster);
bool is_lfn_entry(FAT32_DirectoryEntry* entry);
void append_lfn_part(FAT32_LFNEntry* lfn);
void print_short_name(uint8_t* name);
bool fat32_is_dir(const FAT32_DirectoryEntry* entry);
uint32_t get_entry_cluster(const FAT32_DirectoryEntry* entry);
uint32_t resolve_path_to_cluster(const char* path);
void fat32_list_root_dir(void);

// Directory operations (if you add them)
bool fat32_opendir(FAT32_DIR* dir, uint32_t start_cluster);
bool fat32_readdir(FAT32_DIR* dir, char* name_out, FAT32_DirectoryEntry* entry_out);
void fat32_closedir(FAT32_DIR* dir);
void fat32_list_directory(const char* path);
bool fat32_dir_exists(const char* path);
bool fat32_delete_dir(const char* path);
bool fat32_create_dir(const char* path);

#endif // FAT32_H
