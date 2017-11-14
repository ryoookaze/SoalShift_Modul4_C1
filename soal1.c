#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

static const char* basedir = "/home/avtors/Developments/OSBUN";

static char* check_ext(char* filename)
{
    char *dot = strrchr(filename, '.');
    if(dot == filename || dot == NULL) return "";
    return dot+1;
}

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;

    char newPath[1024];
    sprintf(newPath, "%s%s", basedir, path);

	res = lstat(newPath, stbuf);
	if (res == -1)
	    return -errno;

	return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

    char newPath[1024];
    if(!strcmp(path, "/")) sprintf(newPath, "%s", basedir);
    else sprintf(newPath, "%s%s", basedir, path);

	dp = opendir(newPath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
    char newPath[1024];

    if(!strcmp(path, "/")) sprintf(newPath, "%s", basedir);
    else sprintf(newPath, "%s%s", basedir, path);

	(void) fi;
	fd = open(newPath, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.readdir	= xmp_readdir,
	.read		= xmp_read,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
