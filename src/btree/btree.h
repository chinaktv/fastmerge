#ifndef  __BTREE__H
#define  __BTREE__H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef  TRUE
#define  TRUE    1
#define  FALSE   0
#endif  /* TRUE */

#define LT -1
#define EQ 0
#define GT 1

struct  btree_node {
	off_t left, right, father;
	off_t  data;
};


struct btree {
	struct store * oStore;
	struct store * nStore;
	off_t entries_num;
	off_t root;
	int (*compare)(const void*, const void*);
};

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*));
void btree_close (struct btree * p_btree );
void btree_init  (struct btree * p_btree );
void btree_insert(struct btree * p_btree, off_t data);
int  btree_delete(struct btree * p_btree, void* data);
int  btree_find  (const struct btree * tree, void* find_data);
void btree_query (const struct btree * tree, void* find_data, void*(*handler)(const void*,void*), void * handler_state);
void btree_print (const struct btree  * p_btree );

#endif 

