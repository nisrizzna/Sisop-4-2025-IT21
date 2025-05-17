#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

char RELIC_DIR[512];
char LOG_FILE[512];
char latest_created_file[256];
char created_files[100][256];
int created_count = 0;

static const char *virtual_filename = "Baymax.jpeg";
static off_t total_size = 0;

char last_read_file[256] = "";

void write_log(const char *action, const char *desc);

void calculate_total_size() {
    total_size = 0;
    char path[512];
    for (int i = 0; i < 100; i++) {
        snprintf(path, sizeof(path), "%s/Baymax.jpeg.%03d", RELIC_DIR, i);
        FILE *fp = fopen(path, "rb");
        if (!fp)
            break;
        fseek(fp, 0, SEEK_END);
        total_size += ftell(fp);
        fclose(fp);
    }
    printf("[DEBUG] total_size calculated = %lld bytes\n", (long long)total_size);
}

static int x_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path + 1, virtual_filename) == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = total_size;
        return 0;
    }

    for (int i = 0; i < created_count; i++) {
        if (strcmp(path + 1, created_files[i]) == 0) {
            stbuf->st_mode = S_IFREG | 0644;
            stbuf->st_nlink = 1;

            int size = 0;
            char part_path[512];
            for (int j = 0; j < 100; j++) {
                snprintf(part_path, sizeof(part_path), "%s/%s.%03d", RELIC_DIR, created_files[i], j);
                FILE *fp = fopen(part_path, "rb");
                if (!fp) break;
                fseek(fp, 0, SEEK_END);
                size += ftell(fp);
                fclose(fp);
            }

            stbuf->st_size = size;
            return 0;
        }
    }

    return -ENOENT;
}

static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, virtual_filename, NULL, 0);

    for (int i = 0; i < created_count; i++) {
        filler(buf, created_files[i], NULL, 0);
    }

    return 0;
}

static int x_open(const char *path, struct fuse_file_info *fi) {
    const char *filename = path + 1;
    if (strcmp(filename, virtual_filename) == 0)
        return 0;

    for (int i = 0; i < created_count; i++) {
        if (strcmp(filename, created_files[i]) == 0)
            return 0;
    }
    return -ENOENT;
}

static int x_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("[DEBUG] create file: %s\n", path + 1);
    strncpy(latest_created_file, path + 1, sizeof(latest_created_file));

    if (created_count < 100) {
        strncpy(created_files[created_count], path + 1, sizeof(created_files[0]));
        created_count++;
    }

    char logdesc[256];
    snprintf(logdesc, sizeof(logdesc), "%s", path + 1);
    write_log("CREATE", logdesc);

    return 0;
}

static int x_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("[DEBUG] write to file: %s (offset: %lld, size: %zu)\n", latest_created_file, (long long)offset, size);

    int part_index = offset / 1024;
    int offset_in_part = offset % 1024;
    size_t bytes_written = 0;

    int first_part = part_index;
    int last_part = -1;

    while (bytes_written < size) {
        char part_path[512];
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", RELIC_DIR, latest_created_file, part_index);

        FILE *fp = fopen(part_path, offset_in_part == 0 ? "wb" : "r+b");
        if (!fp) {
            perror("[ERROR] fopen");
            return -EIO;
        }

        fseek(fp, offset_in_part, SEEK_SET);

        int chunk_size = 1024 - offset_in_part;
        if (chunk_size > size - bytes_written)
            chunk_size = size - bytes_written;

        fwrite(buf + bytes_written, 1, chunk_size, fp);
        fclose(fp);

        bytes_written += chunk_size;
        offset_in_part = 0;
        last_part = part_index;
        part_index++;
    }

    char parts[512] = "";
    for (int i = first_part; i <= last_part; i++) {
        char p[32];
        snprintf(p, sizeof(p), "%s.%03d", latest_created_file, i);
        strcat(parts, i == first_part ? p : ", ");
        strcat(parts, p);
    }

    char logdesc[512];
    snprintf(logdesc, sizeof(logdesc), "%s -> %s", latest_created_file, parts);
    write_log("WRITE", logdesc);

    return bytes_written;
}

static int x_unlink(const char *path) {
    const char *filename = path + 1;
    printf("[DEBUG] unlink file: %s\n", filename);

    char part_path[512];
    int last_index = -1;

    for (int i = 0; i < 100; i++) {
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", RELIC_DIR, filename, i);
        if (access(part_path, F_OK) != 0)
            break;
        remove(part_path);
        last_index = i;
    }

    for (int i = 0; i < created_count; i++) {
        if (strcmp(created_files[i], filename) == 0) {
            for (int j = i; j < created_count - 1; j++) {
                strcpy(created_files[j], created_files[j + 1]);
            }
            created_count--;
            break;
        }
    }

    if (last_index >= 0) {
        char logdesc[256];
        snprintf(logdesc, sizeof(logdesc), "%s.000 - %s.%03d", filename, filename, last_index);
        write_log("DELETE", logdesc);
    }

    return 0;
}

static int x_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    const char *filename = path + 1;
    printf("[DEBUG] x_read() dipanggil untuk %s\n", filename);

    if (offset == 0 && strcmp(last_read_file, filename) != 0) {
        snprintf(last_read_file, sizeof(last_read_file), "%s", filename);
        char logdesc[256];
        snprintf(logdesc, sizeof(logdesc), "%s", filename);
        write_log("READ", logdesc);
    }

    char part_path[512];
    int total_read = 0;
    int part_index = 0;
    off_t curr_offset = 0;
    size_t bytes_to_read = size;

    while (bytes_to_read > 0) {
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", RELIC_DIR, filename, part_index);
        FILE *fp = fopen(part_path, "rb");
        if (!fp) break;

        fseek(fp, 0, SEEK_END);
        long part_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset >= curr_offset + part_size) {
            curr_offset += part_size;
            fclose(fp);
            part_index++;
            continue;
        }

        int start = (offset > curr_offset) ? offset - curr_offset : 0;
        int can_read = part_size - start;
        if (can_read > bytes_to_read) can_read = bytes_to_read;

        fseek(fp, start, SEEK_SET);
        size_t bytes_read = fread(buf + total_read, 1, can_read, fp);
        fclose(fp);

        if (bytes_read != can_read) return -EIO;

        total_read += bytes_read;
        bytes_to_read -= bytes_read;
        curr_offset += part_size;
        part_index++;
    }

    return total_read;
}

static struct fuse_operations operations = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open    = x_open,
    .read    = x_read,
    .create  = x_create,
    .write   = x_write,
    .unlink  = x_unlink
};

void write_log(const char *action, const char *desc) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);

    fprintf(fp, "[%s] %s: %s\n", timebuf, action, desc);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    char cwd[512];
    getcwd(cwd, sizeof(cwd));

    snprintf(RELIC_DIR, sizeof(RELIC_DIR), "%s/relics", cwd);
    snprintf(LOG_FILE, sizeof(LOG_FILE), "%s/activity.log", cwd);

    calculate_total_size();
    return fuse_main(argc, argv, &operations, NULL);
}
