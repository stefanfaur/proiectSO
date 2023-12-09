#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH_LEN 256
#define OUT_SUFFIX "statistica.txt"
#define MAX_OUT_FILE_ENTRY_SIZE 1024
#define MAX_DATETIME_LEN 32

struct BMPHeader {
  uint16_t signature;
  uint32_t fileSize;
  uint32_t reserved;
  uint32_t dataOffset;
} __attribute__((
    packed)); // remove padding so that it's compatible with the bmp data layout
// forgot it was padded by default, took me an hour to figure it out :D

void print_bmp_header(const struct BMPHeader *header) { // for debug
  printf("BMP Header:\n");
  printf("Signature: %u\n", header->signature);
  printf("File Size: %u\n", header->fileSize);
  printf("Reserved: %u\n", header->reserved);
  printf("Data Offset: %u\n", header->dataOffset);
}

struct BMPInfoHeader {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bitCount;
  uint32_t compression;
  uint32_t imageSize;
  uint32_t xPixelsPerM;
  uint32_t yPixelsPerM;
  uint32_t colorsUsed;
  uint32_t colorsImportant;
} __attribute__((packed)); // same pain as above

void print_bmp_info_header(const struct BMPInfoHeader *header) { // for debug
  printf("BMP Info Header:\n");
  printf("Size: %u\n", header->size);
  printf("Width: %u\n", header->width);
  printf("Height: %u\n", header->height);
  printf("Planes: %u\n", header->planes);
  printf("Bit Count: %u\n", header->bitCount);
  printf("Compression: %u\n", header->compression);
  printf("Image Size: %u\n", header->imageSize);
  printf("X Pixels Per Meter: %u\n", header->xPixelsPerM);
  printf("Y Pixels Per Meter: %u\n", header->yPixelsPerM);
  printf("Colors Used: %u\n", header->colorsUsed);
  printf("Colors Important: %u\n", header->colorsImportant);
}

struct BMPFile {
  struct BMPHeader header;
  struct BMPInfoHeader info;
  uint8_t *data;
} __attribute__((packed));
void print_bmp_file(const struct BMPFile *bmpFile) { // for debug, again
  printf("BMP File:\n");
  print_bmp_header(&bmpFile->header);
  print_bmp_info_header(&bmpFile->info);
  printf("\n");
}

// global buffers for convenience
static char time_buf[MAX_DATETIME_LEN] = "";       // datetime buffer
static char out_buf[MAX_OUT_FILE_ENTRY_SIZE] = ""; // out content buffer
static char out_path_buf[MAX_PATH_LEN] = "";       // output path buffer

void write_to_output_file() {
  // create output file with permissions -rw-rw-r--
  int out_fd =
      creat(out_path_buf, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  if (out_fd < 0) {
    perror("Error creating output file");
    exit(EXIT_FAILURE);
  }

  // write data
  size_t len = strlen(out_buf);
  int ret_val = write(out_fd, out_buf, len);
  if (ret_val < 0) {
    perror("Error writing output file");
    exit(EXIT_FAILURE);
  } else if (ret_val != len) {
    fprintf(stderr,
            "Error writing output file: Wrote %d bytes instead of %lu\n",
            ret_val, len);
    exit(EXIT_FAILURE);
  }
  close(out_fd);
}

void write_regular_file_stats(const char *name, struct stat *stp) {
  // write stats to buffer
  snprintf(out_buf, MAX_OUT_FILE_ENTRY_SIZE,
           "file name: %s\n"
           "size: %lld\n"
           "user id: %d\n"
           "time of last modification: %s\n"
           "number of hard links: %hu\n"
           "user permissions: %c%c%c\n"
           "group permissions: %c%c%c\n"
           "others permissions: %c%c%c\n\n",
           name, stp->st_size, stp->st_uid, time_buf, stp->st_nlink,
           (stp->st_mode & S_IRUSR) ? 'R' : '-',
           (stp->st_mode & S_IWUSR) ? 'W' : '-',
           (stp->st_mode & S_IXUSR) ? 'X' : '-',
           (stp->st_mode & S_IRGRP) ? 'R' : '-',
           (stp->st_mode & S_IWGRP) ? 'W' : '-',
           (stp->st_mode & S_IXGRP) ? 'X' : '-',
           (stp->st_mode & S_IROTH) ? 'R' : '-',
           (stp->st_mode & S_IWOTH) ? 'W' : '-',
           (stp->st_mode & S_IXOTH) ? 'X' : '-');

  write_to_output_file();
}

void write_bmp_file_stats(const char *name, const char *path,
                          struct stat *stp) {
  // open file
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  // move cursor to InfoHeader section in bitmap
  if (lseek(fd, sizeof(struct BMPHeader), SEEK_SET) < 0) {
    perror("Error seeking through file");
    exit(EXIT_FAILURE);
  }

  // read InfoHeader
  struct BMPInfoHeader info;
  if (read(fd, &info, sizeof(info)) < 0) {
    perror("Error reading bitmap header");
    exit(EXIT_FAILURE);
  }

  // close file
  close(fd);

  // write stats to buffer
  snprintf(out_buf, MAX_OUT_FILE_ENTRY_SIZE,
           "file name: %s\n"
           "height: %d\n"
           "width: %d\n"
           "size: %lld\n"
           "user id: %d\n"
           "time of last modification: %s\n"
           "number of hard links: %hu\n"
           "user permissions: %c%c%c\n"
           "group permissions: %c%c%c\n"
           "others permissions: %c%c%c\n\n",
           name, info.height, info.width, stp->st_size, stp->st_uid, time_buf,
           stp->st_nlink, (stp->st_mode & S_IRUSR) ? 'R' : '-',
           (stp->st_mode & S_IWUSR) ? 'W' : '-',
           (stp->st_mode & S_IXUSR) ? 'X' : '-',
           (stp->st_mode & S_IRGRP) ? 'R' : '-',
           (stp->st_mode & S_IWGRP) ? 'W' : '-',
           (stp->st_mode & S_IXGRP) ? 'X' : '-',
           (stp->st_mode & S_IROTH) ? 'R' : '-',
           (stp->st_mode & S_IWOTH) ? 'W' : '-',
           (stp->st_mode & S_IXOTH) ? 'X' : '-');

  write_to_output_file();
}

void write_link_stats(const char *name, const char *path, struct stat *stp) {
  // read stats of target file
  struct stat tgf;
  if (stat(path, &tgf) < 0) {
    perror("Error reading target file stats");
    exit(EXIT_FAILURE);
  }

  // write stats to buffer
  snprintf(out_buf, MAX_OUT_FILE_ENTRY_SIZE,
           "link name: %s\n"
           "link size: %lld\n"
           "target file size: %lld\n"
           "time of last modification: %s\n"
           "user permissions: %c%c%c\n"
           "group permissions: %c%c%c\n"
           "others permissions: %c%c%c\n\n",
           name, stp->st_size, tgf.st_size, time_buf,
           (stp->st_mode & S_IRUSR) ? 'R' : '-',
           (stp->st_mode & S_IWUSR) ? 'W' : '-',
           (stp->st_mode & S_IXUSR) ? 'X' : '-',
           (stp->st_mode & S_IRGRP) ? 'R' : '-',
           (stp->st_mode & S_IWGRP) ? 'W' : '-',
           (stp->st_mode & S_IXGRP) ? 'X' : '-',
           (stp->st_mode & S_IROTH) ? 'R' : '-',
           (stp->st_mode & S_IWOTH) ? 'W' : '-',
           (stp->st_mode & S_IXOTH) ? 'X' : '-');

  write_to_output_file();
}

void write_dir_stats(const char *name, struct stat *stp) {
  // write stats to buffer
  snprintf(out_buf, MAX_OUT_FILE_ENTRY_SIZE,
           "directory name: %s\nuser id: %d\n"
           "time of last modification: %s\n"
           "user permissions: %c%c%c\n"
           "group permissions: %c%c%c\n"
           "others permissions: %c%c%c\n\n",
           name, stp->st_uid, time_buf, (stp->st_mode & S_IRUSR) ? 'R' : '-',
           (stp->st_mode & S_IWUSR) ? 'W' : '-',
           (stp->st_mode & S_IXUSR) ? 'X' : '-',
           (stp->st_mode & S_IRGRP) ? 'R' : '-',
           (stp->st_mode & S_IWGRP) ? 'W' : '-',
           (stp->st_mode & S_IXGRP) ? 'X' : '-',
           (stp->st_mode & S_IROTH) ? 'R' : '-',
           (stp->st_mode & S_IWOTH) ? 'W' : '-',
           (stp->st_mode & S_IXOTH) ? 'X' : '-');

  write_to_output_file();
}

void convert_bmp_to_grayscale(const char *path) {
  // open file
  int fd = open(path, O_RDWR);
  if (fd < 0) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  // read Header & InfoHeader
  struct BMPFile f;
  if ((read(fd, &f.header, sizeof(f.header)) < 0) ||
      (read(fd, &f.info, sizeof(f.info)) < 0)) {
    perror("Error reading bitmap header");
    exit(EXIT_FAILURE);
  }

  // print_bmp_file(&f);

  if (f.info.bitCount <= 8) {
    fprintf(stderr, "%d: Unsupported bitmap format for %s\n", f.info.bitCount,
            path);
    exit(EXIT_FAILURE);
  }

  // read pixel data
  int imgSize = f.info.width * f.info.height * 3;
  f.data = (uint8_t *)malloc(imgSize);
  if (f.data == NULL) {
    perror("Cannot allocate memory for pixel data");
    exit(EXIT_FAILURE);
  }
  if (read(fd, f.data, imgSize) < 0) {
    perror("Error reading bitmap pixel data");
    exit(EXIT_FAILURE);
  }

  // convert to grayscale
  // p_grey = 0.299*p_red + 0.587*p_green + 0.114*p_blue
  for (int i = 0; i < f.info.height; ++i) {
    for (int j = 0; j < f.info.width; ++j) {
      uint8_t *pixel = &f.data[(i * f.info.width + j) *
                               3]; // store in f.data(acting as buffer now) to
                                   // overwrite image later
      uint8_t gray =
          (uint8_t)(0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0]);
      pixel[0] = pixel[1] = pixel[2] = gray;
    }
  }

  // go to pixel data section of file
  if (lseek(fd, sizeof(struct BMPHeader) + sizeof(struct BMPInfoHeader),
            SEEK_SET) < 0) {
    perror("Error seeking through file");
    exit(EXIT_FAILURE);
  }

  // overwrite file
  if (write(fd, f.data, imgSize) < 0) {
    perror("Error converting bitmap to grayscale");
    free(f.data);
    exit(EXIT_FAILURE);
  }
  free(f.data);
  close(fd);
}

size_t build_input_file_path(char *const *argv, const struct dirent *dr,
                             const char *file_path) { // build file path
  memset(file_path, 0, MAX_PATH_LEN);
  strncat(file_path, argv[1], MAX_PATH_LEN - 1);
  strncat(file_path, "/", MAX_PATH_LEN - strlen(file_path) - 1);
  strncat(file_path, dr->d_name, MAX_PATH_LEN - strlen(file_path) - 1);
  size_t path_len = strlen(file_path);
  return path_len;
}

void build_output_file_path(char *const *argv, const struct dirent *dr) {
  memset(out_path_buf, 0, MAX_PATH_LEN);
  strncat(out_path_buf, argv[2], MAX_PATH_LEN - 1);
  strncat(out_path_buf, "/", MAX_PATH_LEN - strlen(out_path_buf) - 1);
  strncat(out_path_buf, dr->d_name, MAX_PATH_LEN - strlen(out_path_buf) - 1);
  strncat(out_path_buf, "_", MAX_PATH_LEN - strlen(out_path_buf) - 1);
  strncat(out_path_buf, OUT_SUFFIX,
          MAX_PATH_LEN - strlen(out_path_buf) - 1);
}

void process_regular_child1(const struct dirent *dr, const char *file_path,
                            struct stat *fst, const int *p1fd,
                            const int *p2fd) {
  write_regular_file_stats(dr->d_name, fst);

  // close both ends of pipe2
  close(p2fd[0]);
  close(p2fd[1]);

  // close read end of pipe1
  close(p1fd[0]);

  // redirect stdout to write end of pipe1
  if (dup2(p1fd[1], 1) < 0) {
    perror("Error redirecting stdout");
    exit(EXIT_FAILURE);
  }

  // send file contents through pipe1
  execlp("cat", "cat", file_path, NULL);
}

void process_regular_child2(char *const *argv, const int *p1fd,
                            const int *p2fd) { // close write end of pipe1
  close(p1fd[1]);

  // redirect stdin to read end of pipe1
  if (dup2(p1fd[0], 0) < 0) {
    perror("Error redirecting stdin");
    exit(EXIT_FAILURE);
  }

  // close read end of pipe2
  close(p2fd[0]);

  // redirect stdout to write end of pipe2
  if (dup2(p2fd[1], 1) < 0) {
    perror("Error redirecting stdout");
    exit(EXIT_FAILURE);
  }

  // call script which reads from stdin and writes to stdout
  execlp("/bin/sh", "/bin/sh", "./s9.sh", argv[3], NULL);
}

int read_piped_script_output(const int *p2fd) {
  char bf[16] = "";
  if (read(p2fd[0], bf, sizeof(bf)) < 0) {
    perror("Error reading from pipe");
    exit(EXIT_FAILURE);
  }
  int cnt = atoi(bf);
  return cnt;
}

int process_regular_file(char *const *argv, int valid_sentence_cnt,
                         const struct dirent *dr, const char *file_path,
                         struct stat *fst, int pid, const int *p1fd,
                         const int *p2fd) {
  // this returns the number of correct sentences in the file
  if (pid == 0) { // 1st child
    process_regular_child1(dr, file_path, fst, p1fd, p2fd);

  } else {                    // parent
    if ((pid = fork()) < 0) { // create 2nd child to exec script and read from
                              // pipe(which should be loaded by the 1st child)
      perror("Error creating process");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) { // 2nd child
      process_regular_child2(argv, p1fd, p2fd);
    }
    // parent

    // close both ends of pipe1
    close(p1fd[0]);
    close(p1fd[1]);

    // close write end of pipe2
    close(p2fd[1]);

    // read number from pipe2 as string and convert to int
    int cnt = read_piped_script_output(p2fd);

    // update total number of correct sentences
    valid_sentence_cnt += cnt;
  }
  return valid_sentence_cnt;
}

void process_bmp(const struct dirent *dr, const char *file_path,
                 struct stat *fst, int pid) {
  if (pid == 0) { // 1st child
    write_bmp_file_stats(dr->d_name, file_path, fst);
    exit(EXIT_SUCCESS);
  } else {                    // parent
    if ((pid = fork()) < 0) { // create 2nd child
      perror("Error creating process");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) { // 2nd child
      convert_bmp_to_grayscale(file_path);
      exit(EXIT_SUCCESS);
    }
  }
}

bool is_bmp(const char *file_path, size_t path_len) {
  return file_path[path_len - 4] == '.' && file_path[path_len - 3] == 'b' &&
         file_path[path_len - 2] == 'm' && file_path[path_len - 1] == 'p';
}

void load_formatted_time(const struct stat *fst) {
  time_t t = (*fst).st_mtime;
  struct tm *lt = localtime(&t);
  strftime(time_buf, MAX_DATETIME_LEN, "%c", lt);
}

int processEntry(char *const *argv, int valid_sentence_cnt,
                 const struct dirent *dr, const char *file_path,
                 struct stat *fst, int pid) {

  size_t path_len = build_input_file_path(argv, dr, file_path);

  // read file stats
  if (lstat(file_path, fst) < 0) {
    perror("Error reading file stats");
    exit(EXIT_FAILURE);
  }

  // format datetime properly and store in buffer
  load_formatted_time(fst); // loads the formatted time into time_buf

  // build output path and store in buffer
  build_output_file_path(argv, dr);

  // create pipes
  //  pipe1: child1 -> child2
  //  pipe2: child2 -> parent
  int p1fd[2], p2fd[2];
  if ((pipe(p1fd) < 0) || (pipe(p2fd) < 0)) {
    perror("Error creating pipes");
    exit(EXIT_FAILURE);
  }

  // create 1st child process
  if ((pid = fork()) < 0) {
    perror("Error creating process");
    exit(EXIT_FAILURE);
  }

  if (S_ISLNK((*fst).st_mode)) { // symlink
    if (pid == 0) {              // 1st child
      write_link_stats(dr->d_name, file_path, fst);
      exit(EXIT_SUCCESS);
    }

  } else if (S_ISDIR((*fst).st_mode)) { // dir
    if (pid == 0) {                     // 1st child
      write_dir_stats(dr->d_name, fst);
      exit(EXIT_SUCCESS);
    }

  } else if (is_bmp(file_path, path_len)) { // bmp
    process_bmp(dr, file_path, fst, pid);

  } else if (S_ISREG((*fst).st_mode)) { // regular (here we have to send
                                        // contents through pipe, to the script)
    valid_sentence_cnt = process_regular_file(argv, valid_sentence_cnt, dr,
                                              file_path, fst, pid, p1fd, p2fd);
  }

  // make sure all pipes are closed
  close(p1fd[0]);
  close(p1fd[1]);
  close(p2fd[0]);
  close(p2fd[1]);
  return valid_sentence_cnt;
}

DIR *safe_open_dir(char *const *argv) {
  DIR *dirptr = opendir(argv[1]);
  if (dirptr == NULL) {
    perror("Error opening input directory");
    exit(EXIT_FAILURE);
  }
  return dirptr;
}

void check_arguments(int argc, char *const *argv) {
  if (argc != 4) {
    fprintf(stderr, "Wrong number of arguments.\n");
    fprintf(stderr, "Usage: %s <input_directory> <output_directory> <c>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }
}

void print_status(int pid) {
  int status = 0;
  while ((pid = wait(&status)) > 0) {
    printf("Process with pid %d ended with code %d\n", pid, status);
  }
}

int main(int argc, char **argv) {
  check_arguments(argc, argv);
  // open input directory
  DIR *dirptr = safe_open_dir(argv);
  int valid_sentence_cnt = 0;

  // skip '.' and '..' entries
  struct dirent *dr;
  dr = readdir(dirptr); // unused, just skips
  dr = readdir(dirptr); // unused, just skips
  dr = readdir(dirptr);

  // loop input dir on remaining entries
  char file_path[MAX_PATH_LEN] = "";
  struct stat fst;
  int pid = 0;
  for (; dr != NULL; dr = readdir(dirptr)) {
    valid_sentence_cnt =
        processEntry(argv, valid_sentence_cnt, dr, file_path, &fst, pid);
  }

  print_status(pid);

  // close input dir
  closedir(dirptr);

  // print final message
  printf("Au fost identificate in total %d propozitii corecte care contin "
         "caracterul %c\n",
         valid_sentence_cnt, argv[3][0]);

  return 0;
}
