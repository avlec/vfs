#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "disk.h"

void fseek_w(FILE * file, long int offset, int whence)
{
    if(fseek(file, offset, whence) != 0)
    {
        ERR("fseek() result doesn't match requested.\r\n\t"
            "Exiting.");
        exit(EXIT_FAILURE);
    }
}

void fread_w(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    if(fread(ptr, size, nmemb, stream) != nmemb) {
        ERR("fread() result doesn't match requested.\r\n\t"
            "Exiting.");
        exit(EXIT_FAILURE);
    }
}

void fwrite_w(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    if(fwrite(ptr, size, nmemb, stream) != nmemb) {
        ERR("fwrite() result doesn't match requested.\r\n\t"
            "Exiting.");
        exit(EXIT_FAILURE);
    }
}


int8_t vfs_page_free_check(vfs_t vfs, uint16_t page_number)
{
    // Create the bit mask
    uint8_t byte_mask =  0b10000000u >> page_number % 8;

    // Seek to free block vector location + offset
    uint16_t byte_offset = VFS_PAGE_SIZE + page_number / 8;
    // TODO check this is in proper range
    fseek_w(vfs->vdisk, byte_offset, SEEK_SET);

    uint8_t byte = 0;
    // Read a single byte from the file
    fread_w(&byte, 1, 1, vfs->vdisk);

    return byte & byte_mask;
}

/*
 * @brief: This function marks in the free block vector when a new page is used.
 *
 * @param vfs: file system which the operation executes on.
 * @param page_number: the page number which is being marked as non-free
 * @param marking_as_used: true  indicates bit is being set to 0
 *                      false indicates bit is being set to 1
 *
 * Macros with defines have been provided to make usage easier, and increase
 * readability.
 */
void vfs_page_free_modify(vfs_t vfs, uint16_t page_number, bool marking_as_used)
{
    uint8_t byte_mask = 0b10000000u >> page_number % 8u;

    // Seek to free block vector location + offset
    uint16_t byte_offset = VFS_PAGE_SIZE + page_number / 8;
    // TODO check this is in proper range
    fseek_w(vfs->vdisk, byte_offset, SEEK_SET);

    uint8_t byte;
    // Read a single byte from the file
    fread_w(&byte, 1, 1, vfs->vdisk);

    //byte = (marking_as_used ? (byte ^ byte_mask) : (byte | (uint8_t)~byte_mask));
    byte = byte ^ byte_mask;

    fseek_w(vfs->vdisk, byte_offset, SEEK_SET);

    fwrite_w(&byte, 1, 1, vfs->vdisk);
}

/*
 * @brief Writes the contents of the buffer to page page_number

void vfs_page_write(vfs_t vfs, uint16_t page_number, int8_t * buffer, int16_t buffer_size)
{
    // Buffer size checking.
    if(buffer_size > VFS_PAGE_SIZE)
    {
        ERR("Attempting to write more than 1 page worth at a time.\r\n\t"
            "Writing just one page.");
        buffer_size = 512;
    }

    if(fseek(vfs->vdisk, page_number * VFS_PAGE_SIZE, SEEK_SET) != 0)
    {
        ERR("fseek() result doesn't match requested.\r\n\t"
            "Exiting.");
        exit(EXIT_FAILURE);
    }
    if(fwrite(buffer, sizeof(*buffer), buffer_size, vfs->vdisk) != buffer_size)
    {
        ERR("fwrite() result doesn't match requested.\r\n\t"
            "Exiting.");
        exit(EXIT_FAILURE);
    }
}
 */

struct inode vfs_get_inode_page(vfs_t vfs, uint16_t page_number, uint16_t page_index)
{
    struct inode inode;
    // TODO check page_number and page_index is in proper range
    fseek_w(vfs->vdisk, page_number * VFS_PAGE_SIZE + page_index * 32, SEEK_SET);

    fread_w(&inode, sizeof(inode), 1, vfs->vdisk);
    return inode;
}

/*
 * This function adds inodes to pages. Assumes the page is allocated.
 */
void vfs_add_inode_page(vfs_t vfs, inode_t inode, uint16_t page_number, uint16_t page_index)
{
    // TODO check page_number and page_index is in proper range
    fseek_w(vfs->vdisk, page_number * VFS_PAGE_SIZE + page_index * 32, SEEK_SET);
    fwrite_w(inode, sizeof(*inode), 1, vfs->vdisk);
}


/*
 * @brief: this function allocates a new page, returns page number
 *
 * @param vfs: virtual file system of which to allocate a new page on.
 * @return: page number allocated.
 */
uint16_t vfs_allocate_new_page(vfs_t vfs)
{
    uint8_t fbv_contents[VFS_PAGE_SIZE];
    fseek_w(vfs->vdisk, VFS_FREE_BLOCK_VECTOR_PAGE_OFFSET, SEEK_SET);
    fread_w(fbv_contents, sizeof(*fbv_contents), sizeof(fbv_contents), vfs->vdisk);

    // Iterate through the bytes in the free block vector
    int first_free_blocks = 0;
    for(first_free_blocks = 0; first_free_blocks < VFS_PAGE_SIZE; ++first_free_blocks)
    {
        if (fbv_contents[first_free_blocks] != 0)
            break;
    }
    // Find where the free block is.
    uint8_t bit_offset = 0;
    for(bit_offset = 0; bit_offset < 8; ++bit_offset)
    {
        if((fbv_contents[first_free_blocks] & 0b10000000u >> bit_offset) != 0)
            break;
    }

    uint16_t allocated_page_index = first_free_blocks * 8 + bit_offset;

    // mark page as taken
    vfs_page_free_mark(vfs, allocated_page_index);

    // populate page with zeros
    fseek_w(vfs->vdisk, allocated_page_index * VFS_PAGE_SIZE, SEEK_SET);
    uint8_t zeros[VFS_PAGE_SIZE] = { 0xff };
    fwrite_w(zeros, sizeof(*zeros), sizeof(zeros), vfs->vdisk);


    return allocated_page_index;
}

void vfs_update_inode(vfs_t vfs, inode_t inode, uint16_t inode_number)
{
    // TODO this so it doesn't read initial inode data everytime.

    // Lookup dense index for page.
    fseek_w(vfs->vdisk, VFS_RESERVED_PAGES_START_OFFSET + (inode_number / 16) * 2, SEEK_SET);
    uint16_t page_number = 0;
    fread_w(&page_number, sizeof(page_number), 1, vfs->vdisk);

    vfs_add_inode_page(vfs, inode, page_number, inode_number % 16);
}

inode_t vfs_get_inode(vfs_t vfs, int16_t inode_number)
{
    inode_t inode = (inode_t) malloc(sizeof(struct inode));

    // query dense index.
    fseek_w(vfs->vdisk, VFS_RESERVED_PAGES_START_OFFSET + (inode_number / 16) * 2, SEEK_SET);
    uint16_t page_number = 0;
    fread_w(&page_number, sizeof(page_number), 1, vfs->vdisk);

    // visit page pointed to by dense index
    // go to inode offset on page
    *inode = vfs_get_inode_page(vfs, page_number, inode_number % 16);

    return inode;
}

uint16_t vfs_new_inode(vfs_t vfs, int32_t flags)
{
    struct inode new_inode = {
            .file_size = 0,
            .file_flags = flags,
            .d_pages = {},
            .si_page = 0,
            .di_page = 0
    };

    // Adding new page or adding to exisiting page?
    uint16_t page_index = vfs->inodes % 16;
    int page_number = 0;
    if(page_index == 0)
    {
        // Allocate new page for holding inodes.
        page_number = vfs_allocate_new_page(vfs);

        // Add dense index to page.
        fseek_w(vfs->vdisk, VFS_RESERVED_PAGES_START_OFFSET + (vfs->inodes / 16) * 2, SEEK_SET);
        fwrite_w(&page_number, sizeof(page_number), 1, vfs->vdisk);
    }
    else {
        // Lookup dense index for page.
        fseek_w(vfs->vdisk, VFS_RESERVED_PAGES_START_OFFSET + (vfs->inodes / 16) * 2, SEEK_SET);

        fread_w(&page_number, sizeof(page_number), 1, vfs->vdisk);
    }

    vfs_add_inode_page(vfs, &new_inode, page_number, page_index);


    return vfs->inodes++;
}

void vfs_create(vfs_t vfs)
{
    strcpy(vfs->magic_number, "vfs");
    vfs->pages = VFS_TOTAL_RESERVED_BLOCK_COUNT;
    vfs->inodes = 0;

    // Create & write super block
    {
        struct page super_block;
        memset(&super_block.bytes[0], 0x00, sizeof(super_block));
        memcpy(&super_block.bytes[0], &vfs->magic_number, sizeof("vfs"));
        memcpy(&super_block.bytes[4], &vfs->pages, sizeof(vfs->pages));
        memcpy(&super_block.bytes[8], &vfs->inodes, sizeof(vfs->inodes));
        fwrite(&super_block, sizeof(super_block), 1, vfs->vdisk);
    }

    // Create & write free block
    {
        struct page free_block;
        memset(&free_block, 0b11111111, sizeof(free_block));
        memset(&free_block, 0b00111111, 2);
        memset(&free_block, 0b00000000, 1);
        fwrite(&free_block, sizeof(free_block), 1, vfs->vdisk);
    }

    // Create reserved blocks
    {
        struct page reserved_blocks[8];
        memset(&reserved_blocks, 0, sizeof(reserved_blocks));
        fwrite(&reserved_blocks, sizeof(*reserved_blocks), sizeof(reserved_blocks)/sizeof(*reserved_blocks), vfs->vdisk);
    }

    vfs_new_inode(vfs, VFS_NEW_DIRECTORY_FLAGS);
}

vfs_t vfs_open(const char * vdisk)
{
    vfs_t new_vfs = (vfs_t) malloc(sizeof(struct vfs));

    new_vfs->vdisk = fopen(vdisk, "rb+");
    new_vfs->pages = 0;
    new_vfs->inodes = 0;

    // Check if file exists new_vfs->vdisk
    if(true) // TODO when done remove this.
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
