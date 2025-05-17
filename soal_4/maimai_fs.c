#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <zlib.h>

#define AES_KEY_LEN 32
#define AES_BLOCK_SIZE 16

#define CHIHO_DIR "/home/chiwiw/sisop/modul_4/soal_4/chiho"

const unsigned char *aes_key = (const unsigned char *)"inikuncipanjangtepattigapuluhempat"; // 32 bytes

int is_starter_path(const char *path) {
    return strcmp(path, "/starter") == 0 || strncmp(path, "/starter/", 9) == 0;
}

int is_metro_path(const char *path) {
    return strcmp(path, "/metro") == 0 || strncmp(path, "/metro/", 7) == 0;
}

int is_dragon_path(const char *path) {
    return strcmp(path, "/dragon") == 0 || strncmp(path, "/dragon/", 8) == 0;
}

int is_blackrose_path(const char *path) {
    return strcmp(path, "/blackrose") == 0 || strncmp(path, "/blackrose/", 11) == 0;
}

int is_heaven_path(const char *path) {
    return strcmp(path, "/heaven") == 0 || strncmp(path, "/heaven/", 8) == 0;
}

int is_youth_path(const char *path) {
    return strcmp(path, "/youth") == 0 || strncmp(path, "/youth/", 7) == 0;
}

int is_prism_path(const char *path) {
    return strcmp(path, "/7sref") == 0 || strncmp(path, "/7sref/", 7) == 0;
}

const char *chiho_paths[] = {
    "/starter/",
    "/metro/",
    "/dragon/",
    "/blackrose/",
    "/heaven/",
    "/youth/"
};
const int chiho_count = 6;

char *find_prism_source_file(const char *path) {
    const char *filename = path + strlen("/7sref/");
    for (int i = 0; i < chiho_count; ++i) {
        char full[1024];

        // Starter: nama + .mai
        if (strcmp(chiho_paths[i], "/starter/") == 0) {
            snprintf(full, sizeof(full), "%s%s%s.mai", CHIHO_DIR, chiho_paths[i], filename);
        }
        // Metro: shift nama
        else if (strcmp(chiho_paths[i], "/metro/") == 0) {
            char shifted[256];
            shift_string(shifted, filename);
            snprintf(full, sizeof(full), "%s%s%s", CHIHO_DIR, chiho_paths[i], shifted);
        }
        else {
            snprintf(full, sizeof(full), "%s%s%s", CHIHO_DIR, chiho_paths[i], filename);
        }

        struct stat st;
        if (lstat(full, &st) == 0) {
            return strdup(full);
        }
    }
    return NULL; // tidak ditemukan
}

// Kompresi: compress2
int compress_buffer(const char *input, size_t input_len, char **output, size_t *output_len) {
    uLongf bound = compressBound(input_len);
    *output = malloc(bound);
    if (*output == NULL) return -1;

    int res = compress2((Bytef *)*output, &bound, (const Bytef *)input, input_len, Z_BEST_COMPRESSION);
    if (res != Z_OK) {
        free(*output);
        return -1;
    }

    *output_len = bound;
    return 0;
}

// Dekompresi: uncompress
int decompress_buffer(const char *input, size_t input_len, char **output, size_t expected_len) {
    *output = malloc(expected_len);
    if (*output == NULL) return -1;

    uLongf out_len = expected_len;
    int res = uncompress((Bytef *)*output, &out_len, (const Bytef *)input, input_len);
    if (res != Z_OK) {
        free(*output);
        return -1;
    }

    return out_len; // jumlah byte didekompres
}

int encrypt_aes(const unsigned char *plaintext, int plaintext_len, unsigned char *key,
    unsigned char *iv, unsigned char *ciphertext) {
EVP_CIPHER_CTX *ctx;
int len, ciphertext_len;

if (!(ctx = EVP_CIPHER_CTX_new())) return -1;
if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) return -1;
if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) return -1;
ciphertext_len = len;
if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) return -1;
ciphertext_len += len;
EVP_CIPHER_CTX_free(ctx);
return ciphertext_len;
}

int decrypt_aes(const unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
    unsigned char *iv, unsigned char *plaintext) {
EVP_CIPHER_CTX *ctx;
int len, plaintext_len;

if (!(ctx = EVP_CIPHER_CTX_new())) return -1;
if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) return -1;
if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) return -1;
plaintext_len = len;
if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) return -1;
plaintext_len += len;
EVP_CIPHER_CTX_free(ctx);
return plaintext_len;
}

void rot13_buffer(char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        if (c >= 'A' && c <= 'Z') {
            buf[i] = 'A' + (c - 'A' + 13) % 26;
        } else if (c >= 'a' && c <= 'z') {
            buf[i] = 'a' + (c - 'a' + 13) % 26;
        }
    }
}

void shift_string(char *dst, const char *src) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i] + (i % 256);
    }
    dst[len] = '\0';
}

void unshift_string(char *dst, const char *src) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i] - (i % 256);
    }
    dst[len] = '\0';
}

char *full_path(const char *fuse_path, int use_transformed_name) {
    char temp[8192];

    if (is_starter_path(fuse_path) && use_transformed_name) {
        snprintf(temp, sizeof(temp), "%s%s.mai", CHIHO_DIR, fuse_path);
    } else if (is_metro_path(fuse_path) && use_transformed_name) {
        const char *filename = strrchr(fuse_path, '/');
        if (filename != NULL) {
            char shifted[256], dir[4096];
            shift_string(shifted, filename + 1); // skip '/'
            strncpy(dir, fuse_path, filename - fuse_path + 1);
            dir[filename - fuse_path + 1] = '\0';
            snprintf(temp, sizeof(temp), "%s%s%s", CHIHO_DIR, dir, shifted);
        } else {
            snprintf(temp, sizeof(temp), "%s%s", CHIHO_DIR, fuse_path);
        }
    } else {
        snprintf(temp, sizeof(temp), "%s%s", CHIHO_DIR, fuse_path);
    }

    return strdup(temp);
}

// getattr
static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi;

    if (is_prism_path(path)) {
        char *real = find_prism_source_file(path);
        if (!real) return -ENOENT;
        int res = lstat(real, st);
        free(real);
        return (res == -1) ? -errno : 0;
    }

    int res;
    char *real_path = full_path(path, 0);
    res = lstat(real_path, st);
    free(real_path);

    if (res == -1 && is_starter_path(path)) {
        char *with_ext = full_path(path, 1);
        res = lstat(with_ext, st);
        free(with_ext);
    } else if (res == -1 && is_metro_path(path)) {
        char *shifted_path = full_path(path, 1);
        res = lstat(shifted_path, st);
        free(shifted_path);
    }

    if (res == -1)
        return -errno;
    return 0;
}

// readdir
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (strcmp(path, "/") == 0) {
        filler(buf, "starter", NULL, 0, 0);
        filler(buf, "metro", NULL, 0, 0);
        filler(buf, "dragon", NULL, 0, 0);
        filler(buf, "blackrose", NULL, 0, 0);
        filler(buf, "heaven", NULL, 0, 0);
        filler(buf, "youth", NULL, 0, 0);
        filler(buf, "7sref", NULL, 0, 0);
        return 0;
    }

    char *real_path = full_path(path, 0);
    DIR *dp = opendir(real_path);
    if (dp == NULL) {
        free(real_path);
        return -errno;
    }

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        if (is_starter_path(path)) {
            size_t len = strlen(de->d_name);
            if (len > 4 && strcmp(de->d_name + len - 4, ".mai") == 0) {
                char display_name[256];
                strncpy(display_name, de->d_name, len - 4);
                display_name[len - 4] = '\0';
                filler(buf, display_name, NULL, 0, 0);
            }
            continue;
        }

        if (is_metro_path(path)) {
            char unshifted[256];
            unshift_string(unshifted, de->d_name);
            filler(buf, unshifted, NULL, 0, 0);
            continue;
        }

        if (is_prism_path(path)) {
            filler(buf, ".", NULL, 0, 0);
            filler(buf, "..", NULL, 0, 0);
            
            char seen[1000][256]; // untuk hindari duplikat
            int seen_count = 0;
        
            for (int i = 0; i < chiho_count; ++i) {
                char base[512];
                snprintf(base, sizeof(base), "%s%s", CHIHO_DIR, chiho_paths[i]);
                DIR *dp = opendir(base);
                if (!dp) continue;
        
                struct dirent *de;
                while ((de = readdir(dp)) != NULL) {
                    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                        continue;
        
                    char nama[256];
                    if (strcmp(chiho_paths[i], "/starter/") == 0) {
                        size_t len = strlen(de->d_name);
                        if (len > 4 && strcmp(de->d_name + len - 4, ".mai") == 0) {
                            strncpy(nama, de->d_name, len - 4);
                            nama[len - 4] = '\0';
                        } else continue;
                    }
                    else if (strcmp(chiho_paths[i], "/metro/") == 0) {
                        unshift_string(nama, de->d_name);
                    }
                    else {
                        strncpy(nama, de->d_name, sizeof(nama));
                        nama[sizeof(nama) - 1] = '\0';
                    }
        
                    // Cek apakah sudah pernah ditampilkan
                    int already_seen = 0;
                    for (int j = 0; j < seen_count; ++j) {
                        if (strcmp(nama, seen[j]) == 0) {
                            already_seen = 1;
                            break;
                        }
                    }
        
                    if (!already_seen) {
                        strncpy(seen[seen_count++], nama, sizeof(nama));
                        filler(buf, nama, NULL, 0, 0);
                    }
                }
                closedir(dp);
            }
            return 0;
        }        
        filler(buf, de->d_name, NULL, 0, 0);
    }

    closedir(dp);
    free(real_path);
    return 0;
}

// open
static int fs_open(const char *path, struct fuse_file_info *fi) {
    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                            is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));

    int fd = open(real_path, fi->flags);
    free(real_path);
    if (fd == -1)
        return -errno;
    close(fd);
    return 0;
}

int do_actual_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                                 is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));
    int fd = open(real_path, O_RDONLY);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }

    int res = pread(fd, buf, size, offset);
    close(fd);
    free(real_path);
    if (res == -1) return -errno;

    // transformasi sesuai area
    if (is_dragon_path(path)) {
        rot13_buffer(buf, res);
    }
    else if (is_heaven_path(path)) {
        if (res <= AES_BLOCK_SIZE) return 0;
        unsigned char iv[AES_BLOCK_SIZE];
        unsigned char *plain = malloc(res);
        unsigned char *cipher = (unsigned char *)buf + AES_BLOCK_SIZE;

        memcpy(iv, buf, AES_BLOCK_SIZE);
        int decrypted_len = decrypt_aes(cipher, res - AES_BLOCK_SIZE, (unsigned char *)aes_key, iv, plain);
        if (decrypted_len < 0) {
            free(plain);
            return -EIO;
        }

        memcpy(buf, plain, decrypted_len);
        free(plain);
        return decrypted_len;
    }
    else if (is_youth_path(path)) {
        if (res <= sizeof(uint32_t)) return 0;
        uint32_t original_size;
        memcpy(&original_size, buf, sizeof(uint32_t));
        char *decompressed;
        int decompressed_len = decompress_buffer(buf + sizeof(uint32_t), res - sizeof(uint32_t), &decompressed, original_size);
        if (decompressed_len < 0) return -EIO;
        memcpy(buf, decompressed, decompressed_len);
        free(decompressed);
        return decompressed_len;
    }

    return res;
}

// read
static int fs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    (void) fi;
    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                            is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));

    int fd = open(real_path, O_RDONLY);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }

    int res = pread(fd, buf, size, offset);
    close(fd);
    free(real_path);

    if (res == -1) {
        return -errno;
    }
    
    if (is_dragon_path(path)) {
        rot13_buffer(buf, res);
    }

    if (is_heaven_path(path)) {
        if (res <= AES_BLOCK_SIZE) return 0;

        unsigned char iv[AES_BLOCK_SIZE];
        unsigned char *plain = malloc(res);
        unsigned char *cipher = (unsigned char *)buf + AES_BLOCK_SIZE;

        memcpy(iv, buf, AES_BLOCK_SIZE);
        int decrypted_len = decrypt_aes(cipher, res - AES_BLOCK_SIZE, (unsigned char *)aes_key, iv, plain);
        if (decrypted_len < 0) {
            free(plain);
            return -EIO;
        }

        memcpy(buf, plain, decrypted_len);
        free(plain);
        return decrypted_len;
    }

    if (is_youth_path(path)) {
        if (res <= sizeof(uint32_t)) return 0;
    
        // Baca size asli
        uint32_t original_size;
        memcpy(&original_size, buf, sizeof(uint32_t));
    
        // Dekompres sisanya
        char *decompressed;
        int decompressed_len = decompress_buffer(buf + sizeof(uint32_t), res - sizeof(uint32_t), &decompressed, original_size);
        if (decompressed_len < 0) {
            return -EIO;
        }
    
        memcpy(buf, decompressed, decompressed_len);
        free(decompressed);
        return decompressed_len;
    }
    
    if (is_prism_path(path)) {
        char *real = find_prism_source_file(path);
        if (!real) return -ENOENT;
    
        // Buat path palsu seperti seolah-olah user mengakses chiho langsung
        char fake_path[512];
        snprintf(fake_path, sizeof(fake_path), "%s", real + strlen(CHIHO_DIR));
        int ret = fs_read(fake_path, buf, size, offset, fi);
        free(real);
        return ret;
    }
    

    return res;
    
}

// write
static int fs_write(const char *path, const char *buf, size_t size,
    off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    if (is_prism_path(path))
    return -EROFS;


    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                            is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));

    int fd = open(real_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }

    const char *write_buf = buf;

    // ROT13 jika path ke /dragon/
    char *temp_buf = NULL;
    if (is_dragon_path(path)) {
        temp_buf = malloc(size);
        memcpy(temp_buf, buf, size);
        rot13_buffer(temp_buf, size);
        write_buf = temp_buf;
    }
    else if (is_heaven_path(path)) {
        unsigned char iv[AES_BLOCK_SIZE];
        RAND_bytes(iv, AES_BLOCK_SIZE); // generate IV

        unsigned char *cipher = malloc(size + AES_BLOCK_SIZE * 2); // allocate buffer
        int cipher_len = encrypt_aes((unsigned char *)buf, size, (unsigned char *)aes_key, iv, cipher + AES_BLOCK_SIZE);

        if (cipher_len < 0) {
            free(cipher);
            free(real_path);
            return -EIO;
        }

        memcpy(cipher, iv, AES_BLOCK_SIZE); // prepend IV

        int res = pwrite(fd, cipher, cipher_len + AES_BLOCK_SIZE, offset);
        free(cipher);
        close(fd);
        free(real_path);
        return res;
    }
    else if (is_youth_path(path)) {
        // Kompresi pakai zlib
        char *compressed;
        size_t compressed_len;
        if (compress_buffer(buf, size, &compressed, &compressed_len) != 0) {
            close(fd);
            free(real_path);
            return -EIO;
        }
    
        // Simpan size asli di awal file (4 byte)
        uint32_t original_size = (uint32_t)size;
        int res = pwrite(fd, &original_size, sizeof(uint32_t), offset);
        if (res != sizeof(uint32_t)) {
            free(compressed);
            close(fd);
            free(real_path);
            return -EIO;
        }
    
        // Simpan data terkompres setelah size
        res = pwrite(fd, compressed, compressed_len, offset + sizeof(uint32_t));
        free(compressed);
        close(fd);
        free(real_path);
        return res + sizeof(uint32_t);
    }    


    int res = pwrite(fd, write_buf, size, offset);
    close(fd);
    free(real_path);
    if (temp_buf) free(temp_buf);

    if (res == -1)
        return -errno;
    return res;
}


// create
static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    if (is_prism_path(path))
    return -EROFS;


    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                            is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));

    int fd = creat(real_path, mode);
    if (fd == -1) {
        free(real_path);
        return -errno;
    }
    close(fd);
    free(real_path);
    return 0;
}

// unlink
static int fs_unlink(const char *path) {
    if (is_prism_path(path))
    return -EROFS;

    
    char *real_path = full_path(path, is_starter_path(path) || is_metro_path(path) ||
                            is_dragon_path(path) || is_heaven_path(path) || is_youth_path(path));

    int res = unlink(real_path);
    free(real_path);
    if (res == -1)
        return -errno;
    return 0;
}

static const struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
    .write   = fs_write,
    .create  = fs_create,
    .unlink  = fs_unlink,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fs_oper, NULL);
}
