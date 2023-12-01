#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>


/*
 * API synopsis:
 *
 * int  symlink(const char *filename, const char *symlinkname);
 * int  chown(const char *path, uid_t owner, gid_t group);
 * int  (l)stat(const char *restrict pathname, struct stat *restrict statbuf);
 * ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsiz);
 * int  unlink(const char *path);
 *
 */


// static inline void nop(void) { return; }


#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


int symlink_resolved()
{
	system("ls -la /dev/{cu*,hydra*}");
	printf("\n===> symlink_resolved?\n");
	
	const char *sname = "/dev/hydrabus";
	struct stat st_link, st_file; // = malloc(sizeof(struct stat));
	
	int ret = lstat(sname, &st_link);
	if (ret) {
		printf("\tsymlink can't be open!\n");
		printf("\tERR: %s (%d)\n", strerror(errno), errno);
		return 0;
	} else {
		printf("\tsymlink: open %s - OK\n", sname);
	}
	
	bool is_lnk = S_ISLNK(st_link.st_mode);
	printf("\tlnk: %s\n", is_lnk ? "YES" : "NO");
	if (!is_lnk) {
		printf("\tsymlink: file %s is not symlink!\n", sname);
		return 0;
	}
	
	char fname[PATH_MAX] = { '\0' };
	ssize_t size = readlink(sname, fname, PATH_MAX);
	if (-1 == size) {
		printf("\tcan't readlink symlink!\n");
		printf("\tERR: %s (%d)\n", strerror(errno), errno);
		return 0;
	}
	printf("\tsymlink: %s -> file: %s - OK\n", sname, fname);
	
	ret = stat(sname, &st_file);
	if (ret) {
		printf("\tsymlink target file %s doesn't exist!\n", sname);
		printf("\tERR: %s (%d)\n", strerror(errno), errno);
		return 0;
	}
	
	return 0;
}


int main(int argc, const char *argv[])
{
	if (argc) { return symlink_resolved(); }
	
	printf("\n===> get(e)uid/getlogin\n");
	
	printf("\tuid/euid: %d / %d\n", getuid(), geteuid());
	printf("\tlogin: %s\n", getlogin());
	
	
	printf("\n===> getpwnam\n");
	
	int ret = errno = 0;
	struct passwd *pwd = getpwnam(getlogin());
	if (!pwd) {
		printf("Error: %s (%d)\n", strerror(errno), errno);
		return 0;
	}
	
	printf("\tpw_name: %s\n", pwd->pw_name);
	printf("\tpw_uid: %d\n", pwd->pw_uid);
	
	uid_t user_id = pwd->pw_uid;
	
	
	printf("\n===> symlink\n");
	
	const char *fname = "/dev/cu.usbmodemfd121";
	const char *sname = "/dev/hydrabus";
	ret = symlink(fname, sname);
	if (ret) {
		if (EEXIST == errno) {
			printf("\tWARN: already exist\n");
		} else {
			printf("\tERR: %s (%d)\n", strerror(errno), errno);
			return 0;
		}
	}
	
	ret = chown(sname, user_id, -1);
	if (ret) { printf("\tERR: %s (%d)\n", strerror(errno), errno); }
	
	ret = chown(fname, user_id, -1);
	if (ret) { printf("\tERR: %s (%d)\n", strerror(errno), errno); }
	
	system("ls -la /dev/{cu*,hydra*}");
	
	
	printf("\n===> stat/unlink\n");
	
	struct stat st; // = malloc(sizeof(struct stat));
	ret = lstat(sname, &st);
	if (ret) {
		printf("\tERR: %s (%d)\n", strerror(errno), errno);
		return 0;
	}
	
	int  is_lnk_raw = S_ISLNK(st.st_mode);
	bool is_lnk     = S_ISLNK(st.st_mode);
	printf("\tlnk: %s / %d\n", is_lnk ? "YES" : "NO", is_lnk_raw);
	printf("\tuid: %d\n", st.st_uid);
	
	if (st.st_uid != user_id) {
		ret = chown(sname, user_id, -1);
		if (ret) { printf("\tERR: %s (%d)\n", strerror(errno), errno); }
		printf("\tpermissions tried to be re-applied!\n");
	}
	
	if (is_lnk) {
		ret = unlink(sname);
		if (ret) { printf("\tERR: %s (%d)\n", strerror(errno), errno); }
	}
	
	
	system("ls -la /dev/{cu*,hydra*}");
	
	
	return 0;
}


