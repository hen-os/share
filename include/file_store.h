#ifndef MICROHTTPS_FILE_STORE_H
#define MICROHTTPS_FILE_STORE_H

#include <stddef.h>
#include <sys/types.h>

#define FILE_STORE_NAME_MAX_LENGTH 256

typedef enum {
    FILE_ENTRY_UNKNOWN = 0,
    FILE_ENTRY_REGULAR,
    FILE_ENTRY_DIRECTORY
} file_entry_type_t;

typedef struct {
    char name[FILE_STORE_NAME_MAX_LENGTH];
    size_t size;
    file_entry_type_t type;
} file_entry_t;

int file_store_list(
    const char *directory,
    file_entry_t *entries,
    size_t entries_capacity,
    size_t *entry_count
);

int file_store_open(
    const char *directory,
    const char *filename,
    off_t *file_size
);

int file_store_filename_valid(
    const char *filename
);

#endif
