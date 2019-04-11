#include <stdlib.h>
#include <stdio.h>

#include "../file/file.h"

int main() {
    vfs_t vfs = vfs_open("vdisk.img");

    directory_close(directory_create(vfs, "/home"));
    file_close(file_create(vfs, "/home/avlec"));
    file_close(file_create(vfs, "/alec"));

    file_t avlec = file_open(vfs, "/home/avlec");

    char buffer1[512] = "this is garbage";
    file_write(buffer1, sizeof(*buffer1), sizeof(buffer1)/sizeof(*buffer1), avlec);

    file_rewind(avlec);

    char buffer2 [512] = {};
    file_read(buffer2, sizeof(*buffer2), sizeof(buffer2)/sizeof(*buffer2), avlec);

    file_close(avlec);

    vfs_close(vfs);
    return EXIT_SUCCESS;
}