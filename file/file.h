#ifndef FILE_H
#define FILE_H

#include "../disk/disk.h"

struct page_map {
    uint16_t * pages;
    uint32_t page_count;
};
typedef struct page_map page_map;

struct file {
    vfs_t vfs;
    uint16_t inode_number;
    page_map pagemap;
    uint16_t cursor_page;
    uint16_t cursor_page_pos;
    inode_t inode;
    char * name;
    char * path;
};
typedef struct file * file_t;

struct directory {
    vfs_t vfs;
    uint16_t inode_number;
    inode_t inode;
    char * name;
    char * path;
};
typedef struct directory * directory_t;

directory_t directory_create(vfs_t vfs, char * directory_path);
directory_t directory_open(vfs_t vfs, char * directory_path);
void directory_close(directory_t dir);

file_t file_create(vfs_t vfs, char * file_path);
file_t file_open(vfs_t vfs, char * filepath);
size_t file_read(void * buffer, size_t elem_size, size_t num_elems, file_t file);
size_t file_write(void * buffer, size_t elem_size, size_t num_elems, file_t file);
#define VFS_SEEK_SET 0b00000001
#define VFS_SEEK_CUR 0b00000010
#define VFS_SEEK_END 0b00000100
size_t file_seek(file_t file, uint32_t offset, uint8_t mode);
size_t file_rewind(file_t avlec);
void file_close(file_t file);

#endif
