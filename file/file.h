#ifndef FILE_H
#define FILE_H

#include "../disk/disk.h"

struct page_map {
    uint16_t * pages;
    uint32_t page_count;
};
typedef struct page_map * page_map_t;

struct file {
    vfs_t vfs;
    uint16_t inode_number;
    page_map_t page_map;
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
size_t file_read(void * dest, size_t size, size_t count, file_t file);
size_t file_write(void * src, size_t size, size_t count, file_t file);
void file_close(file_t file);

#endif
