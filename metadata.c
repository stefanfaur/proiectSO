#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct metadata_t {
    char file_name[40];
    int height;
    int width;
    int size;
    int user_id;
    char last_modified[11];
    int link_count;
    char user_access[4];
    char group_access[4];
    char others_access[4];
}metadata_t;

char *format_metadata(metadata_t m) {
    char *buffer = (char *)malloc(200 * sizeof(char));
    sprintf(buffer, "nume fisier: %39s\ninaltime: %d\nlungime: %d\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %d\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
        m.file_name,
        m.height,
        m.width,
        m.size,
        m.user_id,
        m.last_modified,
        m.link_count,
        m.user_access,
        m.group_access,
        m.others_access
);
    return buffer;
}

void print_metadata(metadata_t m) { 
    printf("%s", format_metadata(m));
}

void close_file(int file_descriptor) {
    if (close(file_descriptor) < 0) {
        perror("Error while closing file: ");
        exit(-1);
    }
}

void get_image_size(int file_descriptor, int *height, int *width, int *size) {

    if(lseek(file_descriptor, 18, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }
    
    if(read(file_descriptor, width, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    if(lseek(file_descriptor, 22, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }

    if(read(file_descriptor, height, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    if(lseek(file_descriptor, 2, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(-1);
    }
    
    if(read(file_descriptor, size, 4) < 0) {
        perror("Error reading from file: ");
        exit(-1);
    }

    printf("Width: %d\nHeight: %d\nSize: %d bytes\n", *width, *height, *size);
}

int open_image(char *file_path) {
    int image_descriptor = open(file_path, O_RDONLY);
    if (image_descriptor < 0) {
        perror("Error while opening file: ");
        exit(-1);
    }
    return image_descriptor;
}

void get_image_stats(const char *file_name, struct stat *buffer) {
    if(stat(file_name, buffer) < 0) {
        perror("Error getting file stats: ");
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    metadata_t metadata;
    struct stat stats;
    strcpy(metadata.file_name, argv[1]);

    int image_descriptor = open_image(metadata.file_name);
    get_image_size(image_descriptor, 
                    &metadata.height, 
                    &metadata.width, 
                    &metadata.size);
    
    get_image_stats(metadata.file_name,
                    &stats);

    metadata.user_id = (int)stats.st_uid;
    metadata.link_count = (int)stats.st_nlink;

    close_file(image_descriptor);
    return 0;
}