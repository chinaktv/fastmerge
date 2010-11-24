#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>

#include "btree.h"
#include "store.h"
#include "ui.h"
#include "info.h"

struct btree_info {
	struct btree *tree;
	struct store *userinfo_store;
};

static struct btree_info *avlbtree_ui_create(void)
{
	struct btree_info *bi;
	
	bi = (struct btree_info*)malloc(sizeof(struct btree_info));
	assert(bi);

	bi->userinfo_store = store_open_memory(sizeof(struct user_info), 102400);
//	bi->userinfo_store = store_open_disk("/tmp/temp", sizeof(struct user_info), 102400);

	bi->tree = avlbtree_new_memory(bi->userinfo_store);

	return bi;
}

static int userinfo_insert(struct btree *tree, char *info_str, size_t seek, int *add, int *update)
{
	struct user_info new_data;
	char key[20] = {0, }, *p;
	int len;

	if (tree == NULL || info_str == NULL)
		return -1;

	p = strchr(info_str, ',');

	len  =p - info_str;
	if (len > 18)
		len = 18;

	memcpy(key, info_str, p - info_str);

	memset(&new_data, 0, sizeof(struct user_info));
	userinfo_parser(&new_data, info_str, seek);

	btree_insert(tree, &new_data, key, &new_data.update, add, update);

	return 0;
}

static int avlbtree_ui_addfile(struct btree_info *bi, const char *filename, int *add, int *update)
{
	if (filename && bi) {
		FILE *fp;
		char str[512];
		if ((fp  = fopen(filename, "r")) != NULL) {
			size_t seek;

			while (!feof(fp)) {
				seek = ftell(fp);
				if (fgets(str, 512, fp))
					userinfo_insert(bi->tree, str, seek, add, update);
			}
			fclose(fp);

			return 0;
		}
	}

	return -1;
}

static void avlbtree_ui_out(struct btree_info *ui, const char *filename)
{
	FILE *out = stdout;

	if (filename) {
		printf("output %s\n", filename);
		out = fopen(filename, "w+");
		if (out == NULL)
			out = stdout;
	}
#if 1
	btree_print(ui->tree, (void (*)(void*, void*))userinfo_print, out);
#else
	{
		int i;

		for (i=0; i < ui->tree->entries_num; i++) {
			struct user_info *info = store_read(ui->userinfo_store, i);
			userinfo_print(out, info);

			store_release(ui->userinfo_store, i, info);
		}
	}
#endif
	if (out != stdout)
		fclose(out);
}

static void avlbtree_ui_free(struct btree_info *ui)
{
	store_close(ui->userinfo_store);
	btree_close(ui->tree);
	free(ui);
}

static int avlbtree_ui_find(struct btree_info *ui, const char *key)
{
	return btree_find(ui->tree, key);
}

static void avlbtree_ui_set(struct btree_info *ui, int k, int v)
{
	if (ui->tree->set)
		ui->tree->set(ui->tree, k, v);
}

ui avlbtree_ui = {
	.init    = (void *(*)(void))                            avlbtree_ui_create,
	.addfile = (int (*)(void*, const char *, int *, int *)) avlbtree_ui_addfile,
	.out     = (void (*)(void*, const char *))              avlbtree_ui_out,
	.free    = (void (*)(void *))                           avlbtree_ui_free,
	.find    = (int (*)(void *, const char*))               avlbtree_ui_find,
	.set     = (void (*)(void *, int, int))                 avlbtree_ui_set,
};

