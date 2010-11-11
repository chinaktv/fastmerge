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

static struct btree_info *btree_ui_create(void)
{
	struct btree_info *bi = (struct btree_info*)malloc(sizeof(struct btree_info));

	bi->userinfo_store = store_open_memory(sizeof(struct user_info), 1000);
	bi->tree = btree_new_memory(bi->userinfo_store, (int(*)(const void *, const void *))userinfo_compare, (int (*)(void*, void*))userinfo_update);

	return bi;
}

static int userinfo_insert(struct btree *tree, char *info_str)
{
	off_t new;
	struct store *oStore;
	struct user_info *new_data;

	if (tree == NULL || info_str == NULL)
		return -1;

	oStore = tree->oStore;

	new = store_new(oStore);
	new_data = store_read(oStore, new);

	if (new_data == NULL)
		return -1;

	memset(new_data, 0, sizeof(struct user_info));
	userinfo_parser(new_data, info_str);
	store_write(oStore, new);
	store_release(oStore, new);

	btree_insert(tree, new);

	return 0;
}

#if 0
int userinfo_addfile(struct btree *tree, const char *filename)
{
	if (filename) {
		FILE *fp;
		char str[512];
		if ((fp  = fopen(filename, "r")) != NULL) {
			while (!feof(fp)) {
				if (fgets(str, 512, fp))
					userinfo_insert(tree, str);
			}
			fclose(fp);

			return 0;
		}
	}

	return -1;
}
#endif

static int btree_ui_addfile(struct btree_info *bi, const char *filename)
{
	if (filename && bi) {
		FILE *fp;
		char str[512];
		if ((fp  = fopen(filename, "r")) != NULL) {
			while (!feof(fp)) {
				if (fgets(str, 512, fp))
					userinfo_insert(bi->tree, str);
			}
			fclose(fp);

			return 0;
		}
	}

	return -1;

}

static void btree_ui_out(struct btree_info *ui, const char *filename)
{
	FILE *out = stdout;

	if (filename) {
		printf("filename=%s\n");
		out = fopen(filename, "w+");
		if (out == NULL)
			out = stdout;
	}
	btree_print(ui->tree, (void (*)(void*, void*))userinfo_print, out);

	if (out != stdout)
		fclose(out);
}

static void btree_ui_free(struct btree_info *ui)
{
	store_close(ui->userinfo_store);
	btree_close(ui->tree);
	free(ui);
}

ui btree_ui = {
	.init    = (void *(*)(void))btree_ui_create,
	.addfile = (int (*)(void*, const char *))btree_ui_addfile,
	.out     = (void (*)(void*, const char *))btree_ui_out,
	.free    = (void (*)(void *))btree_ui_free
};

