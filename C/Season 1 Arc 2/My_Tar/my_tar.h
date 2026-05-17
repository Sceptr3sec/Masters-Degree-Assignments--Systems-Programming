#ifndef MY_TAR_H
#define MY_TAR_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

/* Simple archive format (host-endian; ok for this assignment)
 * Magic: "MTAR"
 * For each entry:
 *   uint32_t name_len
 *   char     name[name_len]    // not NUL-terminated on disk
 *   uint64_t file_size
 *   uint64_t mtime             // seconds since epoch
 *   bytes    data[file_size]
 */

ssize_t robust_read (int fd, void *buf, size_t n);
ssize_t robust_write(int fd, const void *buf, size_t n);
int     safe_open    (const char *path, int flags, mode_t mode);
int     safe_close   (int fd);

void    write_all_fd (int fd, const char *s);
void    err_open_archive(const char *archive);
void    err_bad_archive (const char *archive);
void    err_stat_file   (const char *path);

int create_archive (const char *archive_name, int argc, char **argv, int start_index);
int append_archive (const char *archive_name, int argc, char **argv, int start_index);
int update_archive (const char *archive_name, int argc, char **argv, int start_index);
int list_archive   (const char *archive_name);
int extract_archive(const char *archive_name);

#endif
