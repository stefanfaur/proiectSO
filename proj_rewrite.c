#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void convert_to_grayscale(const char *input_path, const char *output_path) {
  int input_file = open(input_path, O_RDONLY);
  if (input_file == -1) {
    perror("Could not open input file");
    exit(EXIT_FAILURE);
  }

  char header[54];
  if (read(input_file, header, sizeof(header)) < 0) {
    perror("Could not read header");
    close(input_file);
    exit(EXIT_FAILURE);
  }

  int width = *(int *)&header[18];
  int height = *(int *)&header[22];
  int size = width * height * 3;

  unsigned char *original_image = (unsigned char *)malloc(size);
  if (original_image == NULL) {
    perror("Could not allocate memory");
    close(input_file);
    exit(EXIT_FAILURE);
  }

  if (read(input_file, original_image, size) < 0) {
    printf("%s\n", input_path);
    perror("convert_to_grayscale: Could not read image");
    free(original_image);
    close(input_file);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; i += 3) {
    unsigned char blue = original_image[i];
    unsigned char green = original_image[i + 1];
    unsigned char red = original_image[i + 2];

    unsigned char grayscale =
        (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);

    original_image[i] = original_image[i + 1] = original_image[i + 2] =
        grayscale;
  }

  int output_file = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (output_file == -1) {
    perror("open");
    free(original_image);
    close(input_file);
    exit(EXIT_FAILURE);
  }

  if (write(output_file, header, sizeof(header)) != sizeof(header)) {
    perror("Could not write header.");
    free(original_image);
    close(input_file);
    close(output_file);
    exit(EXIT_FAILURE);
  }

  if (write(output_file, original_image, size) != size) {
    perror("Could not write image.");
    free(original_image);
    close(input_file);
    close(output_file);
    exit(EXIT_FAILURE);
  }

  free(original_image);
  close(input_file);
  close(output_file);
}

void process_entry(const char *input_path, const struct dirent *entry,
                   const char *output_dir, const char *c) {
  char input_full_path[PATH_MAX];
  char output_file_path[PATH_MAX];

  snprintf(input_full_path, sizeof(input_full_path), "%s/%s", input_path,
           entry->d_name);

  if (strstr(entry->d_name, ".bmp") != NULL) { // if it's a bmp file, create a child process for converting it to grayscale
    int bmp_child_pid = fork();
    if (bmp_child_pid == 0) {
      snprintf(output_file_path, sizeof(output_file_path), "%s/%s", output_dir,
               entry->d_name);

      convert_to_grayscale(input_full_path, output_file_path);

      exit(EXIT_SUCCESS);
    } else if (bmp_child_pid < 0) {
      perror("Could not create child process for BMP conversion");
      exit(EXIT_FAILURE);
    }
  }

  int stat_child_pid = fork(); // create a child process for logging statistics
  if (stat_child_pid == 0) {
    snprintf(output_file_path, sizeof(output_file_path), "%s/%s_statistica.txt",
             output_dir, entry->d_name);

    int output_file =
        open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_file == -1) {
      perror("Could not open output file");
      exit(EXIT_FAILURE);
    }

    struct stat buffer;
    char sbuffer[1000];

    if (lstat(input_full_path, &buffer) == -1) {
      perror("Could not get file stats: ");
      exit(EXIT_FAILURE);
    }
    if (S_ISREG(buffer.st_mode) && strstr(entry->d_name, ".bmp") == NULL) {
      // non-bmp file
    int pipe_fds[2];
    if (pipe(pipe_fds) < 0) {
      perror("Could not create pipe");
      exit(EXIT_FAILURE);
    }

    // Proces fiu pentru generare continut
    int gen_content_pid = fork();
    if (gen_content_pid == 0) {
      // De exemplu, se poate apela un script extern pentru aceasta
      close(pipe_fds[0]); // Inchidere capat de citire
      char *content = "Continut generat\n"; // Aici apelez scriptul???? Ce continut generat? wtf
      int content_length = strlen(content);
      write(pipe_fds[1], content, content_length);
      close(pipe_fds[1]); // Inchidere capat de scriere
      exit(EXIT_SUCCESS);
    }

    // Proces fiu pentru calcul numar propozitii corecte
    int calc_sentences_pid = fork();
    if (calc_sentences_pid == 0) {
      close(pipe_fds[1]); // Inchidere capat de scriere

      // Citesc continutul din pipe si calculez numarul de propozitii corecte
      // int num_sentences = ...; // Logica de calcul a propozitiilor corecte // Aici apelez scriptul????
      // write(pipe_fds[0], &num_sentences, sizeof(num_sentences));

      close(pipe_fds[0]); // Inchide capat de citire
      exit(EXIT_SUCCESS);
    }

    close(pipe_fds[0]);
    close(pipe_fds[1]);

    // Astept terminarea proceselor fiu
    int status;
    waitpid(gen_content_pid, &status, 0);
    printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", gen_content_pid, WEXITSTATUS(status));

    waitpid(calc_sentences_pid, &status, 0);
    printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", calc_sentences_pid, WEXITSTATUS(status));
  }

    if (S_ISREG(buffer.st_mode)) { // if a regular file
      sprintf(sbuffer, "\nnume fisier: %s\n", entry->d_name);
      write(output_file, sbuffer, strlen(sbuffer));

      if (strstr(entry->d_name, ".bmp") != NULL) {

        int width, height;
        int file_descriptor = open(input_full_path, O_RDONLY);
        if (file_descriptor == -1) {
          perror("Couldn't open file");
          return;
        }
        // seek and read the width
        if (lseek(file_descriptor, 18, SEEK_SET) < 0) {
          perror("Error seeking width in file");
          return;
        }
        if (read(file_descriptor, &width, sizeof(width)) < 0) {
          perror("Error reading width from file");
          return;
        }

        // seek and read the height
        if (lseek(file_descriptor, 22, SEEK_SET) < 0) {
          perror("Error seeking height in file: ");
          return;
        }
        if (read(file_descriptor, &height, sizeof(height)) < 0) {
          perror("Error reading height from file");
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

      // Last modified time
      struct tm *tm_info;
      tm_info = localtime(&buffer.st_mtime);
      strftime(sbuffer, sizeof(sbuffer), "Last modified:: %Y-%m-%d %H:%M:%S\n",
               tm_info);
      write(output_file, sbuffer, strlen(sbuffer));

      // User access rights
      sprintf(sbuffer, "Drepturi de acces user: ");
      strcat(sbuffer, ((buffer.st_mode & S_IRUSR)) ? "R" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IWUSR)) ? "W" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IXUSR)) ? "X" : "-");
      strcat(sbuffer, "\n");
      write(output_file, sbuffer, strlen(sbuffer));

      // Group access rights
      sprintf(sbuffer, "Drepturi de acces grup: ");
      strcat(sbuffer, ((buffer.st_mode & S_IRGRP)) ? "R" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IWGRP)) ? "W" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IXGRP)) ? "X" : "-");
      strcat(sbuffer, "\n");
      write(output_file, sbuffer, strlen(sbuffer));

      // Other access rights
      sprintf(sbuffer, "Drepturi de acces other: ");
      strcat(sbuffer, ((buffer.st_mode & S_IROTH)) ? "R" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IWOTH)) ? "W" : "-");
      strcat(sbuffer, ((buffer.st_mode & S_IXOTH)) ? "X" : "-");
      strcat(sbuffer, "\n");
      write(output_file, sbuffer, strlen(sbuffer));

    } else if (S_ISLNK(buffer.st_mode)) {
      // Symlink
      sprintf(sbuffer, "Nume legatura: %s\n", entry->d_name);
      write(output_file, sbuffer, strlen(sbuffer));

      // Symlink size
      sprintf(sbuffer, "Dimensiune legatura: %lld\n", buffer.st_size);
      write(output_file, sbuffer, strlen(sbuffer));

      // Target size
      struct stat target_buffer;
      if (stat(input_full_path, &target_buffer) == 0) {
        sprintf(sbuffer, "Dimensiune fisier target: %lld\n",
                target_buffer.st_size);
        write(output_file, sbuffer, strlen(sbuffer));
      }

      // User access rights for the symbolic link
      sprintf(sbuffer, "Drepturi de acces user legatura: %s\n",
              ((buffer.st_mode & S_IRUSR) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces grup legatura: %s\n",
              ((buffer.st_mode & S_IRGRP) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces other legatura: %s\n",
              ((buffer.st_mode & S_IROTH) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

    } else if (S_ISDIR(buffer.st_mode)) {
      // Directory
      sprintf(sbuffer, "Nume director: %s\n", entry->d_name);
      write(output_file, sbuffer, strlen(sbuffer));

      // User identifier
      struct passwd *user_info = getpwuid(buffer.st_uid);
      if (user_info != NULL) {
        sprintf(sbuffer, "Identificatorul utilizatorului: %s\n",
                user_info->pw_name);
        write(output_file, sbuffer, strlen(sbuffer));
      }

      // User access rights for the directory
      sprintf(sbuffer, "Drepturi de acces user: %s\n",
              ((buffer.st_mode & S_IRUSR) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces grup: %s\n",
              ((buffer.st_mode & S_IRGRP) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces other: %s\n",
              ((buffer.st_mode & S_IROTH) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));
    } else if (S_ISDIR(buffer.st_mode)) {
      // Directory
      sprintf(sbuffer, "Nume director: %s\n", entry->d_name);
      write(output_file, sbuffer, strlen(sbuffer));

      struct passwd *user_info = getpwuid(buffer.st_uid);
      if (user_info != NULL) {
        sprintf(sbuffer, "Identificatorul utilizatorului: %s\n",
                user_info->pw_name);
        write(output_file, sbuffer, strlen(sbuffer));
      }

      // Drepturi de acces pentru director
      sprintf(sbuffer, "Drepturi de acces user: %s\n",
              ((buffer.st_mode & S_IRUSR) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces grup: %s\n",
              ((buffer.st_mode & S_IRGRP) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));

      sprintf(sbuffer, "Drepturi de acces other: %s\n",
              ((buffer.st_mode & S_IROTH) ? "R" : "-"));
      write(output_file, sbuffer, strlen(sbuffer));
    }

    write(output_file, "\n", 1);
    close(output_file);

    exit(EXIT_SUCCESS);
  } else if (stat_child_pid < 0) {
    perror("Could not create child process for statistics");
    exit(EXIT_FAILURE);
  }

  int status;

  pid_t bmp_child_pid = fork();
  if (bmp_child_pid == 0) {
    snprintf(output_file_path, sizeof(output_file_path), "%s/%s", output_dir,
             entry->d_name);

    // Child process for BMP conversion
    convert_to_grayscale(input_full_path, output_file_path);

    exit(EXIT_SUCCESS);
  } else if (bmp_child_pid < 0) {
    perror("Could not create child process for BMP conversion");
    exit(EXIT_FAILURE);
  }

  waitpid(bmp_child_pid, &status, 0);
  printf("Proces incheiat cu pid %d si cod %d\n", bmp_child_pid,
         WEXITSTATUS(status));

  waitpid(stat_child_pid, &status, 0);
  printf("Proces incheiat cu pid %d si cod %d\n", stat_child_pid,
         WEXITSTATUS(status));
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <director_intrare> <director_iesire> <c>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  char c = argv[3][0]; // Presupunem că argumentul <c> este un singur caracter
  if (!isalnum(c)) {
    fprintf(stderr, "Argumentul <c> trebuie să fie un caracter alfanumeric.\n");
    exit(EXIT_FAILURE);
  }

  DIR *dir = opendir(argv[1]);
  if (dir == NULL) {
    perror("Could not open input directory");
    exit(EXIT_FAILURE);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      process_entry(argv[1], entry, argv[2], argv[3]);
    }
  }

  closedir(dir);

  return 0;
}
