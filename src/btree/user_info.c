#include "btree.h"
#include "store.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>

struct user_info {
	off_t id;
	char card[19];
	char name[20];
	char email[20];
	char sex;
	char mobile[12];
	time_t update;
};

static int compare_user_info(const struct user_info *a, const struct user_info *b) 
{
	return strcmp(a->card, b->card);
//	if(a->id == b->id) { return EQ; }
//	if(a->id <  b->id) { return LT; }
//	return GT;
}

struct btree *userinfo_create()
{
	struct store *rand = store_open_memory(sizeof(struct user_info), 1000);
	return  btree_new_memory(rand, (int(*)(const void *, const void *))compare_user_info);
}

struct user_info *userinfo_new(struct btree *tree)
{
	struct store *oStore;
	off_t new;
	struct user_info *new_data;

	if (tree == NULL)
		return NULL;

	oStore = tree->oStore;

	new = store_new(oStore);
	new_data = store_read(oStore, new);
	if  (new_data) {
		store_write(oStore, new);
		store_release(oStore, new);
	}

	return new_data;
}

static int userinfo_parser(struct user_info *info, const char *info_str)
{
	// 815300198704187597,move,female,move@qq.com,134120369062,2010-11-02 23:11:47
	char *tmp = strdup(info_str);
	char *key = strtok(tmp, ",");
	int i = 0;
	while (key) {
		switch (i) {
			case 0:
				strncpy(info->card, key, sizeof(info->card));
				break;
			case 1:
				strncpy(info->name, key, sizeof(info->name));
				break;
			case 2:
				info->sex = key[0];
				break;
			case 3:
				strncpy(info->email, key, sizeof(info->email));
				break;
			case 4:
				strncpy(info->mobile, key, sizeof(info->mobile));
				break;
		
			case 5:
				// TODO info->update =  
				break;
		}

		i++;
		key = strtok(NULL, ",");
	}

//	printf("%s,%s,%s,%s\n", info->card, info->name, info->email, info->mobile);
	return 0;
}

int userinfo_insert(struct btree *tree, const char *info_str)
{
	off_t new;
	struct store *oStore;
	struct user_info *new_data;

	if (tree == NULL || info_str == NULL)
		return -1;

	oStore = tree->oStore;
	if (oStore == NULL)
		return -1;

	new = store_new(oStore);
	new_data = store_read(oStore, new);

	if (new_data == NULL)
		return -1;

	userinfo_parser(new_data, info_str);
	store_write(oStore, new);
	store_release(oStore, new);

	btree_insert(tree, new);

	return 0;
}

int userinfo_find(struct btree *tree, const char *card)
{

	return 0;
}

#define NUM_OBJECTS 500000

int main(int argc, char **argv)
{
	off_t inserted[NUM_OBJECTS] = {0, };
	FILE *fp;
	char str[256], *key;
	struct btree *tree;
	struct user_info find_info;


	printf("argc=%d\n", argc);
	if (argc ==  1) {
		printf("%s <csv file>\n", argv[0]);
		return -1;
	}


	tree = userinfo_create();
	fp  =  fopen(argv[1], "r");
	while (!feof(fp)) {
		memset(str, 0, 256);
		fgets(str, 255, fp);
		userinfo_insert(tree, str);
	}
	fclose(fp);

	fp  =  fopen(argv[1], "r");
	while (!feof(fp)) {
		memset(str, 0, 256);

		fgets(str, 255, fp);
		key = strtok(str, ",");
		if (key == NULL)
			break;

		strncpy(find_info.card, key, sizeof(find_info.card));
		if (btree_find(tree, &find_info) != 1)
			printf("not found %s\n", find_info.card);
		else
			printf("found %s\n", find_info.card);
	}
	fclose(fp);

#if 0
	for (ind = 0; ind < NUM_OBJECTS; ind++) {
		off_t num = tree->entries_num;
		assert(tree->entries_num == NUM_OBJECTS - ind);
		assert(btree_find(tree, &(inserted[ind])));

		assert(btree_delete(tree, &(inserted[ind])));
		t = inserted[ind] + 1;
		assert(!btree_delete(tree, &t));

		assert(tree->entries_num + 1 == num);

	}
#endif
	btree_close(tree);

	return  0;
}
