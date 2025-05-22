# Laporan Praktikum Sisop Modul 4 Kelompok IT21
# Anggota
1. Nisrina Bilqis - 5027241054
2. Hanif Mawla Faizi - 5027241064
3. Dina Rahmadani - 5027241065

---

# Soal 1
---


---
# Soal 2
---
## Deskripsi Soal
Diberikan direktori relics/ berisi pecahan file gambar Baymax.jpeg.000 hingga .013. Buatlah filesystem virtual menggunakan FUSE dengan nama Baymax yang:
1. Membaca file Baymax.jpeg yang telah dipotong-potong menjadi 14 file `.000`, `.001`, ..., `.013`
2. Mengizinkan pengguna membuat file baru dan menyimpannya dalam bentuk pecahan `.000,` `.001,` dst.
3. Membaca kembali file-file tersebut seolah utuh
4. Menyediakan log seluruh aktivitas sistem file dalam `activity.log`

## Struktur Direktori
```bash
soal_2/
├── baymax.c           <-- source code
├── relics/            <-- folder berisi pecahan file .000, .001, ...
├── mount_dir/         <-- direktori mount FUSE
├── activity.log       <-- file log aktivitas filesystem

```
## Pengerjaan 
### a. Menampilkan File Virtual Baymax.jpeg
📄 Tujuan:
Menggabungkan file pecahan Baymax.jpeg.000 hingga .013 secara virtual ke dalam 1 file Baymax.jpeg di mount point.
🧠 Konsep:
`x_getattr()` mengatur atribut file virtual
`x_readdir()` menampilkan nama file
`x_read()` menggabungkan isi dari pecahan `.000`, `.001`, ..., `.013`
Fungsi pembantu `calculate_total_size()` menghitung ukuran file utuh

Kode :
```c
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
}
```
```c
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

    return -ENOENT;
}
```
```c
static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, virtual_filename, NULL, 0);

    return 0;
}
```
```c
static int x_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    const char *filename = path + 1;

    if (strcmp(filename, virtual_filename) != 0)
        return -ENOENT;

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
```

### b. Menyimpan File Baru secara Terpecah
📄 Tujuan:
Menangani operasi penulisan (write) ke file baru yang dibuat pengguna. Data ditulis dalam potongan 1KB, disimpan sebagai file `.000`, `.001`, dll di `relics/`.
🧠 Konsep:
`x_create()` menyimpan nama file yang dibuat
`x_write()` memecah data dan menulisnya ke banyak file
Kode :
```c
static int x_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
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
```
```c
static int x_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int part_index = offset / 1024;
    int offset_in_part = offset % 1024;
    size_t bytes_written = 0;

    int first_part = part_index;
    int last_part = -1;

    while (bytes_written < size) {
        char part_path[512];
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", RELIC_DIR, latest_created_file, part_index);

        FILE *fp = fopen(part_path, offset_in_part == 0 ? "wb" : "r+b");
        if (!fp) return -EIO;

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
```

### c. Menghapus Semua File Pecahan
📄 Tujuan:
Saat pengguna menghapus file di mount point, semua file pecahannya di `relics/` juga harus dihapus.
🧠 Konsep:
`x_unlink()` akan memanggil `remove()` pada semua file pecahan berdasarkan nama
Kode :
```c
static int x_unlink(const char *path) {
    const char *filename = path + 1;
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
```

### d. Logging ke activity.log
📄 Tujuan:
Semua aksi seperti CREATE, WRITE, READ, DELETE harus tercatat ke dalam file activity.log secara terstruktur.
🧠 Konsep:
Fungsi `write_log()` digunakan untuk mencatat waktu + aksi ke file log
`x_create`, `x_write`, `x_read`, `x_unlink` memanggil `write_log()`
Kode :
```c
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
```

---
# Soal 3
---


---
# Soal 4
---

