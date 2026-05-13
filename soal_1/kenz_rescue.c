#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *source_dir = "amba_files";

static int kenz_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fullpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    sprintf(fullpath, "%s%s", source_dir, path);

    res = lstat(fullpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

static int kenz_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    dp = opendir(source_dir);

    if (dp == NULL)
        return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {

        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0)
            continue;

        filler(buf, de->d_name, NULL, 0);
    }

    closedir(dp);

    return 0;
}

static int kenz_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char fullpath[1024];

    sprintf(fullpath, "%s%s", source_dir, path);

    res = open(fullpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

static int kenz_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    int fd;
    int res;
    char fullpath[1024];

    (void) fi;

    sprintf(fullpath, "%s%s", source_dir, path);

    fd = open(fullpath, O_RDONLY);

    if (fd == -1)
        return -errno;
    res = pread(fd, buf, size, offset);

    if (res == -1)
        res = -errno;
    close(fd);
    return res;
}
static struct fuse_operations kenz_oper = {
    .getattr = kenz_getattr,
    .readdir = kenz_readdir,
    .open = kenz_open,
    .read = kenz_read,
};
int main(int argc, char *argv[])
{
    umask(0);
    return fuse_main(argc, argv, &kenz_oper, NULL);
}
