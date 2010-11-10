#include "btree.h"
#include "store.h"

#include  <stdio.h>
#include  <stdlib.h>
#include  <assert.h>
#include  <values.h>


/*= Start of Code =========================================================*/
#ifndef  RAND_MAX
#define  RAND_MAX    ((1 << (sizeof( int ) * 8 - 1)) - 1)
#endif  

static double doubleRand( void ) 
{
	return  (double)rand()/(double)RAND_MAX;
}

int compare_offt(const off_t * a, const off_t * b) 
{
	if(*a == *b) { return EQ; }
	if(*a <  *b) { return LT; }
	return GT;
}

const void* print_offt(const off_t * a, void * n) {
	printf("%ld\n", (long)*(off_t*)a);
	return a;
}

#define NUM_OBJECTS 5000

int main(int argc, char **argv)
{

	int  ind;
	off_t inserted[NUM_OBJECTS] = {0, };
	off_t t;
	struct store *rand = store_open_memory(sizeof(off_t), NUM_OBJECTS);
	struct btree *tree = btree_new_memory(rand, (int(*)(const void *, const void *))compare_offt);

	for  ( ind = 0; ind < NUM_OBJECTS; ind++ ) {
		off_t new = store_new(rand);
		off_t *new_data = store_read(rand, new);
		inserted[ind] = 2 * (off_t) (100000.0 * doubleRand());
		printf(".");
		fflush(stdout);

		*new_data = inserted[ind];  
		store_write(rand, new);
		store_release(rand, new);

		btree_insert( tree, new );

	}

	btree_close(tree);
	printf("Closing tree after print...\n");
	fflush(stdout);

	return  0;
}
