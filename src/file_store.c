#include "file_store.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FILE_STORE_PATH_MAX_LENGTH 4096

int file_store_list(
    const char *directory,
    file_entry_t *entries,
    size_t entries_capacity,
    size_t *entry_count
)
{
    DIR *dir = opendir(directory);

    if (dir == NULL) {
        return -1;
    }

    size_t count = 0;

    for (;;) {
        struct dirent *entry = readdir(dir);

        if (entry == NULL) {
            break;
        }

        if (
            strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0
        ) {
            continue;
        }

        if (count >= entries_capacity) {
            closedir(dir);
            return -1;
        }

        size_t name_length = strlen(entry->d_name);

        if (
            name_length >=
            sizeof(entries[count].name)
        ) {
            closedir(dir);
            return -1;
        }

        char path[FILE_STORE_PATH_MAX_LENGTH];

        int path_length = snprintf(
            path,
            sizeof(path),
            "%s/%s",
            directory,
            entry->d_name
        );

        if (
            path_length < 0 ||
            (size_t)path_length >= sizeof(path)
        ) {
            closedir(dir);
            return -1;
        }

        struct stat file_stat;

        if (stat(path, &file_stat) == -1) {
            closedir(dir);
            return -1;
        }

        memcpy(
            entries[count].name,
            entry->d_name,
            name_length + 1
        );

        entries[count].size =
            (size_t)file_stat.st_size;

        if (S_ISREG(file_stat.st_mode)) {
            entries[count].type =
                FILE_ENTRY_REGULAR;
        } else if (S_ISDIR(file_stat.st_mode)) {
            entries[count].type =
                FILE_ENTRY_DIRECTORY;
        } else {
            entries[count].type =
                FILE_ENTRY_UNKNOWN;
        }

        count++;
    }

    closedir(dir);

    *entry_count = count;

    return 0;
}

int file_store_filename_valid(
    const char *filename
)
{
    if (filename == NULL || filename[0] == '\0') {
        return 0;
    }

    if (
        strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0
    ) {
        return 0;
    }

    for (const char *p = filename; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            return 0;
        }
    }

    return 1;
}

int file_store_open(
    const char *directory,
    const char *filename,
    off_t *file_size
)
{
    if (!file_store_filename_valid(filename)) {
        return -1;
    }

    char path[FILE_STORE_PATH_MAX_LENGTH];

    int path_length = snprintf(
        path,
        sizeof(path),
        "%s/%s",
        directory,
        filename
    );

    if (
        path_length < 0 ||
        (size_t)path_length >= sizeof(path)
    ) {
        return -1;
    }

    int fd = open(
        path,
        O_RDONLY
    );

    if (fd == -1) {
        return -1;
    }

    struct stat file_stat;

    if (fstat(fd, &file_stat) == -1) {
        close(fd);
        return -1;
    }

    if (!S_ISREG(file_stat.st_mode)) {
        close(fd);
        return -1;
    }

    *file_size = file_stat.st_size;

    return fd;
}
