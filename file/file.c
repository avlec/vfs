#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "file.h"

struct file {
    vfs_t vfs;
    uint16_t inode_number;
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

struct page_map {
    uint16_t * pages;
    uint32_t page_count;
};

struct page_map build_page_map(vfs_t vfs, inode_t inode)
{
    struct page_map page_map = {};
    // count full pages, and add an extra page for an unfilled page.
    page_map.page_count = inode->file_size / 512 + ((inode->file_size % 512 == 0) ? 0 : 1);
    page_map.pages = calloc(page_map.page_count, sizeof(int16_t));

    printf("page map for %d pages\r\n", page_map.page_count);

    for(int i = 0; i < ((page_map.page_count < 10) ? page_map.page_count : 10); ++i)
    {
        page_map.pages[i] = inode->d_pages[i];
    }

    if (page_map.page_count > 10)
    {
        // TODO add all single indirect pages and double indirect pages
    }
    return page_map;
}

directory_t directory_open(vfs_t vfs, char * path)
{
    directory_t dir = (directory_t) malloc(sizeof(struct directory));
    dir->vfs = vfs;
    dir->path = (char *) calloc(strlen(path) + 1, sizeof(char));
    memcpy(dir->path, path, strlen(path));
    dir->name = (char *) calloc(31, sizeof(char));

    char * token = strtok(path, "/");
    uint16_t current_inode = 0;
    inode_t current_dir = vfs_get_inode(vfs, current_inode);
    struct page_map current_dir_page_map = build_page_map(vfs, current_dir);
    while(token != NULL)
    {
        memset(dir->name, 0, 31);
        memcpy(dir->name, token, strlen(token));

        // search in the pages of current_dir for
        // the entry matching token
        // if found update current dir
        free(current_dir_page_map.pages);
        current_dir_page_map = build_page_map(vfs, current_dir);

        token = strtok(NULL, "/");
    }
    free(current_dir_page_map.pages);

    dir->inode_number = current_inode;
    dir->inode = current_dir;
    return dir;
}

void directory_close(directory_t dir)
{
    dir->vfs = NULL;

    if(dir->inode != NULL)
        free(dir->inode);
    dir->inode = NULL;
    
    dir->inode_number = 0;

    if(dir->name != NULL)
        free(dir->name);
    dir->name = NULL;

    if(dir->path != NULL)
        free(dir->path);
    dir->path = NULL;

    free(dir);
}

void directory_add(directory_t dir, file_t file)
{
    struct page_map page_map = build_page_map(dir->vfs, dir->inode);

    // can we hold the entry in the number of pages we have?
    if (dir->inode->file_size < page_map.page_count * VFS_PAGE_SIZE)
    {
        // we got room
        if(fseek(dir->vfs->vdisk, page_map.pages[page_map.page_count - 1] * VFS_PAGE_SIZE + (dir->inode->file_size / 32) % 16, SEEK_SET) != 0)
        {
            ERR("fseek() result doesn't match requested.\r\n\t"
                "Exiting.");
            exit(EXIT_FAILURE);
        }

        if(fwrite(&file->inode_number, sizeof(file->inode_number), 1, dir->vfs->vdisk) != 1)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }

        if(fwrite(file->name, sizeof(*file->name), 30, dir->vfs->vdisk) != 1)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // we need room
        uint16_t new_page = vfs_allocate_new_page(dir->vfs);
        printf("KILL ME%d\r\n", page_map.pages[page_map.page_count - 1]);
        if(fseek(dir->vfs->vdisk, page_map.pages[page_map.page_count - 1] * VFS_PAGE_SIZE + (dir->inode->file_size / 32) % 16, SEEK_SET) != 0)
        {
            ERR("fseek() result doesn't match requested.\r\n\t"
                "Exiting.");
            exit(EXIT_FAILURE);
        }

        if(fwrite(&file->inode_number, sizeof(file->inode_number), 1, dir->vfs->vdisk) != 1)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }


        if(fwrite(file->name, sizeof(*file->name), 30, dir->vfs->vdisk) != 30)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }

        dir->inode->d_pages[dir->inode->file_size / 16] = new_page;
    }

    dir->inode->file_size += 32;
    vfs_update_inode(dir->vfs, dir->inode, dir->inode_number);
}

file_t file_create(vfs_t vfs, char * file_path)
{
    file_t new_file = (file_t) malloc(sizeof(struct file));
    new_file->vfs = vfs;
    // create and store new inode
    new_file->inode_number =  vfs_new_file_inode(vfs);
    // get inode
    new_file->inode = vfs_get_inode(vfs, new_file->inode_number);

    // parse filepath
    // build directory path
    // get filename
    size_t string_length = strlen(file_path) + 1;
    new_file->path = (char *) malloc(sizeof(char) * string_length);
    memcpy(new_file->path, file_path, string_length);

    // Initialize '\0' filled strings so we can copy to them and not worry about adding
    // a single '\0' at the end
    new_file->name = (char *) calloc(31, sizeof(char));
    char * directory_path = (char *) calloc(string_length, sizeof(char));


    // find index of last `/`
    int last_slash = -1;
    for(int i = 0; i < string_length - 1; ++i)
    {
        if (file_path[i] == '/') {
            // This ignores slashes with multiple `/`'s in a row
            if (i != 0)
                if (file_path[i - 1] == '/')
                    continue;
            if (i != string_length - 1)
                if (file_path[i + 1] == '/')
                    continue;
            last_slash = i + 1;
        }
    }

    memcpy(directory_path, file_path, last_slash);
    memcpy(new_file->name, file_path+last_slash, string_length - last_slash);

    printf("file_path %s, directory path %s, file_name %s\r\n", new_file->path, directory_path, new_file->name);

    // add directory entry
    directory_t dir = directory_open(vfs, directory_path);

    directory_add(dir, new_file);

    directory_close(dir);
    return new_file;
}

void file_open(vfs_t vfs, char * filepath)
{

}

void file_close(file_t file)
{
    file->vfs = NULL;

    if(file->name != NULL)
        free(file->name);
    file->name = NULL;

    file->inode_number = 0;

    if(file->inode != NULL)
        free(file->inode);
    file->inode = NULL;

    if(file->path != NULL)
        free(file->path);
    file->path = NULL;

    free(file);
}

int main() {
    vfs_t vfs = vfs_open("vdisk.img");

    file_close(file_create(vfs, "/avlec"));
    file_close(file_create(vfs, "/avlec"));

    vfs_close(vfs);
    return EXIT_SUCCESS;
}