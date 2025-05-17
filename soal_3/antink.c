#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>


// ROT13 function
void apply_rot13(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = buf[i];
        if (('a' <= c && c <= 'z'))
            buf[i] = 'a' + (c - 'a' + 13) % 26;
        else if (('A' <= c && c <= 'Z'))
            buf[i] = 'A' + (c - 'A' + 13) % 26;
    }
}

// Check if a file is "berbahaya"
int is_dangerous(const char *name) {
    char lower[256];
    int i;
    for (i = 0; name[i] && i < 255; i++)
        lower[i] = tolower(name[i]);
    lower[i] = '\0';
    return strstr(lower, "nafis") || strstr(lower, "kimcun");
}

// Balik nama file
void reverse_name(const char *src, char *dst) {
    int len = strlen(src);
    for (int i = 0; i < len; i++)
        dst[i] = src[len - 1 - i];
    dst[len] = '\0';
}

// Logging
void write_log(const char *path, const char *action) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t) - 1] = '\0'; // hapus newline

    fprintf(log, "[%s] %s %s\n", t, action, path);
    fclose(log);
}

void full_path(char fpath[PATH_MAX], const char *path) {
    if (strcmp(path, "/") == 0) {
        snprintf(fpath, PATH_MAX, "/home/nisrizzna/s3m4/uai");
    } else {
        snprintf(fpath, PATH_MAX, "/home/nisrizzna/s3m4/uai%s", path);
    }
}

static int antink_getattr(const char *path, struct stat *stbuf) {
    char fpath[PATH_MAX];
    full_path(fpath, path);
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fpath[PATH_MAX];
    full_path(fpath, path);

    dp = opendir(fpath);
    if (dp == NULL) return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {
        char name[256];

        // Cek apakah nama file berbahaya
        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, name);  // Misalnya: kimcun.txt -> txt.nucmik
        } else {
            strncpy(name, de->d_name, sizeof(name));
            name[sizeof(name) - 1] = '\0';  // Pastikan null-terminated
        }

        struct stat st;
        memset(&st, 0, sizeof(st));

        // Buat full path ke file entry
        char fullentry[PATH_MAX];
        snprintf(fullentry, sizeof(fullentry), "%s/%s", fpath, de->d_name);

        // Handle DT_UNKNOWN dengan fallback ke lstat
        if (de->d_type == DT_UNKNOWN) {
            if (lstat(fullentry, &st) == -1) {
                continue;  // skip jika gagal
            }
        } else {
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
        }

        // Masukkan ke buffer readdir
        if (filler(buf, name, &st, 0)) break;
    }

    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    full_path(fpath, path);
    int res = open(fpath, fi->flags);
    if (res == -1) return -errno;

    close(res);
    write_log(path, "OPEN");

    if (is_dangerous(path)) {
        write_log(path, "[WARNING] DANGEROUS FILE ACCESSED!");
    }

    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    full_path(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);

    if (!is_dangerous(path) && res > 0) {
        apply_rot13(buf, res);
    }

    write_log(path, "READ");
    return res;
}

static int antink_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s%s", SOURCE_DIR, path);

    int fd = open(fullpath, O_CREAT | O_WRONLY, mode);
    if (fd == -1) return -errno;

    close(fd);
    write_log(path, "CREATE");
    return 0;
}

static int antink_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s%s", SOURCE_DIR, path);

    int fd = open(fullpath, O_WRONLY);
    if (fd == -1) return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);
    write_log(path, "WRITE");
    return res;
}

static int antink_unlink(const char *path) {
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s%s", SOURCE_DIR, path);

    int res = unlink(fullpath);
    if (res == -1) return -errno;

    write_log(path, "UNLINK");
    return 0;
}

static int antink_mkdir(const char *path, mode_t mode) {
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s%s", SOURCE_DIR, path);

    int res = mkdir(fullpath, mode);
    if (res == -1) return -errno;

    write_log(path, "MKDIR");
    return 0;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
    .write   = antink_write,
    .create  = antink_create,
    .unlink  = antink_unlink,
    .mkdir   = antink_mkdir,
};

int main(int argc, char *argv[]) {
    // Pastikan folder SOURCE_DIR sudah ada dan bisa diakses
    return fuse_main(argc, argv, &antink_oper, NULL);
}
