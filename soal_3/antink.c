#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>#define FUSE_USE_VERSION 29
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define ORIGINAL_DIR "/mnt/original"
#define LOG_FILE "/mnt/logs/log.txt"

void write_log(const char *level, const char *msg) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log, "[%s] %04d-%02d-%02d %02d:%02d:%02d %s\n", level,
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, msg);
        fclose(log);
    }
}

int is_reversed(const char *path) {
    return strstr(path, "nafis") || strstr(path, "kimcun");
}

void reverse_name(const char *src, char *dest) {
    int len = strlen(src);
    for (int i = 0; i < len; i++) {
        dest[i] = src[len - i - 1];
    }
    dest[len] = '\0';
}

void rot13(char *buf, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (isalpha(buf[i])) {
            if ((buf[i] >= 'a' && buf[i] <= 'm') || (buf[i] >= 'A' && buf[i] <= 'M'))
                buf[i] += 13;
            else
                buf[i] -= 13;
        }
    }
}

void fullpath(char fpath[1024], const char *path) {
        if (strlen(path) > 1023) {
        fprintf(stderr, "Path terlalu panjang, dipotong\n");
    }

    const char *filename = path + 1;
    char original_name[1024];

    if (is_reversed(filename)) {
        reverse_name(filename, original_name);
        snprintf(fpath, 1024, "%s/%s", ORIGINAL_DIR, original_name);
    } else {
        snprintf(fpath, 1024, "%s%s", ORIGINAL_DIR, path);
    }
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    char fpath[1024];
    fullpath(fpath, path);
    int res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fpath[1024];

    (void) offset;
    (void) fi;

    fullpath(fpath, path);
    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char new_name[1024];
        if (is_reversed(de->d_name)) {
            reverse_name(de->d_name, new_name);
            filler(buf, new_name, &st, 0);
        } else {
            filler(buf, de->d_name, &st, 0);
        }
    }

    closedir(dp);
    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];
    fullpath(fpath, path);

    int res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    char fpath[1024];
    fullpath(fpath, path);

    FILE *fp = fopen(fpath, "r");
    if (!fp)
        return -errno;

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *tmp = (char *)malloc(fsize + 1);
    fread(tmp, 1, fsize, fp);
    fclose(fp);

    tmp[fsize] = '\0';

    if (!is_reversed(path) && strstr(path, ".txt")) {
        rot13(tmp, fsize);
    }

    if (offset < fsize) {
        if (offset + size > fsize)
            size = fsize - offset;
        memcpy(buf, tmp + offset, size);
    } else {
        size = 0;
    }

    free(tmp);

    char log_msg[2048];
    sprintf(log_msg, "READ: %s", path);
    write_log("INFO", log_msg);

    return size;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

int main(int argc, char *argv[]) {
    write_log("INFO", "Antink server berjalan...");
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>

void apply_rot13(char *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = buf[i];
        if (('a' <= c && c <= 'z'))
            buf[i] = 'a' + (c - 'a' + 13) % 26;
        else if (('A' <= c && c <= 'Z'))
            buf[i] = 'A' + (c - 'A' + 13) % 26;
    }
}

int is_dangerous(const char *name) {
    char lower[256];
    int i;
    for (i = 0; name[i] && i < 255; i++)
        lower[i] = tolower(name[i]);
    lower[i] = '\0';
    return strstr(lower, "nafis") || strstr(lower, "kimcun");
}

void reverse_name(const char *src, char *dst) {
    int len = strlen(src);
    for (int i = 0; i < len; i++)
        dst[i] = src[len - 1 - i];
    dst[len] = '\0';
}

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

        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, name);  
        } else {
            strncpy(name, de->d_name, sizeof(name));
            name[sizeof(name) - 1] = '\0';  
        }

        struct stat st;
        memset(&st, 0, sizeof(st));

        char fullentry[PATH_MAX];
        snprintf(fullentry, sizeof(fullentry), "%s/%s", fpath, de->d_name);

        if (de->d_type == DT_UNKNOWN) {
            if (lstat(fullentry, &st) == -1) {
                continue;  
            }
        } else {
            st.st_ino = de->d_ino;
            st.st_mode = de->d_type << 12;
        }

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
    return fuse_main(argc, argv, &antink_oper, NULL);
}
