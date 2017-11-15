#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

static const char* basedir = "/home/avtors/Developments/OSBUN";

static char* check_ext(char* filename)
{
    char *dot = strrchr(filename, '.');
    if(dot == filename || dot == NULL) return "";
    return dot+1;
}

static char* get_relative_dir(const char* filename)
{
    char *slash = strrchr(filename, '/');
    *(slash+1) = '\0';
    return filename;
}

static char* get_filename(char* filename)
{
    char *dot = strrchr(filename, '/');

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
    printf("Attribute for %s\n", newPath);

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

    printf("Opening folder : %s\n", newPath);
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
        printf("Listing file : %s/%s\n", newPath, de->d_name);
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

    char ext[10];
    strcpy(ext, check_ext(path));

    // This will rename the file if the ext is pdf, doc, or txt
    if(!strcmp(ext, "pdf") || !strcmp(ext, "doc") || !strcmp(ext, "txt"))
    {
        char newName[256];
        char oldName[256];
        char fileName[256];
        char folder[256];

        sprintf(newName, "%s%s.ditandai", basedir, path);
        sprintf(oldName, "%s%s", basedir, path);
        strcpy(fileName, oldName);
        strcpy(fileName, get_filename(fileName));

        printf("File name : %s to %s\n", oldName, newName);
        system("zenity --error --text='Terjadi kesalahan! File berisi konten berbahaya.'"); // Show dialog box
        rename(oldName, newName);

        sprintf(folder, "%s%s", basedir, get_relative_dir(path));

        char folder_rahasia[256];
        sprintf(folder_rahasia, "%srahasia", folder);

        strcat(folder, "/rahasia/");
        strcat(folder, fileName);
        strcat(folder, ".ditandai");

        // Create folder rahasia
        DIR *fol_stat = opendir(folder_rahasia);
        if(fol_stat == NULL) // Knock knock?
            mkdir(folder_rahasia, 0755);

        // Move all .ditandai to rahasia/
        rename(newName, folder);


        // .ditandai file should not be able rwe
        chmod(folder, 0000);

        return -1;
    }
    if(!strcmp(path, "/")) sprintf(newPath, "%s", basedir);
    else sprintf(newPath, "%s%s", basedir, path);

    (void) fi;
    fd = open(newPath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    printf("Reading file : %s\n", newPath);

    close(fd);
    return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("\n\nTEST WRITE %s\n\n", path);
	int fd;
	int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	(void) fi;

	fd = open(newPath, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = write(fd, buf, size);

	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
    int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	res = utimensat(0, newPath, ts, AT_SYMLINK_NOFOLLOW);

	if(res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	res = lchown(newPath, uid, gid);

	if(res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	res = chmod(newPath, mode);

	if(res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	res = open(newPath, fi->flags);

	if(res == -1)
		return -errno;

	close(res);

	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

	if(S_ISREG(mode)) {
		res = open(newPath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0) res = close(res);
	}
	else if (S_ISFIFO(mode)) res = mkfifo(newPath, mode);
	else res = mknod(newPath, mode, rdev);

	if(res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
    int res;
    char newFrom[512];
    char newTo[512];
    char simpanan_dir[1024];
    char filename[128];
    strcpy(filename, from); // Copy orig filename, else will fail

    // Get relative dir, where folder simpanan should be
    // and create them folder ayy
    sprintf(simpanan_dir, "%s%s/simpanan", basedir, get_relative_dir(filename));
    printf("SIMPANAN IS %s\n", simpanan_dir);
    DIR *sim = opendir(simpanan_dir);
    if(sim == NULL)
        mkdir(simpanan_dir, 0755);

    sprintf(newFrom, "%s%s", basedir, from);
    sprintf(newTo, "%s/simpanan%s", basedir, to);

    // Debug purpose
    printf("RENAMING %s TO %s\n", newFrom, newTo);

    res = rename(newFrom, newTo);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_unlink(const char *path)
{
    int res;
    char newPath[512];
    sprintf(newPath, "%s%s", basedir, path);

    res = unlink(newPath);
    if (res == -1)
        return -errno;

    return 0;
}


static struct fuse_operations xmp_oper = {
    .getattr	= xmp_getattr,
    .readdir	= xmp_readdir,
    .read		= xmp_read,
    .write      = xmp_write,
    .utimens    = xmp_utimens,
    .chown      = xmp_chown,
    .chmod      = xmp_chmod,
    .open       = xmp_open,
    .mknod      = xmp_mknod,
    .rename     = xmp_rename,
    .unlink     = xmp_unlink,
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
