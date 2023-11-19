#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

typedef struct metadata_t {
    char file_name[256];
    int height;
    int width;
    int size;
    int user_id;
    char last_modified[11];
    int link_count;
    char user_access[4];
    char group_access[4];
    char others_access[4];
} metadata_t;

char *format_metadata(metadata_t m) {
    char *buffer = (char *)malloc(200 * sizeof(char));
    sprintf(buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %d\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n\n",
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

int open_file(char *file_path) {
    int file_descriptor = open(file_path, O_RDONLY);
    if (file_descriptor < 0) {
        perror("Error while opening file: ");
        exit(EXIT_FAILURE);
    }
    return file_descriptor;
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

void fill_image_properties(metadata_t *metadata, int file_descriptor) {

    int width, height, size;

    // Seek and read the width
    if (lseek(file_descriptor, 18, SEEK_SET) < 0) {
        perror("Error seeking width in file");
        return;
    }
    if (read(file_descriptor, &width, sizeof(width)) != sizeof(width)) {
        perror("Error reading width from file");
        return;
    }

    // Seek and read the height
    if (lseek(file_descriptor, 22, SEEK_SET) < 0) {
        perror("Error seeking height in file");
        return;
    }
    if (read(file_descriptor, &height, sizeof(height)) != sizeof(height)) {
        perror("Error reading height from file");
        return;
    }

    // Seek and read the file size
    if (lseek(file_descriptor, 2, SEEK_SET) < 0) {
        perror("Error seeking file size in file");
        return;
    }
    if (read(file_descriptor, &size, sizeof(size)) != sizeof(size)) {
        perror("Error reading file size from file");
        return;
    }

    // Set the metadata fields
    metadata->width = width;
    metadata->height = height;
    metadata->size = size;
}


void initialize_metadata(metadata_t *metadata, const char *file_name) {
    if (strlen(file_name) >= sizeof(metadata->file_name)) {
        fprintf(stderr, "File name too long.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(metadata->file_name, file_name);
}

void write_metadata_to_file(const char *file_name, const char *metadata) {
    int fd = open(file_name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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


const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

int is_bmp_file(const char *file_path) {
    const char *ext = get_file_extension(file_path);
    return strcmp(ext, "bmp") == 0;
}

void process_file(const char *file_path, FILE *output_file) {
    metadata_t metadata;
    struct stat stats;
    initialize_metadata(&metadata, file_path);

    int isBmp = is_bmp_file(file_path);
    if (isBmp) {
        int file_descriptor = open_file(metadata.file_name);
        fill_image_properties(&metadata, file_descriptor);
        close_file(file_descriptor);
    } else {
        int file_descriptor = open_file(metadata.file_name);
        close_file(file_descriptor);
    }

    get_image_stats(metadata.file_name, &stats); // put metadata into stats (that's the buffer)
    fill_metadata_from_stat(&stats, &metadata);

    char *formatted_metadata = format_metadata(metadata);
    write_metadata_to_file("statistica.txt", format_metadata(metadata)); // Write to the output file
    free(formatted_metadata);
}

void process_directory(const char *dir_path, FILE *output_file) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip the '.' and '..' entries
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_stat;
        if (stat(path, &entry_stat) != 0) {
            perror("Error getting file stats");
            continue;
        }

        if (S_ISREG(entry_stat.st_mode)) {
            process_file(path, output_file);
        } else if (S_ISDIR(entry_stat.st_mode)) {
            process_directory(path, output_file);
        }
        
        // TODO: handle symlinks and others here, didn't have time to do it
    }

    closedir(dir);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *output_file = fopen("statistica.txt", "w");
    if (!output_file) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    process_directory(argv[1], output_file);

    fclose(output_file);

    return EXIT_SUCCESS;
}


