#ifndef JBOD_H_
#define JBOD_H_
#include <stdint.h>

// Fill the below values with right values
#define JBOD_NUM_DISKS            16 
#define JBOD_DISK_SIZE            65536 
#define JBOD_BLOCK_SIZE           256 
#define JBOD_NUM_BLOCKS_PER_DISK  256

typedef enum {
  JBOD_MOUNT,
  JBOD_UNMOUNT,
  JBOD_SEEK_TO_DISK,
  JBOD_SEEK_TO_BLOCK,
  JBOD_READ_BLOCK,
  JBOD_WRITE_BLOCK,
  JBOD_SIGN_BLOCK,
  JBOD_NUM_CMDS,
} jbod_cmd_t;

typedef enum {
  JBOD_NO_ERROR,
  JBOD_UNMOUNTED,
  JBOD_ALREADY_MOUNTED,
  JBOD_ALREADY_UNMOUNTED,
  JBOD_CACHELOAD_FAIL,
  JBOD_CACHEWRITE_FAIL,
  JBOD_BAD_CMD,
  JBOD_BAD_DISK_NUM,
  JBOD_BAD_BLOCK_NUM,
  JBOD_BAD_READ,
  JBOD_BAD_WRITE,
  JBOD_NUM_ERRNOS,
} jbod_error_t;

int jbod_operation(uint32_t op, uint8_t *block);

extern jbod_error_t jbod_error;
const char *jbod_error_string(int eno);

#endif

/*
Given: 

Total Storage = 1 MB = 1048576 bytes
Each block = 256 bytes 

Number of blocks per disk and number of disk: 

Number of disks (j) * Number of blocks per disk (i) => i * j * 256 = 1 MB = 1048576 bytes
i * j =  (1048576 bytes/ 256 bytes) = 4096 : Product of number of disks and number of blocks per disk
Combination satisfying these values: 16 = j, 256 = i 

Disk size: 
Disk size = i * 256 = 256 * 256 = 65536 bytes
1024 bytes = 1 KB, so  65536 bytes = 65536/1024 = 64 KB

*/ 
