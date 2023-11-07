#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "time.h"

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
    sprintf(buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %d\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
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
        exit(EXIT_FAILURE);
    }
}

void get_image_size(int file_descriptor, int *height, int *width, int *size) {

    if(lseek(file_descriptor, 18, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }
    
    if(read(file_descriptor, width, 4) < 0) {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }

    if(lseek(file_descriptor, 22, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if(read(file_descriptor, height, 4) < 0) {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }

    if(lseek(file_descriptor, 2, SEEK_SET) < 0) {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }
    
    if(read(file_descriptor, size, 4) < 0) {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }
}

int open_image(char *file_path) {
    int image_descriptor = open(file_path, O_RDONLY);
    if (image_descriptor < 0) {
        perror("Error while opening file: ");
        exit(EXIT_FAILURE);
    }
    return image_descriptor;
}

void get_image_stats(const char *file_name, struct stat *buffer) {
    if(stat(file_name, buffer) < 0) {
        perror("Error getting file stats: ");
        exit(EXIT_FAILURE);
    }
}

char* timespec_to_date(const struct timespec *ts) {
    // Allocate memory for the date string
    char *date_str = malloc(11); // DD-MM-YYYY + '\0'
    if (date_str == NULL) {
        perror("malloc");
        return NULL;
    }

    // Convert timespec seconds to tm struct
    struct tm lt;
    if (localtime_r(&(ts->tv_sec), &lt) == NULL) {
        free(date_str);
        return NULL;
    }

    if (strftime(date_str, 11, "%d-%m-%Y", &lt) == 0) {
        fprintf(stderr, "strftime returned 0");
        free(date_str);
        return NULL;
    }

    return date_str;
}

void mode_to_string(mode_t mode, char *str, char who) {
    switch (who) {
        case 'u': // User
            str[0] = (mode & S_IRUSR) ? 'r' : '-';
            str[1] = (mode & S_IWUSR) ? 'w' : '-';
            str[2] = (mode & S_IXUSR) ? 'x' : '-';
            break;
        case 'g': // Group
            str[0] = (mode & S_IRGRP) ? 'r' : '-';
            str[1] = (mode & S_IWGRP) ? 'w' : '-';
            str[2] = (mode & S_IXGRP) ? 'x' : '-';
            break;
        case 'o': // Others
            str[0] = (mode & S_IROTH) ? 'r' : '-';
            str[1] = (mode & S_IWOTH) ? 'w' : '-';
            str[2] = (mode & S_IXOTH) ? 'x' : '-';
            break;
    }
    str[3] = '\0';
}

void get_permissions(struct stat *stats, metadata_t *metadata) {
    mode_to_string(stats->st_mode, metadata->user_access, 'u');
    mode_to_string(stats->st_mode, metadata->group_access, 'g');
    mode_to_string(stats->st_mode, metadata->others_access, 'o');
}

void fill_metadata_from_stat(struct stat *stats, metadata_t *metadata) {
    metadata->user_id = (int)stats->st_uid;
    metadata->link_count = (int)stats->st_nlink;
    
    strcpy(metadata->last_modified, timespec_to_date(&stats->st_mtimespec));
    
    get_permissions(stats, metadata);
}

void fill_image_properties(metadata_t *metadata, int image_descriptor) {
    get_image_size(image_descriptor, 
                   &metadata->height, 
                   &metadata->width, 
                   &metadata->size);
}

void initialize_metadata(metadata_t *metadata, const char *file_name) {
    if (strlen(file_name) >= sizeof(metadata->file_name)) {
        fprintf(stderr, "File name too long.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(metadata->file_name, file_name);
}

void write_metadata_to_file(const char *file_name, const char *metadata) {
    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        perror("Error opening statistics file: ");
        exit(EXIT_FAILURE);
    }

    if (write(fd, metadata, strlen(metadata)) < 0) {
        perror("Error writing to statistics file: ");
        close(fd); // Try to close the file even if write failed.
        exit(EXIT_FAILURE);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    metadata_t metadata;
    struct stat stats;

    initialize_metadata(&metadata, argv[1]);

    int image_descriptor = open_image(metadata.file_name);
    fill_image_properties(&metadata, image_descriptor);
    get_image_stats(metadata.file_name, &stats);
    fill_metadata_from_stat(&stats, &metadata);
    close_file(image_descriptor);
    char *formatted_metadata = format_metadata(metadata);
    write_metadata_to_file("statistica.txt", format_metadata(metadata));
    free(formatted_metadata);

    //print_metadata(metadata);

    return EXIT_SUCCESS;
}