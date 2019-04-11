#include <stdlib.h>
#include <stdio.h>

#include "../file/file.h"

int main() {
    vfs_t vfs = vfs_open("vdisk.img");

    directory_close(directory_create(vfs, "/home"));
    file_close(file_create(vfs, "/home/avlec"));
    file_close(file_create(vfs, "/alec"));

    file_t avlec = file_open(vfs, "/home/avlec");

    file_write("this is garbage", sizeof(char), sizeof("this is garbage"), avlec);

    file_close(avlec);

    vfs_close(vfs);
    return EXIT_SUCCESS;
}