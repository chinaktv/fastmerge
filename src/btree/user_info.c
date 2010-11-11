#include "btree.h"
#include "store.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>


#define STR_KEY 1

typedef union {
	struct {
		int  zipcode;
		int  y, m, d;
		int  order;
		char check;
	};

	int key[5];
}user_id;

struct user_info {
#if STR_KEY
	char card[20];
#else
	user_id userid;
#endif

	char name[20];
	char email[20];
	char sex;
	char mobile[12];
	struct tm update;
};

int userinfo_insert(struct btree *tree, char *info_str);


void userid_print(user_id id)
{
	printf("%d%d%02d%02d%03d%c", id.zipcode, id.y, id.m, id.d, id.order, id.check);
}


#define KEY (key[x++] - '0')
inline void str2time(struct tm *t, const char *key)
{
	int x = 0;

	t->tm_year  = KEY * 1000;
	t->tm_year += KEY * 100;
	t->tm_year += KEY * 10;
	t->tm_year += KEY * 1;
	t->tm_year -= 1900;

	x++;
	t->tm_mon  = KEY * 10;
	t->tm_mon += KEY * 1;
	
	x++;
	t->tm_mday  = KEY * 10;
	t->tm_mday += KEY * 1;
	
	x++;
	t->tm_hour  = KEY * 10;
	t->tm_hour += KEY * 1;
	
	x++;
	t->tm_min   = KEY * 10;
	t->tm_min  += KEY * 1;
	
	x++;
	t->tm_sec   = KEY * 10;
	t->tm_sec  += KEY * 1;
}

inline int str2userid(user_id *userid, const char *key)
{
	int x = 0, len;

	// 422302 1978 0227 033 8
	len = strlen(key);

	if (len < 15  || len > 18)
		return -1;

	userid->zipcode  = KEY * 100000;
	userid->zipcode += KEY * 10000;
	userid->zipcode += KEY * 1000;
	userid->zipcode += KEY * 100;
	userid->zipcode += KEY * 10;
	userid->zipcode += KEY * 1;

	if (len == 15) {
		userid->y = 1900;
	}
	else {
		userid->y = KEY * 1000;
		userid->y += KEY * 100;
	}

	userid->y += KEY * 10;
	userid->y += KEY;

	userid->m  = KEY * 10;
	userid->m += KEY;

	userid->d  = KEY * 10;
	userid->d += KEY * 1;

	userid->order  = KEY * 100;
	userid->order += KEY * 10;
	userid->order += KEY * 1;

	userid->check = key[17];

	return 0;
}

static int compare_user_info(const struct user_info *a, const struct user_info *b) 
{
#if STR_KEY
	int x = strncmp(a->card, b->card, 17);
	if (x < 0)
		return LT;
	else if (x > 0)
		return GT;

	return EQ;
		
#else
	int i = 0, x;

	for (; i < 5; i++) {
		x = a->userid.key[i] - b->userid.key[i];
		if (x != 0)
			return (x < 0) ? LT: GT;
	}

	return EQ;
#endif
}

static int insert_eq(struct user_info *old, struct user_info *_new)
{
	if (old->update.tm_year < _new->update.tm_year)
		goto update;

	if (old->update.tm_mon < _new->update.tm_mon)
		goto update;

	if (old->update.tm_mday < _new->update.tm_mday)
		goto update;

	if (old->update.tm_hour < _new->update.tm_hour)
		goto update;

	if (old->update.tm_min < _new->update.tm_min)
		goto update;

	if (old->update.tm_sec < _new->update.tm_sec)
		goto update;

	return -1;

update:
	printf("%s -> %s\n", _new->name, old->name);
	memcpy(old, _new, sizeof(struct user_info));

	return 0;
}

struct btree *userinfo_create(const char *filename)
{
	struct store *rand;
	struct btree *tree;

	
	rand = store_open_memory(sizeof(struct user_info), 1000);
	tree = btree_new_memory(rand, (int(*)(const void *, const void *))compare_user_info, (int (*)(void*, void*))insert_eq);

	if (filename) {
		FILE *fp;
		char str[512];
		if ((fp  = fopen(filename, "r")) != NULL) {
			while (!feof(fp)) {
				if (fgets(str, 512, fp))
					userinfo_insert(tree, str);
			}
			fclose(fp);
		}
	}

	return tree;
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

static int userinfo_parser(struct user_info *info, char *info_str)
{
	// 815300198704187597,move,female,move@qq.com,134120369062,2010-11-02 23:11:47
	char *p, *key =  info_str;
	int i = 0;

	while (key) {
		if ((p = strchr(key, ',')))
			*p = 0;
		switch (i) {
			case 0:
#if STR_KEY
				strncpy(info->card, key, sizeof(info->card));
#else
				str2userid(&info->userid, key);
#endif
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
#if 0
				sscanf(key, "%d-%d-%d %d:%d:%d", &info->update.tm_year, \
						&info->update.tm_mon, \
						&info->update.tm_mday,\
						&info->update.tm_hour,\
						&info->update.tm_min,\
						&info->update.tm_sec);
				info->update.tm_year -= 1900;
#endif

				str2time(&info->update, key);
				break;
		}

		if (p)
			key = p + 1;
		else
			break;
		i++;
	}

#if 0
	printf("%d-%d-%d-%d-%d-%c, %s,%s,%s, %s", 
			info->userid.zipcode, info->userid.y, info->userid.m, info->userid.d, info->userid.order, info->userid.check,
			info->name, info->email, info->mobile, asctime(&info->update));
#endif
	return 0;
}

int userinfo_insert(struct btree *tree, char *info_str)
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

int main(int argc, char **argv)
{
	struct btree *tree;


	printf("argc=%d\n", argc);
	if (argc ==  1) {
		printf("%s <csv file>\n", argv[0]);
		return -1;
	}

	tree = userinfo_create(argv[1]);
#if 0
	char *key;
	struct user_info find_info;
	printf("start search!\n");
	fp  =  fopen(argv[1], "r");
	while (!feof(fp)) {

		memset(str, 0, 256);

		if (fgets(str, 255, fp) == NULL)
			break;
		key = strtok(str, ",");
		if (key == NULL)
			break;

//		strcpy(find_info.card, "321200197611104705");
//		str2userid(&find_info.userid, "321200197611104705");
#if STR_KEY
		strcpy(find_info.card, key);
#else
		str2userid(&find_info.userid, key);
#endif
		if (btree_find(tree, &find_info) != 1)
			printf("not found %s\n", key);
	}
	fclose(fp);
#endif
#if 0
#define NUM_OBJECTS 500000
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
