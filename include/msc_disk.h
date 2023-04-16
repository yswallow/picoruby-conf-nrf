/* mruby/c VM */
#include <mrubyc.h>

#define FLASH_TARGET_OFFSET  0x000A0000UL                       /* 512KiB */
#define FLASH_MMAP_ADDR      FLASH_TARGET_OFFSET
#define SECTOR_SIZE          4096 /* same to nrf pagesize*/
#define FLASH_SECTOR_SIZE SECTOR_SIZE
#define SECTOR_COUNT         64   /* 4096 * 64 = 256KiB */

/*
 * Directory entry in FAT Volume
 * Note that FAT data is little endian
 */
typedef struct dir_ent {
  char     Name[11];
  uint8_t  Attr;
  uint8_t  NTRes;
  uint8_t  CrtTimeTenth;
  char     CrtTime[2];
  char     CrtDate[2];
  char     LastAccDate[2];
  uint16_t FstClusHI;
  char     WrtTime[2];
  char     WrtDate[2];
  uint16_t FstClusLO;
  uint32_t FileSize;
} DirEnt;

enum
{
  AUTORELOAD_WAIT = 0,
  AUTORELOAD_READY,
  AUTORELOAD_NONE
};

extern int autoreload_state;

void prk_msc_init(void);

void msc_findDirEnt(const char *filename, DirEnt *entry);

void msc_loadFile(uint8_t *file, DirEnt *entry);

