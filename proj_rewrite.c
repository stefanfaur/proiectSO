#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define PATH_MAX 1024

void process_entry(const char *path, const struct dirent *entry,
                   int output_file) {
  char full_path[PATH_MAX];
  struct stat buffer;
  char sbuffer[100];

  // get absolute path of entry
  snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

  // get current entry stats
  if (lstat(full_path, &buffer) == -1) {
    perror("lstat");
    return;
  }

  if (S_ISREG(buffer.st_mode)) { // if a regular file
    sprintf(sbuffer, "\nnume fisier: %s\n", entry->d_name);
    write(output_file, sbuffer, strlen(sbuffer));

    if (strstr(entry->d_name, ".bmp") != NULL) {
      
      int width, height;
      int file_descriptor = open(full_path, O_RDONLY);
        if (file_descriptor == -1) {
            perror("Couldn't open file: ");
            return;
        }
      // seek and read the width
      if (lseek(file_descriptor, 18, SEEK_SET) < 0) {
        perror("error seeking width in file: ");
        return;
      }
      if (read(file_descriptor, &width, sizeof(width)) < 0 ) {
        perror("error reading width from file: ");
        return;
      }

      // seek and read the height
      if (lseek(file_descriptor, 22, SEEK_SET) < 0) {
        perror("Error seeking height in file: ");
        return;
      }
      if (read(file_descriptor, &height, sizeof(height)) < 0 ) {
        perror("Error reading height from file: ");
        return;
      }

      sprintf(sbuffer, "lungime: %d\n", width);
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "inaltime: %d\n", height);
      write(output_file, sbuffer, strlen(sbuffer));
    }

    sprintf(sbuffer, "contorul de legaturi: %hu\n", buffer.st_nlink);
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "identificatorul utilizatorului: %d\n", buffer.st_uid);
    write(output_file, sbuffer, strlen(sbuffer));

    struct tm *tm_info;
    tm_info = localtime(&buffer.st_mtime);
    strftime(sbuffer, sizeof(sbuffer),
             "timpul ultimei modificari: %Y-%m-%d %H:%M:%S\n", tm_info);
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces user: ");
    strcat(sbuffer, ((buffer.st_mode & S_IRUSR)) ? "R" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IWUSR)) ? "W" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IXUSR)) ? "X" : "-");
    strcat(sbuffer, "\n");
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces grup: ");
    strcat(sbuffer, ((buffer.st_mode & S_IRGRP)) ? "R" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IWGRP)) ? "W" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IXGRP)) ? "X" : "-");
    strcat(sbuffer, "\n");
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces altii: ");
    strcat(sbuffer, ((buffer.st_mode & S_IROTH)) ? "R" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IWOTH)) ? "W" : "-");
    strcat(sbuffer, ((buffer.st_mode & S_IXOTH)) ? "X" : "-");
    strcat(sbuffer, "\n");
    write(output_file, sbuffer, strlen(sbuffer));

  } else if (S_ISLNK(buffer.st_mode)) { // if a symlink
    sprintf(sbuffer, "\nnume legatura: %s\n", entry->d_name);
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "dimensiune: %lld\n", buffer.st_size);
    write(output_file, sbuffer, strlen(sbuffer));

    struct stat target_buffer;
    if (stat(full_path, &target_buffer) == 0) {
      sprintf(sbuffer, "dimensiune fisier: %lld\n", target_buffer.st_size);
      write(output_file, sbuffer, strlen(sbuffer));
    }

    sprintf(sbuffer, "drepturi de acces user: %s\n",
            ((buffer.st_mode & S_IRUSR) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces grup: %s\n",
            ((buffer.st_mode & S_IRGRP) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces altii: %s\n",
            ((buffer.st_mode & S_IROTH) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));

  } else if (S_ISDIR(buffer.st_mode)) { // if a directory
    sprintf(sbuffer, "\nnume director: %s\n", entry->d_name);
    write(output_file, sbuffer, strlen(sbuffer));

    struct passwd *user_info = getpwuid(buffer.st_uid);
    if (user_info != NULL) {
      sprintf(sbuffer, "identificatorul utilizatorului: %s\n",
              user_info->pw_name);
      write(output_file, sbuffer, strlen(sbuffer));
    }

    sprintf(sbuffer, "drepturi de acces user: %s\n",
            ((buffer.st_mode & S_IRUSR) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces grup: %s\n",
            ((buffer.st_mode & S_IRGRP) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));

    sprintf(sbuffer, "drepturi de acces altii: %s\n",
            ((buffer.st_mode & S_IROTH) ? "R" : "-"));
    write(output_file, sbuffer, strlen(sbuffer));
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <director_intrare>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  DIR *dir = opendir(argv[1]);
  if (dir == NULL) {
    perror("Couldn't open directory: ");
    exit(EXIT_FAILURE);
  }

  int output_file = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (output_file == -1) {
    perror("Couldn't open output file: ");
    closedir(dir);
    exit(EXIT_FAILURE);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 &&
        strcmp(entry->d_name, "..") != 0) { // skip '.' and '..' entries
      process_entry(argv[1], entry, output_file);
    }
  }

  close(output_file);
  closedir(dir);

  return 0;
}