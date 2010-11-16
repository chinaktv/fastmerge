#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>

#include "ui.h"
#include "info.h"
#include "btree.h"

int main(int argc, char **argv)
{
	DIR *dirp;
	struct dirent *direp = NULL;
	char *outfile = NULL;
	int add, update;

	ui *userinfo = NULL;
	
	if (argc < 4) {
		printf("%s <user_info.csv> <1/2> <path> [out.csv]\n", argv[0]);
		return -1;
	}

	if (argv[1][0] == '1')
		userinfo = &btree_ui;
	else if (argv[1][0] == '2')
		userinfo = &bthread_ui;

	if (argc == 5)
		outfile = argv[4];

	ui_init(userinfo);
	add = update = 0;
	ui_addfile(userinfo, argv[2], &add, &update);
#if 0
	{
		char *key, str[256];
		FILE *fp  =  fopen(argv[2], "r");
		while (!feof(fp)) {
			memset(str, 0, 256);

			if (fgets(str, 255, fp) == NULL)
				break;
			key = strtok(str, ",");
			if (key == NULL)
				break;

			if (ui_find(userinfo, key) == 0)
				printf("not found %s\n", key);
		}
		fclose(fp);
	}
#endif

#if 1
	dirp = opendir(argv[3]);

	if (dirp) {
		char filename[256];

		direp = readdir(dirp);
		for (; direp != NULL; direp = readdir(dirp)) {
			if (!strcmp(direp->d_name, ".") || !strcmp(direp->d_name, ".."))
				continue;
		
			sprintf(filename, "%s/%s", argv[3], direp->d_name);
			add = update = 0;
			ui_addfile(userinfo, filename, &add, &update);

			if (update != 0)
				printf("%s add %d, update %d\n", filename, add, update);
		}

		closedir(dirp);
	}
#endif
	ui_end(userinfo);
	ui_out(userinfo, outfile);
	ui_free(userinfo);

	return  0;
}

