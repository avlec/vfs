#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "disk.h"

struct vfs {
    FILE * vdisk;
    char magic_number[4];
    uint32_t pages;
    uint32_t inodes;
};
typedef struct vfs * vfs_t;

#define ERR(x) fprintf(stderr, "Error in %s at line %d in %s:\r\n\t %s\r\n", __func__, __LINE__, __FILE__, x)

int8_t vfs_page_freecheck(vfs_t vfs, uint16_t page_number)
{
    uint8_t byte_mask =  0b10000000 >> page_number % 8;

    // Seek to freeblock vector location + offset
    uint16_t byte_offset = DISK_PAGE_SIZE + page_number / 8;
    if(fseek(vfs->vdisk, byte_offset, SEEK_SET) != 0)
    {
        ERR("fseek() result doesn't match requested."
            "Exiting.");
    }
    uint8_t byte;
    // Read a single byte from the file
    if(fread(&byte, 1, 1, vfs->vdisk) != 1) {
        ERR("fread() result doesn't match requested."
            "Exiting.");
        exit(EXIT_FAILURE);
    }

    return byte & byte_mask;
}

void vfs_page_freemark(vfs_t vfs, uint16_t page_number)
{
    uint8_t byte_mask =  0b10000000 >> page_number % 8;

    // Seek to freeblock vector location + offset
    uint16_t byte_offset = DISK_PAGE_SIZE+page_number/8;
    if(fseek(vfs->vdisk, byte_offset, SEEK_SET) != 0)
    {
        ERR("fseek() result doesn't match requested."
            "Exiting.");
    }
    uint8_t byte;
    // Read a single byte from the file
    if(fread(&byte, 1, 1, vfs->vdisk) != 1) {
        ERR("fread() result doesn't match requested."
            "Exiting");
        exit(EXIT_FAILURE);
    }

    byte = byte | byte_mask;

    if(fwrite(&byte, 1, 1, vfs->vdisk) != 1)
    {
        ERR("fwrite() result doesn't match requested."
            "Exiting.");
        exit(EXIT_FAILURE);
    }
}

void vfs_page_write(vfs_t vfs, uint16_t page_number, int8_t * buffer, int16_t buffer_size)
{
    if(fseek(vfs->vdisk, page_number * DISK_PAGE_SIZE, SEEK_SET) != 0)
    {
        ERR("fseek() result doesn't match requested."
            "Exiting.");
        exit(EXIT_FAILURE);
    }
    if(fwrite(buffer, 1, buffer_size, vfs->vdisk) != buffer_size)
    {
        ERR("fwrite() result doesn't match requested."
            "Exiting.");
        exit(EXIT_FAILURE);
    }

}

/*
 * This function adds inodes to existing inode pages
 *  and creates new inode pages when the freeblockvector says they're empty
 *  and page_index is zero
 */
void vfs_add_inode_page(vfs_t vfs, inode_t inode, uint16_t page_number, uint16_t page_index)
{
    if(vfs_page_freecheck(vfs, page_number))
    {
        // non-zero indicates not taken
        if(page_index != 0){
            ERR("Page not taken, and page_index non-zero."
                "Exiting");
            exit(EXIT_FAILURE);
        }

        // get me a new page.
        vfs_page_freemark(vfs, page_number);

        vfs_page_write(vfs, page_number, inode, sizeof(*inode));
    }
    else
    {
        // 0 indicates taken..
        if(page_index == 0)
        {
            ERR("Page taken, and page_index zero."
                "Exiting.");
            exit(EXIT_FAILURE);
        }

        // check to make sure the index isn't being taken by another inode.
        // if it's free good,
        // if not exit_failure
    }
}

void vfs_add_inode(vfs_t vfs, inode_t inode)
{
    // What page, and what position in that page is the inode being added to
    uint16_t inode_page = vfs->inodes / 16;
    uint16_t inode_page_index = vfs->inodes % 16;

    // Adds inode to a page, and accounts if a new page is required.
    vfs_add_inode_page(vfs, inode,  10+inode_page, inode_page_index);

    if(inode_page_index == 0)
    {
        // add to dense index;

    }

}

void vfs_create(vfs_t vfs)
{
    strcpy(vfs->magic_number, "vfs");
    vfs->pages = 10;
    vfs->inodes = 0;

    // Create & write superblock
    {
        struct page superblock;
        memset(&superblock.bytes[0], 0x00, sizeof(superblock));
        memcpy(&superblock.bytes[0], &vfs->magic_number, sizeof("vfs"));
        memcpy(&superblock.bytes[4], &vfs->pages, sizeof(vfs->pages));
        memcpy(&superblock.bytes[8], &vfs->inodes, sizeof(vfs->inodes));
        fwrite(&superblock, sizeof(superblock), 1, vfs->vdisk);
    }

    // Create & write freeblock
    {
        struct page freeblock;
        memset(&freeblock, 0b11111111, sizeof(freeblock));
        memset(&freeblock, 0b00111111, 2);
        memset(&freeblock, 0b00000000, 1);
        fwrite(&freeblock, sizeof(freeblock), 1, vfs->vdisk);
    }

    // Create reserved blocks
    {
        struct page reservedblocks[8];
        memset(&reservedblocks, 0, sizeof(reservedblocks));
        fwrite(&reservedblocks, sizeof(*reservedblocks), sizeof(reservedblocks)/sizeof(*reservedblocks), vfs->vdisk);
    }

    struct inode rootnode = {
            .file_size = 0,
            .file_flags = 0,
            .d_pages = { 0 },
            .si_pages = 0,
            .di_pages = 0
    };
    vfs_add_inode(vfs, &rootnode);
}

vfs_t vfs_open(const char * vdisk)
{
    vfs_t new_vfs = (vfs_t) malloc(sizeof(struct vfs));

    new_vfs->vdisk = fopen(vdisk, "rb+");
    new_vfs->pages = 0;
    new_vfs->inodes = 0;

    // Check if file exists new_vfs->vdisk
    if(0 == 0)
    {
        printf("Disk doesn't exist. Creating blank disk %s\r\n", vdisk);
        new_vfs->vdisk = fopen(vdisk, "wb+");

        vfs_create(new_vfs);
    }

    printf("Opened disk %s\r\n", vdisk);

    return new_vfs;
}

void vfs_close(vfs_t vfs)
{
    fclose(vfs->vdisk);
    free(vfs);
}

int main() {
    vfs_t vfs = vfs_open("vdisk.img");



    vfs_close(vfs);
    exit(EXIT_SUCCESS);
}
