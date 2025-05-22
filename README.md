# Laporan Praktikum Sisop Modul 4 Kelompok IT21
# Anggota
1. Nisrina Bilqis - 5027241054
2. Hanif Mawla Faizi - 5027241064
3. Dina Rahmadani - 5027241065

---

# Soal 1

## Deskripsi Soal
Pada praktikum ini, diminta untuk membuat sebuah program C yang dapat membaca file teks berisi data hexadecimal di dalam folder anomali, lalu mengubah data tersebut menjadi file gambar berekstensi .png yang disimpan di dalam folder anomali/image. Selain itu, setiap proses konversi harus dicatat ke dalam file log bernama conversion.log yang juga berada di dalam folder anomali.

## Deskripsi Program
Program ini melakukan proses sebagai berikut:

1. Membuat folder output jika belum ada.
2. Membuka folder input anomali, lalu membaca semua file .txt di dalamnya.
3. Setiap file .txt berisi representasi data gambar dalam format hexadecimal.
4. Setiap dua karakter hexadecimal dibaca dan diubah menjadi byte menggunakan fungsi hex_to_byte.
5. Data hasil konversi ditulis ke file gambar .png di folder image.
6. Setiap proses konversi dicatat ke dalam file log conversion.log dengan timestamp.
7. Setelah selesai, program menutup semua file dan folder yang digunakan.

## Penjelasan Fungsi dan Prosedur

1. ```unsigned char hex_to_byte(const char *hex)```
Mengonversi 2 karakter hexadecimal menjadi 1 byte data.

2. ```void get_timestamp(char *date_str, char *time_str)```
Mengambil waktu lokal saat ini dan menyimpannya dalam format string tanggal dan waktu.

3. Fungsi utama ```main()```
Melakukan seluruh proses mulai dari pengecekan folder, pembacaan file, konversi, pembuatan file hasil, dan pencatatan log.

## Kelebihan Program

Program konversi file hexadecimal ke file gambar PNG ini memiliki beberapa kelebihan yang dapat menunjang kemudahan dan keamanan dalam proses pengolahan data. Berikut beberapa kelebihannya:

1. Proses Otomatis dan Massal
Program dapat memproses banyak file sekaligus dalam satu kali eksekusi tanpa perlu menambahkan nama file secara manual, sehingga sangat efisien untuk jumlah file yang besar.
2. Pembuatan Folder Output Otomatis
Program secara otomatis membuat folder output (```anomali/image```) apabila belum tersedia, sehingga tidak perlu melakukan pembuatan folder manual terlebih dahulu.
3. Pembuatan File Log Terstruktur
Setiap proses konversi dicatat secara rapi ke dalam file ```conversion.log``` beserta waktu dan nama file yang diproses, sehingga dapat memudahkan proses tracking data.
4. Penerapan Timestamp Real-Time
Setiap file hasil konversi diberi timestamp berupa tanggal dan waktu saat file diproses. Hal ini sangat berguna untuk membedakan file hasil konversi yang dilakukan di waktu berbeda.
5. Manajemen Memori yang Baik
Program menggunakan alokasi memori dinamis sesuai kebutuhan dan melakukan dealokasi setelah penggunaan untuk menghindari kebocoran memori (memory leak).

## Revisi Soal

Pada implementasi awal program, terdapat kendala di mana program tidak dapat membaca semua file yang terdapat dalam folder ```anomali``` Setelah ditelusuri, penyebabnya adalah karena program hanya memproses file yang memiliki ekstensi ```.txt```. Hal ini dikarenakan adanya kondisi berikut pada bagian loop pembacaan direktori:

```if (strstr(entry->d_name, ".txt") == NULL) continue;```
Kondisi ini menyebabkan file yang tidak memiliki ekstensi .txt akan dilewati dan tidak diproses.

Untuk mengatasi permasalahan tersebut, dilakukan revisi pada bagian pengecekan nama file. Supaya program dapat membaca semua file dalam folder anomali tanpa peduli ekstensi, maka kondisi tersebut dihapus atau dimodifikasi sesuai kebutuhan

### Menghapus pengecekan ekstensi
Jika memang semua file di folder anomali ingin diproses:

```
// while ((entry = readdir(dir)) != NULL) {
//     if (strstr(entry->d_name, ".txt") == NULL) continue;
while ((entry = readdir(dir)) != NULL) {
```


---
# Soal 2
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
##🐧 AntiNK: Sistem Perlindungan File dari Nafis & Kimcun menggunakan FUSE + Docker

AntiNK adalah proyek berbasis FUSE dalam container Docker yang bertugas:

- Membalik nama file yang mencurigakan (nafis, kimcun)
- Mengenkripsi isi file .txt pakai ROT13
- Mencatat semua aktivitas ke dalam log

## 🧱 Struktur Dockerfile
Dockerfile
```FROM ubuntu:20.04```

▶️ Memakai image Ubuntu 20.04 sebagai basis.

Dockerfile
```ENV DEBIAN_FRONTEND=noninteractive```

▶️ Supaya apt gak nanya-nanya interaktif (biar auto saat build).

Dockerfile
```RUN apt-get update && \
    apt-get install -y fuse gcc make libfuse-dev pkg-config```

▶️ Instal tools penting:

- fuse: untuk sistem file user-space
- gcc + make: untuk kompilasi C
- libfuse-dev: pustaka pengembangan FUSE
- pkg-config: bantu cari flag FUSE saat compile

Dockerfile
```RUN mkdir /mnt/antink_mount /mnt/original /mnt/logs```

▶️ Buat 3 direktori mount di dalam container:

- antink_mount: hasil mount sistem file
- original: tempat file asli
- logs: tempat log dicatat

Dockerfile
```COPY antink.c .```

▶️ Salin file sumber antink.c ke dalam image.

Dockerfile
```RUN gcc -D_FILE_OFFSET_BITS=64 -Wall antink.c $(pkg-config fuse --cflags --libs) -o /antink```

▶️ Compile antink.c menjadi executable /antink dengan flag untuk dukung file besar.

Dockerfile
``VOLUME ["/mnt/antink_mount", "/mnt/original", "/mnt/logs"]```

▶️ Tentukan direktori yang akan dimount dari luar ke container.

Dockerfile
```CMD ["/antink", "/mnt/antink_mount", "-f"]```

▶️ Jalankan FUSE dengan mount point /mnt/antink_mount secara foreground (-f).

📦 docker-compose.yml Breakdown
```services:
  antink:
    build: .
    container_name: antink_container```

▶️ Buat container bernama antink_container dari Dockerfile di direktori ini.

```    cap_add:
      - SYS_ADMIN
    security_opt:
      - apparmor:unconfined
    devices:
      - /dev/fuse```

▶️ Hak istimewa agar FUSE bisa jalan:

- SYS_ADMIN: dibutuhkan oleh FUSE
- apparmor:unconfined: agar container bisa akses FUSE device
- devices: mount /dev/fuse dari host

```    volumes:
      - ./antink_mount:/mnt/antink_mount
      - ./it24_host:/mnt/original
      - ./antink-logs:/mnt/logs```

▶️ Hubungkan direktori di host ke dalam container:

- it24_host: tempat file asli
- antink-logs: untuk log aktivitas
- antink_mount: mount hasil sistem FUSE

```    tty: true```

▶️ Agar container punya terminal aktif.

## 🧠 Penjelasan Kode antink.c

🔍 Fungsi Kecil Tapi Penting
```void write_log(const char *level, const char *msg)```

▶️ Fungsi nulis log ke file /mnt/logs/log.txt. Format: [LEVEL] YYYY-MM-DD HH:MM:SS Pesan.

```int is_reversed(const char *path)```

▶️ Cek apakah nama file mengandung kata "nafis" atau "kimcun". Return 1 kalau iya.

```void reverse_name(const char *src, char *dest)```

▶️ Balik string dari kanan ke kiri (untuk file "berbahaya").
```void rot13(char *buf, size_t size)```

▶️ Enkripsi isi buffer pakai ROT13 (setengah Caesar cipher, geser 13 huruf).

## 📁 fullpath()

```void fullpath(char fpath[1024], const char *path)```

▶️ Buat path absolut ke file di ORIGINAL_DIR.

Kalau namanya mencurigakan, dibalik dulu.
Misal: /hello/nafis.txt → cari file /mnt/original/tsixfisan

## 📂 xmp_getattr()

```static int xmp_getattr(const char *path, struct stat *stbuf)```

▶️ Panggilan stat() buat ambil metadata file (ukuran, tipe, dll).

FUSE butuh ini buat validasi semua operasi.

## 📜 xmp_readdir()

```static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, ...)```

▶️ Tampilkan isi direktori:

Kalau file mengandung "nafis" atau "kimcun", tampilkan versi dibalik
Misal: di direktori asli ada nafis.txt → ditampilkan sebagai txt.sifan

## 📖 xmp_open()

```static int xmp_open(const char *path, struct fuse_file_info *fi)```

▶️ Coba buka file dengan flag tertentu. Return error kalau gagal.

## 📘 xmp_read()

```static int xmp_read(const char *path, char *buf, size_t size, off_t offset, ...)```

▶️ Baca isi file:

Kalau file .txt dan tidak mencurigakan, isinya dienkripsi pakai ROT13
Setelah dibaca, log ditulis: READ: /namafile

## 🚀 main()

```int main(int argc, char *argv[])```

▶️ Jalankan FUSE dan tulis log saat server aktif.

## 🧪 Contoh Output

```[INFO] 2025-05-22 19:30:45 Antink server berjalan...
[INFO] 2025-05-22 19:31:01 READ: /halo.txt
[INFO] 2025-05-22 19:31:18 READ: /sifank.txt```

---
# Soal 4
## Deskripsi Soal
Disini soal meminta untuk membuat sebuah filesystem virtual menggunakan FUSE (Filesystem in Userspace) dengan beberapa folder khusus yang merepresentasikan area dalam game Maimai. Setiap folder memiliki karakteristik dan behavior tersendiri terhadap file di dalamnya, seperti perubahan nama file, isi file, enkripsi, kompresi, hingga akses read-only.

 Struktur utama direktori adalah sebagai berikut
```bash
chiho/
├── starter/
├── metro/
├── dragon/
├── blackrose/
├── heaven/
├── youth/
```
FUSE mount point: `fuse_dir/`

### a. Starter : Penanganan Ekstensi `.mai`
File yang ditulis ke direktori `starter` di mount point `fuse_dir/` akan disimpan ke `chiho/starter` dengan ekstensi `.mai.` Namun, ekstensi ini tidak boleh terlihat dari sisi pengguna di `fuse_dir`.

**Implementasi:**
Pada `full_path`, ditambahkan `.mai` saat write dan read:
```c
if (is_starter_path(fuse_path) && transformed) {
    ...
    snprintf(temp, sizeof(temp), "%s%s%s.mai", CHIHO_DIR, dir, name);
    return strdup(temp);
}
```
Dan di `fs_readdir`, nama file dipotong sebelum `.mai`:
```c
if (len > 4 && strcmp(de->d_name + len - 4, ".mai") == 0) {
    strncpy(display_name, de->d_name, len - 4);
    display_name[len - 4] = '\0';
    filler(buf, display_name, NULL, 0, 0);
}
```

### b. Metro : String Shifting pada Isi File
Isi file di metro akan di-shift saat ditulis dan di-unshift saat dibaca. Nama file tetap.

**Implementasi:**
```c
// Saat write
else if (is_metro_path(path)) {
    shift_string(temp, temp);
    res = pwrite(fd, temp, size, offset);
}

// Saat read
else if (is_metro_path(path)) {
    unshift_string(buf, buf);
}
```

### c. Dragon: ROT13 Enkripsi
Isi file akan di-enkripsi menggunakan ROT13 saat write, dan di-dekripsi saat read.

**Implementasi:**
```c
// fs_write
rot13_buffer(temp, size);

// fs_read
rot13_buffer(buf, res);
```

### d. Heaven: Enkripsi AES 256 CBC
Isi file dienkripsi dengan AES 256 CBC saat disimpan, menggunakan IV acak yang disisipkan di awal file.

**Implementasi:**
```c
// fs_write
RAND_bytes(iv, AES_BLOCK_SIZE);
cipher_len = encrypt_aes(...);
memcpy(cipher, iv, AES_BLOCK_SIZE);
```
```c
// fs_read
memcpy(iv, buf, AES_BLOCK_SIZE);
decrypted_len = decrypt_aes(...);
```
### e. Youth: Kompresi zlib (gzip)
Isi file dikompresi menggunakan zlib dan disimpan bersama ukuran asli (4 byte di awal).

**Implementasi:**
```c
// fs_write
uint32_t original_size = (uint32_t)size;
pwrite(fd, &original_size, sizeof(uint32_t), offset);
pwrite(fd, compressed, compressed_len, offset + sizeof(uint32_t));
```
```c
// fs_read
memcpy(&original_size, buf, sizeof(uint32_t));
decompressed_len = decompress_buffer(...);
```

### f. Prism (7sref): Read-Only Aggregator
Prism (`/7sref`) adalah direktori baca-saja yang menggabungkan isi dari semua folder lain (`starter`, `metro`, dst) ke dalam satu view. File ditampilkan dalam bentuk hasil dekripsi/dekompresi/asli sesuai lokasi aslinya.

**Implementasi:**
1. Fungsi `fs_readdir` mengumpulkan nama file unik dari seluruh folder `chiho_*`.
2. Fungsi `fs_read` meneruskan akses ke path chiho sebenarnya:
```c
char *real = find_prism_source_file(path);
snprintf(fake_path, sizeof(fake_path), "%s", real + strlen(CHIHO_DIR));
int res = fs_read(fake_path, buf, size, offset, fi);
```

## Testing :
```bash
# Starter
echo "halo starter" > fuse_dir/starter/test.txt
cat chiho/starter/test.mai     # hasil tersimpan
cat fuse_dir/starter/test.txt  # terlihat normal

# Metro
echo "hello metro" > fuse_dir/metro/test.txt
cat chiho/metro/test.txt       # isi dalam bentuk shifted
cat fuse_dir/metro/test.txt    # terlihat normal

# Dragon
echo "abc xyz" > fuse_dir/dragon/test.txt
cat fuse_dir/dragon/test.txt   # otomatis ROT13: nop klm

# Heaven
echo "secret heaven" > fuse_dir/heaven/test.txt
cat fuse_dir/heaven/test.txt   # otomatis terdekripsi

# Youth
echo "compress this" > fuse_dir/youth/test.txt
cat fuse_dir/youth/test.txt    # otomatis didekompres

# Prism
cat fuse_dir/7sref/test.txt    # akan tampil dari salah satu area chiho
```
