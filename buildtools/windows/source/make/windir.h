#ifndef WINDIR_H
#define WINDIR_H

#include <wtypes.h>
#include <winbase.h>

struct dirent {
	char *d_name;
};
typedef struct {
	HANDLE dp;
	WIN32_FIND_DATA fdata;
	struct dirent de;
} dir_s;
typedef dir_s DIR;

DIR *opendir(char *);
struct dirent *readdir(DIR *);
void closedir(DIR *);

#endif
