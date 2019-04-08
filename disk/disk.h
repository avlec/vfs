#ifndef DISK_H
#define DISK_H

#include <stdint.h>

#define DISK_PAGE_SIZE 512


struct page {
    uint8_t bytes[DISK_PAGE_SIZE];
};

struct inode {
    uint32_t file_size;
    uint32_t file_flags;
    uint16_t d_pages[10];
    uint16_t si_pages;
    uint16_t di_pages;
};
typedef struct inode * inode_t;

#endif