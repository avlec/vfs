#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define VFS_PAGE_SIZE 512

#define VFS_SUPER_BLOCK_PAGE_COUNT 1
#define VFS_SUPER_BLOCK_PAGE 0
#define VFS_SUPER_BLOCK_PAGE_OFFSET (VFS_SUPER_BLOCK_PAGE * VFS_PAGE_SIZE)

#define VFS_FREE_BLOCK_VECTOR_COUNT 1
#define VFS_FREE_BLOCK_VECTOR_PAGE (VFS_SUPER_BLOCK_PAGE_COUNT)
#define VFS_FREE_BLOCK_VECTOR_PAGE_OFFSET (VFS_FREE_BLOCK_VECTOR_PAGE * VFS_PAGE_SIZE)


#define VFS_RESERVED_BLOCK_COUNT 8
#define VFS_RESERVED_PAGES_START (VFS_SUPER_BLOCK_PAGE_COUNT + VFS_FREE_BLOCK_VECTOR_COUNT)
#define VFS_RESERVED_PAGES_START_OFFSET (VFS_RESERVED_PAGES_START * VFS_PAGE_SIZE)
#define VFS_RESERVED_PAGES_END (VFS_RESERVED_PAGES_START + VFS_RESERVED_BLOCK_COUNT - 1)
#define VFS_RESERVED_PAGES_END_OFFSET (VFS_RESERVED_PAGES_END * VFS_PAGE_SIZE)

#define VFS_TOTAL_RESERVED_BLOCK_COUNT (VFS_SUPER_BLOCK_PAGE_COUNT  +\
                                        VFS_FREE_BLOCK_VECTOR_COUNT +\
                                        VFS_RESERVED_BLOCK_COUNT)

#define VFS_DATA_START_BLOCK (VFS_TOTAL_RESERVED_BLOCK_COUNT)
#define VFS_PAGE_START_OFFSET (VFS_RESERVED_BLOCK_COUNT * VFS_PAGE_SIZE)

#define ERR(x) fprintf(stderr, "Error in %s at line %d in %s:\r\n\t%s\r\n", __func__, __LINE__, __FILE__, x)

struct vfs {
    FILE * vdisk;
    char magic_number[4];
    uint32_t pages;
    uint32_t inodes;
};
typedef struct vfs * vfs_t;

struct page {
    uint8_t bytes[VFS_PAGE_SIZE];
};

#define VFS_NEW_FILE_FLAGS      0x40000000
#define VFS_NEW_DIRECTORY_FLAGS 0x80000000
#define VFS_ERROR_FLAGS         0xFFFFFFFF

struct inode {
    uint32_t file_size;
    uint32_t file_flags;
    uint16_t d_pages[10];
    uint16_t si_page;
    uint16_t di_page;
};
typedef struct inode * inode_t;

void vfs_page_free_modify(vfs_t vfs, uint16_t page_number, bool marking_as_used);
static inline void vfs_page_free_mark(vfs_t vfs, uint16_t page_number)
{
    vfs_page_free_modify(vfs, page_number, true);
}
static inline void vfs_page_free_unmark(vfs_t vfs, uint16_t page_number)
{
    vfs_page_free_modify(vfs, page_number, false);
}

void vfs_page_write(vfs_t vfs, uint16_t page_number, int8_t * buffer, int16_t buffer_size);

void vfs_add_inode_page(vfs_t vfs, inode_t inode, uint16_t page_number, uint16_t page_index);

uint16_t vfs_allocate_new_page(vfs_t vfs);

void vfs_update_inode(vfs_t vfs, inode_t inode, uint16_t inode_number);

inode_t vfs_get_inode(vfs_t vfs, int16_t inode_number);

uint16_t vfs_new_inode(vfs_t vfs, int32_t flags);

static inline uint16_t vfs_new_file_inode(vfs_t vfs)
{
    return vfs_new_inode(vfs, VFS_NEW_FILE_FLAGS);
}

static inline uint16_t vfs_new_dir_inode(vfs_t vfs)
{
    return vfs_new_inode(vfs, VFS_NEW_DIRECTORY_FLAGS);
}

void vfs_create(vfs_t vfs);

vfs_t vfs_open(const char * vdisk);

void vfs_close(vfs_t vfs);

#endif