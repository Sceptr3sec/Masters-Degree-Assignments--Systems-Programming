#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* File structure to store file information */
typedef struct s_file {
    char *name;         /* File name */
    long tv_sec;        /* Modification time seconds */
    long tv_nsec;       /* Modification time nanoseconds */
    int is_dir;         /* Flag: 1 if directory, 0 if file */
} t_file;

/* Calculate string length (replacement for strlen) */
int str_len(const char *s) {
    int len = 0;
    if (!s) return 0;
    while (s[len]) len++;
    return len;
}

/* Duplicate string (replacement for strdup) */
char *str_dup(const char *s) {
    int len = str_len(s);
    char *dup = malloc(len + 1);
    int i = 0;
    if (!dup) return NULL;
    while (i < len) {
        dup[i] = s[i];
        i++;
    }
    dup[i] = '\0';
    return dup;
}

/* Compare strings (replacement for strcmp) */
int str_cmp(const char *s1, const char *s2) {
    int i = 0;
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (s1[i] && s2[i] && s1[i] == s2[i]) i++;
    return s1[i] - s2[i];
}

/* Free array of file structures */
void free_files(t_file *files, int count) {
    int i = 0;
    if (!files) return;
    while (i < count) {
        if (files[i].name) free(files[i].name);
        i++;
    }
    free(files);
}

/* Compare files by modification time (most recent first), then by name */
int time_cmp(t_file *a, t_file *b) {
    /* Compare seconds first */
    if (a->tv_sec != b->tv_sec) {
        return (a->tv_sec > b->tv_sec) ? -1 : 1;
    }
    /* If seconds equal, compare nanoseconds */
    if (a->tv_nsec != b->tv_nsec) {
        return (a->tv_nsec > b->tv_nsec) ? -1 : 1;
    }
    /* If times equal, sort by name lexicographically */
    return str_cmp(a->name, b->name);
}

/* Sort files array using bubble sort */
void sort_files(t_file *files, int count, int time_sort) {
    int i = 0;
    int j;
    t_file tmp;
    
    /* Bubble sort implementation */
    while (i < count - 1) {
        j = i + 1;
        while (j < count) {
            int swap = 0;
            /* Determine sorting criteria */
            if (time_sort) {
                if (time_cmp(&files[i], &files[j]) > 0)
                    swap = 1;
            } else {
                if (str_cmp(files[i].name, files[j].name) > 0)
                    swap = 1;
            }
            /* Perform swap if needed */
            if (swap) {
                tmp = files[i];
                files[i] = files[j];
                files[j] = tmp;
            }
            j++;
        }
        i++;
    }
}

/* Get file information for a given path */
int get_file_info(const char *path, t_file *file) {
    struct stat st;
    
    if (lstat(path, &st) == -1) return -1;
    file->name = str_dup(path);
    if (!file->name) return -1;
    /* Use st_mtim for precise modification time (not st_mtime) */
    file->tv_sec = st.st_mtim.tv_sec;
    file->tv_nsec = st.st_mtim.tv_nsec;
    file->is_dir = S_ISDIR(st.st_mode);
    return 0;
}

/* Get file information with proper path construction for directory entries */
int get_file_time(const char *dir_path, const char *filename, t_file *file) {
    char full_path[4096];
    int dir_len = str_len(dir_path);
    int name_len = str_len(filename);
    int i = 0;
    int j = 0;
    struct stat st;
    
    /* Construct full path: dir_path + '/' + filename */
    while (i < dir_len && i < 4095) {
        full_path[i] = dir_path[i];
        i++;
    }
    /* Add separator if needed */
    if (i < 4095 && dir_len > 0 && dir_path[dir_len - 1] != '/') {
        full_path[i] = '/';
        i++;
    }
    /* Append filename */
    while (j < name_len && i < 4095) {
        full_path[i] = filename[j];
        i++;
        j++;
    }
    full_path[i] = '\0';
    
    /* Get file stats using the full path */
    if (lstat(full_path, &st) == -1) return -1;
    file->name = str_dup(filename);  /* Store only the filename, not full path */
    if (!file->name) return -1;
    /* Use st_mtim for precise modification time */
    file->tv_sec = st.st_mtim.tv_sec;
    file->tv_nsec = st.st_mtim.tv_nsec;
    file->is_dir = S_ISDIR(st.st_mode);
    return 0;
}

/* List contents of a directory */
int list_directory(const char *path, int show_hidden, int time_sort) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    t_file *files = NULL;
    int count = 0;
    int capacity = 0;
    int i = 0;
    
    if (!dir) return -1;
    
    /* Read all directory entries */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip hidden files unless -a flag is set */
        if (!show_hidden && entry->d_name[0] == '.') continue;
        
        /* Expand array if needed */
        if (count >= capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            t_file *new_files = malloc(sizeof(t_file) * capacity);
            if (!new_files) {
                free_files(files, count);
                closedir(dir);
                return -1;
            }
            /* Copy existing files to new array */
            i = 0;
            while (i < count) {
                new_files[i] = files[i];
                i++;
            }
            free(files);
            files = new_files;
        }
        
        /* Get file information with proper path construction */
        if (get_file_time(path, entry->d_name, &files[count]) == -1) {
            free_files(files, count);
            closedir(dir);
            return -1;
        }
        count++;
    }
    
    closedir(dir);
    
    /* Sort and print files */
    if (count > 0) {
        sort_files(files, count, time_sort);
        i = 0;
        while (i < count) {
            printf("%s\n", files[i].name);
            i++;
        }
    }
    
    free_files(files, count);
    return 0;
}

/* Print a single file name */
int list_file(const char *path) {
    printf("%s\n", path);
    return 0;
}

/* Main function */
int main(int argc, char **argv) {
    int show_hidden = 0;    /* -a flag */
    int time_sort = 0;      /* -t flag */
    int arg_start = 1;      /* Index of first non-option argument */
    t_file *files = NULL;   /* Array for non-directory files */
    t_file *dirs = NULL;    /* Array for directories */
    int file_count = 0;
    int dir_count = 0;
    int i = 1;
    int j;
    
    /* Parse command line options */
    while (i < argc && argv[i][0] == '-' && argv[i][1]) {
        j = 1;
        while (argv[i][j]) {
            if (argv[i][j] == 'a') show_hidden = 1;
            else if (argv[i][j] == 't') time_sort = 1;
            j++;
        }
        i++;
    }
    arg_start = i;
    
    /* If no arguments, list current directory */
    if (arg_start >= argc) {
        return list_directory(".", show_hidden, time_sort);
    }
    
    /* Allocate arrays for files and directories */
    files = malloc(sizeof(t_file) * argc);
    dirs = malloc(sizeof(t_file) * argc);
    if (!files || !dirs) {
        if (files) free(files);
        if (dirs) free(dirs);
        return 1;
    }
    
    /* Classify arguments as files or directories */
    i = arg_start;
    while (i < argc) {
        struct stat st;
        if (lstat(argv[i], &st) == -1) {
            i++;
            continue;
        }
        
        /* Separate directories from regular files */
        if (S_ISDIR(st.st_mode)) {
            if (get_file_info(argv[i], &dirs[dir_count]) == 0) {
                dir_count++;
            }
        } else {
            if (get_file_info(argv[i], &files[file_count]) == 0) {
                file_count++;
            }
        }
        i++;
    }
    
    /* Sort files and directories separately */
    sort_files(files, file_count, time_sort);
    sort_files(dirs, dir_count, time_sort);
    
    /* Display non-directory files first */
    i = 0;
    while (i < file_count) {
        list_file(files[i].name);
        i++;
    }
    
    /* Display directory contents */
    i = 0;
    while (i < dir_count) {
        /* Print directory name if multiple operands */
        if (file_count + dir_count > 1) {
            printf("%s:\n", dirs[i].name);
        }
        list_directory(dirs[i].name, show_hidden, time_sort);
        /* Add blank line between directories (except after last one) */
        if (i < dir_count - 1) {
            printf("\n");
        }
        i++;
    }
    
    /* Clean up */
    free_files(files, file_count);
    free_files(dirs, dir_count);
    return 0;
}