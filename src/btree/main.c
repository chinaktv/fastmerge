#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>

#include "ui.h"

int main(int argc, char **argv)
{
	DIR *dirp;
	struct dirent *direp = NULL;
	char *outfile = NULL;

	ui *userinfo = &btree_ui;

	if (argc < 3) {
		printf("%s <user_info.csv> <path> [out.csv]\n", argv[0]);
		return -1;
	}
	if (argc == 4)
		outfile = argv[3];

	ui_init(userinfo);

	dirp = opendir(argv[2]);

	if (dirp) {
		char filename[256];
		int x = 0;
		direp = readdir(dirp);
		for (; direp != NULL; direp = readdir(dirp)) {
			if (!strcmp(direp->d_name, ".") || !strcmp(direp->d_name, ".."))
				continue;
		
			sprintf(filename, "%s/%s", argv[2], direp->d_name);
			ui_addfile(userinfo, filename);
			x++;

			if (x % 100 == 0)
				fprintf(stderr, "add %d\n", x);
		}

		closedir(dirp);
	}
	ui_out(userinfo, outfile);
	ui_free(userinfo);

	return  0;
}
