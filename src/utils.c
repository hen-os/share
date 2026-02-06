#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
#else
    #define MKDIR(dir) mkdir(dir, 0777)
#endif

int create_folder(char* dirname, char* path) {
    int check;
    check = MKDIR(dirname);

    if (check == 0) {
        printf("Directory '%s' created successfully.\n", dirname);
    } else {
        if (errno == EEXIST) {
            printf("Directory '%s' already exists.\n", dirname);
        } else {
            perror("Unable to create directory");
            return 1;
        }
    }

    return 0;
}

int create_file(char* filename, char* path) {
}
