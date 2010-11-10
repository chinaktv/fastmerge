#include "btree.h"
#include "store.h"

#include  <stdio.h>
#include  <stdlib.h>
#include  <assert.h>
#include  <values.h>


struct user_info {
	off_t id;
//	char id[19];
	char name[20];
	char email[20];
	int sex;
	int mobile[12];
	time_t update;
};

/*= Start of Code =========================================================*/
#ifndef  RAND_MAX
#define  RAND_MAX    ((1 << (sizeof( int ) * 8 - 1)) - 1)
#endif  

static double doubleRand( void ) 
{
	return  (double)rand()/(double)RAND_MAX;
}

int compare_user_info(const struct user_info *a, const struct user_info *b) 
{
	if(a->id == b->id) { return EQ; }
	if(a->id <  b->id) { return LT; }
	return GT;
}

#define NUM_OBJECTS 500000

int main(int argc, char **argv)
{
	int  ind;
	off_t inserted[NUM_OBJECTS] = {0, };
	off_t t;
	struct store *rand = store_open_memory(sizeof(struct user_info), NUM_OBJECTS);
	struct btree *tree = btree_new_memory(rand, (int(*)(const void *, const void *))compare_user_info);

	for  ( ind = 0; ind < NUM_OBJECTS; ind++ ) {
		off_t new = store_new(rand);
		struct user_info *new_data = store_read(rand, new);
		inserted[ind] = 2 * (off_t) (100000.0 * doubleRand());
		printf(".");
		fflush(stdout);

		new_data->id = inserted[ind];  
		store_write(rand, new);
		store_release(rand, new);

		btree_insert( tree, new );

	}
	for (ind = 0; ind < NUM_OBJECTS; ind++) {
		off_t num = tree->entries_num;
		assert(tree->entries_num == NUM_OBJECTS - ind);
		assert(btree_find(tree, &(inserted[ind])));

		assert(btree_delete(tree, &(inserted[ind])));
		t = inserted[ind] + 1;
		assert(!btree_delete(tree, &t));

		assert(tree->entries_num + 1 == num);

	}

	btree_close(tree);
	printf("Closing tree after print...\n");
	fflush(stdout);

	return  0;
}
