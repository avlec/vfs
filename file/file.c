#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "file.h"

struct page_map build_page_map(inode_t inode)
{
    struct page_map page_map = {};
    // count full pages, and add an extra page for an unfilled page.
    page_map.page_count = inode->file_size / 512 + ((inode->file_size % 512 == 0) ? 0 : 1);
    page_map.pages = calloc(page_map.page_count, sizeof(int16_t));

    //printf("page map for %d pages\r\n", page_map.page_count);

    for(int i = 0; i < ((page_map.page_count < 10) ? page_map.page_count : 10); ++i)
    {
        page_map.pages[i] = inode->d_pages[i];
    }

    if (page_map.page_count > 10)
    {
        // TODO add all single indirect pages and double indirect page

        if (inode->si_page != 0)
        {
            // read in si page.
            for(uint16_t d_page = 0; d_page < 256; ++d_page)
            {
                // add each direct page number (if it's not zero)
            }

        }

        if (inode->di_page != 0) {
            // read in di_page
            for (uint16_t si_page = 0; si_page < 256; ++si_page)
            {
                // read each si page
                for (uint16_t d_page = 0; d_page < 256; ++d_page)
                {
                    // add each direct page number (if it's not zero)
                }
            }
        }
        // read double indirect page.
        // read each page of single indirect pages from double indirect.
        // read direct page number from all single indirect pages.
    }
    return page_map;
}

uint16_t directory_get_inode_number(directory_t dir, char *entry_name)
{
    // create a page map.
    struct page_map pagemap = build_page_map(dir->inode);

    for(int i = 0; i < pagemap.page_count; ++i) {
        if(fseek(dir->vfs->vdisk, pagemap.pages[i] * VFS_PAGE_SIZE, SEEK_SET) != 0)
        {
            ERR("fseek() result doesn't match requested.\r\n\t"
                "Exiting.");
            exit(EXIT_FAILURE);
        }

        for(int j = 0; j < 16; ++j) {
            uint16_t read_inode = 0;
            char  name[31] = {};
            if(fread(&read_inode, sizeof(read_inode), 1, dir->vfs->vdisk) != 1)
            {
                ERR("fread() result doesn't match requested.\r\n\t"
                    "Exiting.");
                exit(EXIT_FAILURE);
            }
            if(fread(name, sizeof(*name), sizeof(name), dir->vfs->vdisk) != sizeof(name))
            {
                ERR("fread() result doesn't match requested.\r\n\t"
                    "Exiting.");
                exit(EXIT_FAILURE);
            }

            if(strncmp(entry_name, name, sizeof(name)-1) == 0) {
                return read_inode;
            }
        }
    }
    return 0;
}


directory_t directory_open(vfs_t vfs, char * directory_path)
{
    directory_t dir = (directory_t) malloc(sizeof(struct directory));
    // Stuff that doesn't change with loop.
    dir->vfs = vfs;
    dir->path = (char *) calloc(strlen(directory_path) + 1, sizeof(char));
    memcpy(dir->path, directory_path, strlen(directory_path));
    dir->name = (char *) calloc(31, sizeof(char));

    // Stuff that changes with loop.
    char * token = strtok(directory_path, "/");
    uint16_t current_inode = 0;
    inode_t current_dir = vfs_get_inode(vfs, current_inode);
    struct page_map current_dir_page_map = build_page_map(current_dir);
    while(token != NULL)
    {
        memset(dir->name, 0, 31);
        memcpy(dir->name, token, strlen(token));

        bool found = false;

        // search in the pages of current_dir for
        for(int i = 0; i < current_dir_page_map.page_count && !found; ++i) {
            // Read page at pages[i]
            if (fseek(vfs->vdisk, current_dir_page_map.pages[i] * VFS_PAGE_SIZE, SEEK_SET) != 0)
            {
                ERR("fseek() result doesn't match requested.\r\n\t"
                    "Exiting.");
                exit(EXIT_FAILURE);
            }

            // read each entry in the page.
            for(int j = 0; j < 16 && !found; ++j)
            {
                uint16_t read_inode = 0;
                char  name[31] = {};
                if(fread(&read_inode, sizeof(read_inode), 1, vfs->vdisk) != 1)
                {
                    ERR("fread() result doesn't match requested.\r\n\t"
                        "Exiting.");
                    exit(EXIT_FAILURE);
                }
                if(fread(name, sizeof(*name), sizeof(name)-1, vfs->vdisk) != sizeof(name) - 1)
                {
                    ERR("fread() result doesn't match requested.\r\n\t"
                        "Exiting.");
                    exit(EXIT_FAILURE);
                }

                // compare name with token
                if(strncmp(token, name, strlen(token)) == 0) {
                    found = true;
                    current_inode = read_inode;
                    current_dir = vfs_get_inode(vfs, current_inode);
                    free(current_dir_page_map.pages);
                    current_dir_page_map = build_page_map(current_dir);
                }
            }
        }
        // the entry matching token
        // if found update current dir

        token = strtok(NULL, "/");
    }

    dir->inode_number = current_inode;
    dir->inode = current_dir;

    free(current_dir_page_map.pages);
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

void directory_add_directory(directory_t parent_dir, directory_t dir)
{
    struct page_map page_map = build_page_map(parent_dir->inode);

    // can we hold the entry in the number of pages we have?
    if (parent_dir->inode->file_size < page_map.page_count * VFS_PAGE_SIZE)
    {
        // we got room
        // write the directory entry with the inode number and file name
        if(fseek(parent_dir->vfs->vdisk, page_map.pages[page_map.page_count - 1] * VFS_PAGE_SIZE + parent_dir->inode->file_size, SEEK_SET) != 0)
        {
            ERR("fseek() result doesn't match requested.\r\n\t"
                "Exiting.");
            exit(EXIT_FAILURE);
        }
        if(fwrite(&dir->inode_number, sizeof(dir->inode_number), 1, parent_dir->vfs->vdisk) != 1)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }
        if(fwrite(dir->name, sizeof(*dir->name), 30, parent_dir->vfs->vdisk) != 30)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // we need room
        uint16_t new_page = vfs_allocate_new_page(parent_dir->vfs);

        if(fseek(parent_dir->vfs->vdisk, new_page * VFS_PAGE_SIZE + (parent_dir->inode->file_size / 32) % 16, SEEK_SET) != 0)
        {
            ERR("fseek() result doesn't match requested.\r\n\t"
                "Exiting.");
            exit(EXIT_FAILURE);
        }

        if(fwrite(&dir->inode_number, sizeof(dir->inode_number), 1, parent_dir->vfs->vdisk) != 1)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }


        if(fwrite(dir->name, sizeof(*dir->name), 30, parent_dir->vfs->vdisk) != 30)
        {
            ERR("fwrite() result doesn't match requested.\r\n\t"
                "Exiting");
            exit(EXIT_FAILURE);
        }

        parent_dir->inode->d_pages[parent_dir->inode->file_size / 16] = new_page;
    }

    parent_dir->inode->file_size += 32;
    vfs_update_inode(parent_dir->vfs, parent_dir->inode, parent_dir->inode_number);
}

directory_t directory_create(vfs_t vfs, char * directory_path)
{
    directory_t dir = (directory_t) malloc(sizeof(struct directory));
    dir->vfs = vfs;
    // create and store new inode
    dir->inode_number =  vfs_new_dir_inode(vfs);
    // get inode
    dir->inode = vfs_get_inode(vfs, dir->inode_number);

    // parse filepath
    // build directory path
    // get filename
    size_t string_length = strlen(directory_path) + 1;
    dir->path = (char *) malloc(sizeof(char) * string_length);
    memcpy(dir->path, directory_path, string_length);

    // Initialize '\0' filled strings so we can copy to them and not worry about adding
    // a single '\0' at the end
    dir->name = (char *) calloc(31, sizeof(char));
    char * absolute_path = (char *) calloc(string_length, sizeof(char));


    // find index of last `/`
    int last_slash = -1;
    for(int i = 0; i < string_length - 1; ++i)
    {
        if (directory_path[i] == '/') {
            // This ignores slashes with multiple `/`'s in a row
            if (i != 0)
                if (directory_path[i - 1] == '/')
                    continue;
            if (i != string_length - 1)
                if (directory_path[i + 1] == '/')
                    continue;
            last_slash = i + 1;
        }
    }

    memcpy(absolute_path, directory_path, last_slash);
    memcpy(dir->name, directory_path+last_slash, string_length - last_slash);

    printf("strlen %d, last slash %d\r\n", string_length, last_slash);
    printf("file_path %s, directory path %s, file_name %s\r\n", dir->path, absolute_path, dir->name);

    // add directory entry
    directory_t parent_dir = directory_open(vfs, absolute_path);

    directory_add_directory(parent_dir, dir);

    directory_close(parent_dir);

    return dir;
}

void directory_add_file(directory_t dir, file_t file)
{
    struct page_map page_map = build_page_map(dir->inode);

    // can we hold the entry in the number of pages we have?
    if (dir->inode->file_size < page_map.page_count * VFS_PAGE_SIZE)
    {
        // we got room
        // write the directory entry with the inode number and file name
        if(fseek(dir->vfs->vdisk, page_map.pages[page_map.page_count - 1] * VFS_PAGE_SIZE + dir->inode->file_size, SEEK_SET) != 0)
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
    }
    else
    {
        // we need room
        uint16_t new_page = vfs_allocate_new_page(dir->vfs);

        if(fseek(dir->vfs->vdisk, new_page * VFS_PAGE_SIZE + (dir->inode->file_size / 32) % 16, SEEK_SET) != 0)
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
    // Reset file cursor
    new_file->pagemap = build_page_map(new_file->inode);
    new_file->cursor_page = 0;
    new_file->cursor_page_pos = 0;

    // parse filepath
    // build directory path
    // get filename
    size_t string_length = strlen(file_path) + 1;
    new_file->path = (char *) malloc(sizeof(char) * string_length);
    memcpy(new_file->path, file_path, string_length);

    // Initialize '\0' filled strings so we can copy to them and not worry about adding
    // a single '\0' at the end
    new_file->name = (char *) calloc(31, sizeof(char));
    char * absolute_path = (char *) calloc(string_length, sizeof(char));


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

    memcpy(absolute_path, file_path, last_slash);
    memcpy(new_file->name, file_path+last_slash, string_length - last_slash);

    printf("file_path %s, directory path %s, file_name %s\r\n", new_file->path, absolute_path, new_file->name);

    // add directory entry
    directory_t parent_dir = directory_open(vfs, absolute_path);

    directory_add_file(parent_dir, new_file);

    directory_close(parent_dir);
    return new_file;
}

file_t file_open(vfs_t vfs, char * file_path)
{
    file_t file = (file_t) calloc(1, sizeof(struct file));
    file->vfs = vfs;

    // Find the file to get inode_number
    // use inode number to get inode
    // use inode to build pagemap

    // parse filepath
    // build directory path
    // get filename
    size_t string_length = strlen(file_path) + 1;
    file->path = (char *) malloc(sizeof(char) * string_length);
    memcpy(file->path, file_path, string_length);

    // Initialize '\0' filled strings so we can copy to them and not worry about adding
    // a single '\0' at the end
    file->name = (char *) calloc(31, sizeof(char));
    char * absolute_path = (char *) calloc(string_length, sizeof(char));


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

    memcpy(absolute_path, file_path, last_slash);
    memcpy(file->name, file_path+last_slash, string_length - last_slash);

    printf("file_path %s, directory path %s, file_name %s\r\n", file->path, absolute_path, file->name);

    directory_t parent_dir = directory_open(vfs, absolute_path);

    file->inode_number = directory_get_inode_number(parent_dir, file->name);

    directory_close(parent_dir);

    if(file->inode_number == 0)
    {
        file_close(file);
        return NULL;
    }

    file->inode = vfs_get_inode(vfs, file->inode_number);
    file->pagemap = build_page_map(file->inode);
    file->cursor_page = 0;
    file->cursor_page_pos = 0;

    return file;
}

void file_close(file_t file)
{
    file->vfs = NULL;

    if(file->name != NULL)
        free(file->name);
    file->name = NULL;

    file->pagemap.page_count = 0;
    if(file->pagemap.pages != NULL)
        free(file->pagemap.pages);
    file->cursor_page_pos = 0;
    file->cursor_page = 0;

    file->inode_number = 0;

    if(file->inode != NULL)
        free(file->inode);
    file->inode = NULL;

    if(file->path != NULL)
        free(file->path);
    file->path = NULL;

    free(file);
}

size_t file_write(void * buffer, size_t elem_size, size_t num_elems, file_t file)
{
    // assuming only concatenation to files.
    // every write always allocates new blocks on the end
    return 0;
}

size_t file_read(void * buffer, size_t elem_size, size_t num_elems, file_t file)
{
    return 0;
}