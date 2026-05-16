# SISOP-4-2026-IT-002
## Soal 1
Pada soal ini diminta untuk membuat sebuah filesystem virtual menggunakan FUSE (Filesystem in Userspace). Filesystem virtual ini digunakan untuk melakukan mount terhadap folder asli bernama `amba_files` ke sebuah mount point bernama `mnt`. Seluruh file yang berada di dalam `amba_files` harus dapat diakses melalui folder mount tersebut tanpa memindahkan file aslinya.
Selain itu, program juga diminta untuk membuat file virtual tambahan bernama `tujuan.txt`. File ini tidak boleh ada secara fisik di dalam folder `amba_files`, melainkan dibuat langsung oleh program FUSE saat filesystem diakses. Dengan demikian, user akan melihat seolah-olah file tersebut benar-benar ada pada filesystem.
Melalui soal ini dipelajari beberapa konsep penting pada sistem operasi Linux, seperti:
- Filesystem virtual menggunakan FUSE
- Konsep mount point
- Callback filesystem pada userspace
- Operasi dasar filesystem seperti `getattr`, `readdir`, `open`, dan `read`
- Passthrough filesystem

### Penjelasan Program
Program bekerja dengan cara:
- Linux melakukan mount ke folder mnt
- Semua request filesystem dialihkan ke program FUSE
- Program membaca file asli dari amba_files
- Program membuat file virtual tujuan.txt
- User hanya melihat hasil virtual filesystem
### Kode Lengkap
```c
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

static const char *source_dir = "amba_files";

static int kenz_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1000];

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path, "/tujuan.txt") == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 33;
        return 0;
    }

    sprintf(fpath, "%s%s", source_dir, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}
```
### Penjelasan Kode
#### 1. FUSE Version
```
#define FUSE_USE_VERSION 28
```
Digunakan untuk menentukan versi API FUSE yang dipakai.

#### 2. Source Directory
```
static const char *source_dir = "amba_files";
```
Menentukan lokasi file asli.
Semua file asli akan dibaca dari folder:
```
amba_files/
```
#### 3. getattr()
```
static int kenz_getattr(...)
```
Fungsi ini dipanggil Linux setiap kali:
- mengecek file
- melakukan ls
- membuka file

Fungsi ini menentukan:
- apakah file ada
- apakah file folder atau regular file
- ukuran file
- permission file
#### 4. Virtual File
```
if (strcmp(path, "/tujuan.txt") == 0)
```
Digunakan untuk membuat file virtual.
File ini:
- tidak ada secara fisik
- hanya muncul di mountpoint
- dibuat langsung oleh program
#### 5. readdir()
```
static int kenz_readdir(...)
```
Fungsi ini dipanggil saat:
```
ls mnt
```
Program membaca seluruh isi:
```
amba_files
```
lalu menampilkannya di:
```
mnt
```
#### 6. filler()
```
filler(buf, de->d_name, NULL, 0);
```
Digunakan untuk menambahkan file ke tampilan directory.
#### 7. open()
```
static int kenz_open(...)
```
Digunakan saat file dibuka.
Program akan membuka file asli dari:
```
amba_files
```
#### 8. read()
```
static int kenz_read(...)
```
Digunakan untuk membaca isi file.
Fungsi ini:
- membaca file asli
- mengirim isi file ke user

### Cara Menjalankan
#### compile
```
gcc kenz_rescue.c -o kenz_rescue `pkg-config fuse --cflags --libs`
```
#### Mount
```
mkdir -p mnt
./kenz_rescue mnt
```
#### Testing
```
ls mnt
cat mnt/1.txt
cat mnt/tujuan.txt
```
#### Unmount
```
fusermount -u mnt
```
### Output
#### 1. Menjalankan Program FUSE
![Output Mode A](assets/soal_1/runprogram.png)
#### 2. Melihat Isi File Pada Mount Point
![Output Mode A](assets/soal_1/ls_mnt.png)
#### 3. Memastikan File Asli Pada `amba_files`
![Output Mode A](assets/soal_1/ls_amba_files.png)
#### 4. Membaca Isi File Melalui Filesystem Virtual
![Output Mode A](assets/soal_1/catmnt.png)
#### 5. Memverifikasi Semua File Mount Sama Dengan File Asli
![Output Mode A](assets/soal_1/tes.png)
#### 6. Membaca File Virtual `tujuan.txt`
![Output Mode A](assets/soal_1/cattujuan.png)

### Kendala Soal 1
1. Permission denied saat mount
2. File Zone.Identifier muncul

---
## Soal 2
Pada soal ini diminta untuk membuat sebuah sistem database sederhana yang menggabungkan teknologi FUSE, Docker, socket programming, dan encryption. Program akan membuat filesystem virtual terenkripsi menggunakan metode XOR sehingga file yang tersimpan pada storage asli berada dalam kondisi terenkripsi, sedangkan user tetap dapat membaca file dalam bentuk normal melalui mount point FUSE.
Selain membuat filesystem terenkripsi, program juga diminta untuk membuat komunikasi client-server menggunakan socket TCP. Server database berjalan di dalam Docker container pada port tertentu, sedangkan client digunakan untuk mengirim command ke server tersebut.
Docker digunakan untuk menjalankan server database secara terisolasi, sedangkan bind mount digunakan untuk menghubungkan filesystem virtual hasil FUSE dengan container Docker. Dengan demikian, container dapat mengakses file yang sebenarnya berasal dari filesystem virtual.
Konsep yang dipelajari pada soal ini meliputi:
- Filesystem virtual menggunakan FUSE
- XOR encryption
- Socket programming TCP
- Docker container
- Bind mount Docker
- Client-server architecture
- File encryption dan decryption

### Penjelasan Program
Program terdiri dari beberapa bagian:
#### 1. Filesystem FUSE
Digunakan untuk:
- membaca file
- menulis file
- mengenkripsi file
#### 2. XOR Encryption
Setiap file yang ditulis:
- akan di XOR
- disimpan dalam bentuk terenkripsi
Namun saat dibaca:
- file otomatis di decrypt
#### 3. Database Server
Server berjalan:
- pada port TCP 9000
- di dalam Docker container
#### 4. Client
Client digunakan untuk:
- connect ke server
- mengirim command database
#### 5. Docker
Docker digunakan untuk:
- menjalankan database server
- bind mount filesystem FUSE
### Penjelasan Kode
#### XOR KEY
```
#define XOR_KEY 0x76
```
Digunakan sebagai key enkripsi.
#### xor_crypt()
```
void xor_crypt(char *data, size_t size)
```
Fungsi ini melakukan:
```
data[i] ^= XOR_KEY;
```
Karena XOR bersifat reversible:
- encrypt XOR
- decrypt XOR lagi

#### fullpath()
```
snprintf(fpath, PATH_MAX, "%s%s.enc", source_dir, path);
```
Mengubah:
```
test.txt
```
menjadi:
```
test.txt.enc
```
#### moo_read()
Saat file dibaca:
- file dibuka
- isi file di decrypt
- plaintext dikirim ke user
#### moo_write()
Saat file ditulis:
- plaintext di encrypt
- ciphertext disimpan
#### TCP Socket
```
socket(AF_INET, SOCK_STREAM, 0)
```
Digunakan untuk komunikasi:
- client
- server

#### Docker Bind Mount
```
-v $(pwd)/fuse_mount:/app/db
```
Artinya:
- folder host dihubungkan ke container
### Cara Menjalankan
#### Compile FUSE
```
gcc fuse.c -o fuse `pkg-config fuse --cflags --libs`
```
#### Jalankan FUSE
```
mkdir -p encrypted_storage
mkdir -p fuse_mount

./fuse fuse_mount
```
#### Compile server
```
gcc server.c -o server
```
#### Compile client
```
gcc client.c -o client
```
#### Build docker
```
docker build -t soal2-server .
```
#### Jalankan docker
```
docker run -d \
--name soal2_container \
-p 9000:9000 \
-v $(pwd)/fuse_mount:/app/db \
soal2-server
```
#### Jalankan client
```
./client
```
### Kendala Soal 2
1. fuse_mount tidak bisa diakses
2. File tidak terenkripsi
---

## Soal 3
Pada soal ini diminta untuk membuat sebuah Samba Server menggunakan Docker yang memiliki beberapa shared folder dengan permission berbeda-beda untuk setiap user. Program harus mendukung autentikasi user, pengaturan akses read-only maupun write, serta pengelolaan shared folder menggunakan permission Linux dan konfigurasi Samba.
Container Docker digunakan untuk menjalankan service Samba sehingga seluruh konfigurasi server berjalan secara terisolasi. Setiap folder seperti `ebooks`, `papers`, `sourcecode`, dan `docs` memiliki aturan akses yang berbeda sesuai requirement soal. Beberapa user hanya boleh membaca file, sedangkan user tertentu diperbolehkan melakukan upload atau modifikasi file.
Selain itu, sistem juga menggunakan bind mount agar folder pada host dapat langsung digunakan di dalam container Samba. Dengan begitu, perubahan file dari host maupun container dapat langsung sinkron.
Melalui soal ini dipelajari beberapa konsep penting, yaitu:
- Samba server
- SMB protocol
- Docker Compose
- Permission Linux
- User dan Group Linux
- Shared folder management
- Bind mount Docker
- Multi-user file sharing

### Penjelasan Program
Program membuat Samba Server menggunakan Docker.
Container akan:
- membuat user
- membuat group
- menjalankan service samba
- mengatur permission tiap folder
### Penjelasan Kode
#### smb.conf
Digunakan untuk konfigurasi samba.
#### Share Folder
```
[docs]
path = /libraryit/docs
```
Membuat folder share bernama docs.
#### valid users
```
valid users = contributor
```
Menentukan siapa yang boleh mengakses.
#### read only
```
read only = yes
```
Menentukan apakah folder hanya bisa dibaca.
#### write list
```
write list = librarian
```
Menentukan siapa yang boleh menulis.
#### Docker Compose
```
ports:
  - "1445:445"
```
Digunakan untuk expose port samba.
#### Bind Mount
```
volumes:
  - ./data/docs:/libraryit/docs
```
Menghubungkan folder host dengan container.
### Cara Menjalankan
#### Build dan Run
```
docker-compose up -d --build
```
#### Login smbclient
```
smbclient //localhost/docs -p 1445 -U contributor%contrib456
```
#### Upload file
```
put test.txt
```
#### List Share
```
smbclient -L //localhost -p 1445 -U member%member123
```
### Kendala Soal 3
1. docker compose tidak ditemukan
2. Permission denied saat upload
