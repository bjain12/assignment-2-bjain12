#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"


static int mounted = 0; // Not mounted

int mdadm_mount(void) {
  // Complete your code here
  
  if (mounted) {
    return -1; // Return -1 if it is not mounted
  }

 uint32_t op = JBOD_MOUNT << 12;  // Prepare the operation (mount) by shifting it to 12 bits to the left to fit.
  if (jbod_operation(op, NULL) == 0) {
    mounted = 1;  // Set mounted to true, operation was successfull in mounting
    return 1;     // Return 1 on successful mount
  }

  return -1;  // Return -1 if the operation fails
}

int mdadm_unmount(void) {
  //Complete your code here
  
  if (!mounted) {
    return -1;  // Return -1 if not mounted
  }

  uint32_t op = JBOD_UNMOUNT << 12;  // Prepare the operation (unmount) my moving 12 bits to left
  if (jbod_operation(op, NULL) == 0) { // If the system is not mounted (mounted == 0), it returns -1
    mounted = 0;  // Set mounted to false, succesfull in unmounting
    return 1;     // Return 1 on successful unmount
  }

  return -1;  // Return -1 if the operation fails
}

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
  // Verify system is mounted and all input parameters are valid
  if (!mounted) {
    return -3;  // System is not mounted
  }
  if (read_len > 1024) {
    return -2;  // Exceeding read length limit
  }
  if (start_addr + read_len > JBOD_NUM_DISKS * JBOD_DISK_SIZE) {
    return -1;  // Address range out of bounds
  }
  if (read_buf == NULL && read_len > 0) {
    return -4;  // Invalid buffer pointer
  }
  if (read_len == 0) {
    return 0;  // Nothing to read
  }

  // Variables to track progress through the read operation
  uint32_t addr = start_addr;
  uint32_t bytes_left = read_len;
  uint32_t total_bytes_read = 0;
  uint8_t temp_block[JBOD_BLOCK_SIZE];

  // Loop through until we've read all requested bytes
  while (bytes_left > 0) {
    // Calculate which disk, block, and offset to read from, based on current address
    uint32_t disk_num = addr / JBOD_DISK_SIZE;  // Disk that holds the current address
    uint32_t block_num = (addr / JBOD_BLOCK_SIZE) & (JBOD_NUM_BLOCKS_PER_DISK - 1);  // Block within disk
    uint32_t offset = addr & (JBOD_BLOCK_SIZE - 1);  // Offset within the block

    // Log status for debugging (use bitwise for address calculations, making logic different)
    printf("Disk: %u, Block: %u, Offset: %u, Addr: %u, Remaining Bytes: %u\n", disk_num, block_num, offset, addr, bytes_left);
    fflush(stdout);

    // Issue disk seek command to position the virtual drive to the correct disk
    if (jbod_operation((JBOD_SEEK_TO_DISK << 12) | disk_num, NULL) != 0) {
      return -1;  // Return on failure
    }

    // Issue block seek command to position the drive to the correct block
    if (jbod_operation((JBOD_SEEK_TO_BLOCK << 12) | (block_num << 4), NULL) != 0) {
      return -1;  // Return on failure
    }

    // Read the block content into the temp_block buffer
    if (jbod_operation((JBOD_READ_BLOCK << 12), temp_block) != 0) {
      return -1;  // Return on failure
    }

    // Log part of the block content for verification (show only first 4 bytes to reduce logging clutter)
    printf("Block start (first 4 bytes): 0x%02x 0x%02x 0x%02x 0x%02x\n", temp_block[0], temp_block[1], temp_block[2], temp_block[3]);
    fflush(stdout);

    // Determine how much data to copy from the block, respecting block and buffer limits
    uint32_t bytes_to_copy = JBOD_BLOCK_SIZE - offset; // Available bytes in the current block
    if (bytes_to_copy > bytes_left) {
      bytes_to_copy = bytes_left;  // Only copy as many bytes as are left to read
    }

    // Copy the relevant part of the block to the output buffer
    memcpy(read_buf + total_bytes_read, temp_block + offset, bytes_to_copy);

    // Update counters and variables after copying
    total_bytes_read += bytes_to_copy;
    bytes_left -= bytes_to_copy;
    addr += bytes_to_copy;

    // Handle transitions: logging the disk/block boundary cases
    if (addr % JBOD_DISK_SIZE == 0 && bytes_left > 0) {
            // Cross to the next disk if we're at the end of the current disk
      printf("Switching to disk %u from disk %u\n", disk_num + 1, disk_num);
    } else if (addr % JBOD_BLOCK_SIZE == 0 && bytes_left > 0) {
            // Cross to the next block if we're at the end of the current block
      printf("Advancing to next block on disk %u\n", disk_num);
    }
  }

  return total_bytes_read;  // Return the total bytes successfully read
}
