#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../file/file.h"

int main() {
    vfs_t vfs = vfs_open("vdisk.img");

    directory_close(directory_create(vfs, "/home"));
    file_close(file_create(vfs, "/home/avlec"));
    file_close(file_create(vfs, "/alec"));

    file_t avlec = file_open(vfs, "/home/avlec");

    char buffer1[512] = "this is garbage!";
    // 320 * 256 + 1
    int i = 0;
    for(i = 0; i < 32 * 10 + 32 * 256 + 32 * 256; ++i)
    {
        file_write(buffer1, sizeof(*buffer1), strlen(buffer1), avlec);
    }


    printf("pages %d\r\n", avlec->pagemap.page_count);

    /*
    file_rewind(avlec);

    char buffer2 [12] = {};
    file_read(buffer2, sizeof(*buffer2), sizeof(buffer2)-1, avlec);

    printf("file_contents: {\r\n%s\r\n}\r\n", buffer2);

     */

    file_close(avlec);

    vfs_close(vfs);
    return EXIT_SUCCESS;
}