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

// Helper function to validate inputs before attempting to read
int validate_inputs(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
  if (!mounted) {
    return -3;  // System is not mounted
  }
  if (read_len == 0) {
    return 0;  // No bytes to read
  }
  if (read_len > 1024) {
    return -2;  // Read length exceeds limit
  }
  if (start_addr + read_len > JBOD_NUM_DISKS * JBOD_DISK_SIZE) {
    return -1;  // Address is out of bounds
  }
  if (read_buf == NULL && read_len > 0) {
    return -4;  // Invalid buffer
  }
  return 1;  // Inputs are valid
}

// Helper function to calculate disk, block, and offset
void calculate_disk_block_offset(uint32_t addr, uint32_t *disk_num, uint32_t *block_num, uint32_t *offset) {
  *disk_num = addr / JBOD_DISK_SIZE;               // Determine disk number
  *block_num = (addr / JBOD_BLOCK_SIZE) % JBOD_NUM_BLOCKS_PER_DISK;  // Determine block number
  *offset = addr % JBOD_BLOCK_SIZE;                // Calculate the offset within the block
}

// Helper function to perform seek operations
int seek_to_disk_block(uint32_t disk_num, uint32_t block_num) {
  uint32_t op = (JBOD_SEEK_TO_DISK << 12) | disk_num;  // Create the operation to seek to disk
  if (jbod_operation(op, NULL) != 0) {
    return -1;  // Disk seek failed
  }

  op = (JBOD_SEEK_TO_BLOCK << 12) | (block_num << 4);  // Create the operation to seek to block
  if (jbod_operation(op, NULL) != 0) {
    return -1;  // Block seek failed
  }

  return 0;  // Success
}

// Helper function to read the data from the block
int read_block_data(uint8_t *block_buf) {
  uint32_t op = (JBOD_READ_BLOCK << 12);  // Command to read block
  if (jbod_operation(op, block_buf) != 0) {
    return -1;  // Reading block failed
  }
  return 0;  // Success
}

// Helper function to copy the data from block to output buffer
void copy_data_to_buffer(uint8_t *read_buf, uint8_t *block_buf, uint32_t offset, uint32_t bytes_to_copy, uint32_t total_bytes_read) {
  for (uint32_t i = 0; i < bytes_to_copy; i++) {
    read_buf[total_bytes_read + i] = block_buf[offset + i];  // Copy data byte by byte
  }
}

// The main function to read data from JBOD
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf) {
  // Validate input parameters before proceeding
  int valid = validate_inputs(start_addr, read_len, read_buf);
  if (valid != 1) {
    return valid;  // Return error code if input validation fails
  }

  // Initialize necessary variables for the reading process
  uint32_t current_addr = start_addr;
  uint32_t bytes_remaining = read_len;
  uint32_t total_bytes_read = 0;
  uint8_t block_buf[JBOD_BLOCK_SIZE];

  // Loop to read the data in chunks of blocks
  while (bytes_remaining > 0) {
    uint32_t disk_num, block_num, offset;

    // Calculate the disk, block, and offset values based on the current address
    calculate_disk_block_offset(current_addr, &disk_num, &block_num, &offset);

    // Seek to the correct disk and block
    if (seek_to_disk_block(disk_num, block_num) != 0) {
      return -1;  // Seek operation failed
    }

    // Read the data from the current block
    if (read_block_data(block_buf) != 0) {
      return -1;  // Block read failed
    }

    // Determine how many bytes to copy from the block
    uint32_t bytes_to_copy = JBOD_BLOCK_SIZE - offset;
    if (bytes_remaining < bytes_to_copy) {
      bytes_to_copy = bytes_remaining;  // Adjust if fewer bytes are needed
    }

    // Copy the data to the output buffer
    copy_data_to_buffer(read_buf, block_buf, offset, bytes_to_copy, total_bytes_read);

    // Update counters and current address
    total_bytes_read += bytes_to_copy;
    current_addr += bytes_to_copy;
    bytes_remaining -= bytes_to_copy;

    // If the block or disk boundary is crossed, move to the next disk or block
    if (current_addr % JBOD_DISK_SIZE == 0 && bytes_remaining > 0) {
      continue;  // Cross disk boundary
    } else if (current_addr % JBOD_BLOCK_SIZE == 0 && bytes_remaining > 0) {
      continue;  // Cross block boundary
    }
  }

  // Return the total number of bytes read
  return total_bytes_read;
}