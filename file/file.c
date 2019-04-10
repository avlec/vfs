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

directory_t directory_open(vfs_t vfs, char * path)
{

}

void directory_close(directory_t dir)
{
    dir->vfs = NULL;
    dir->inode_number = 0;
    free(dir->inode);
    free(dir->name);
    free(dir->path);
    free(dir);
}

file_t file_create(vfs_t vfs, char * file_path)
{
    file_t new_file = (file_t) malloc(sizeof(struct file));
    new_file->vfs = vfs;
    // create and store new inode
    //new_file->inode_number =  vfs_new_file_inode(vfs);
    // get inode
    //new_file->inode = vfs_get_inode(vfs, new_file->inode_number);

    // parse filepath
    // build directory path
    // (/.*/)*[^/]*
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

    printf("last slash %d, strlen %d\r\n", last_slash, string_length);

    memcpy(directory_path, file_path, last_slash);
    memcpy(new_file->name, file_path+last_slash, string_length - last_slash);

    printf("file_path %s, directory path %s, file_name %s\r\n", new_file->path, directory_path, new_file->name);

    // add directory entry
    directory_t dir = directory_open(vfs, directory_path);

    //directory_add(dir, new_file);

    directory_close(dir);
    return new_file;
}

void file_open(vfs_t vfs, char * filepath)
{

}

int main() {
    vfs_t vfs = vfs_open("vdisk.img");

    file_t file = file_create(vfs, "/home");

    return EXIT_SUCCESS;
}