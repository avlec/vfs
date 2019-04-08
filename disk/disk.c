#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"

struct vfs {
    FILE * vdisk;
    char magic_number[4];
    uint32_t pages;
    uint32_t inodes;
};
typedef struct vfs * vfs_t;

void vfs_add_inode(vfs_t vfs, inode_t inode)
{
    // check superblock inode count
    uint8_t inode_dense_i = vfs->inodes/16;
    // which one is it
    uint8_t inode_page_modulo = vfs->inodes % 16;
    // OFFSET 2 blocks, each index is 2 bytes, so multiply dense_i*2
    uint16_t inode_page_number = DISK_PAGE_SIZE * 2 + inode_dense_i*2;
    if((vfs->inodes % 16) == 0)
    {
        // new ref in dense index
    }
    // add in page number inode_page_number
    // will be the inode_page_modulo th position


    // find first spot in reserved sector
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
        memset(&freeblock, 0xFF, sizeof(freeblock));
        memset(&freeblock, 0, 11);
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
}