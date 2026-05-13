#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>
#define XOR_KEY 0x76
static char source_dir[PATH_MAX];

void xor_crypt(char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] ^= XOR_KEY;
    }
}

void fullpath(char fpath[PATH_MAX], const char *path) {
    snprintf(fpath, PATH_MAX, "%s%s.enc", source_dir, path);
}

static int moo_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[PATH_MAX];
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    fullpath(fpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1) {
     	   return -errno;
    }
    return 0;
}

static int moo_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    dp = opendir(source_dir);
    if (dp == NULL) {
        return -errno;
    }
    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".enc")) {
            char name[256];

            strcpy(name, de->d_name);
            name[strlen(name) - 4] = '\0';
            filler(buf, name, NULL, 0);
        }
    }

    closedir(dp);

    return 0;
}

static int moo_open(const char *path, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    fullpath(fpath, path);
    int fd = open(fpath, fi->flags);
    if (fd == -1) {
     	   return -errno;
    }
    close(fd);

    return 0;
}

static int moo_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    (void) fi;
    fullpath(fpath, path);
    int fd = open(fpath, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }
    int res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else {
        xor_crypt(buf, res);
    }
    close(fd);
    return res;
}

static int moo_write(const char *path, const char *buf,
                     size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    (void) fi;
    fullpath(fpath, path);
    int fd = open(fpath, O_WRONLY);
    if (fd == -1) {
        return -errno;
    }

    char *encrypted = malloc(size);
    if (encrypted == NULL) {
        close(fd);
        return -ENOMEM;
    }

    memcpy(encrypted, buf, size);
    xor_crypt(encrypted, size);

    int res = pwrite(fd, encrypted, size, offset);

    free(encrypted);

    if (res == -1) {
        res = -errno;
    }

    close(fd);

    return res;
}

static int moo_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
    char fpath[PATH_MAX];

    (void) fi;

    fullpath(fpath, path);

    int fd = creat(fpath, mode);
    if (fd == -1) {
        return -errno;
    }

    close(fd);

    return 0;
}

static int moo_truncate(const char *path, off_t size) {
    char fpath[PATH_MAX];

    fullpath(fpath, path);

    int res = truncate(fpath, size);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

static int moo_unlink(const char *path) {
    char fpath[PATH_MAX];

    fullpath(fpath, path);

    int res = unlink(fpath);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

static struct fuse_operations moo_oper = {
    .getattr = moo_getattr,
    .readdir = moo_readdir,
    .open = moo_open,
    .read = moo_read,
    .write = moo_write,
    .create = moo_create,
    .truncate = moo_truncate,
    .unlink = moo_unlink,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_dir>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);
    char *fuse_argv[argc];
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; i++) {
        fuse_argv[i - 1] = argv[i];
    }
    int fuse_argc = argc - 1;
    umask(0);
    return fuse_main(fuse_argc, fuse_argv, &moo_oper, NULL);
}
