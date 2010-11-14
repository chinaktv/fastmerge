#ifndef  __BTREE__H
#define  __BTREE__H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memwatch.h"

struct btree_node {
	struct btree_node *left, *right, *parent;
	off_t data;
	char key[31];
	char balance;
};

struct btree {
	struct store * store;
	struct btree_node **node_array;
	int alloc_id;
	int array_count;

	off_t entries_num;
	struct btree_node *root;
	int (*compare)(const void*, const void*);
	int (*insert_eq)(void*, void*);
};

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*));
void btree_close   (struct btree * tree);
void btree_init    (struct btree * tree);
void btree_insert  (struct btree * tree, void *data, const char *key, int *add, int *update);
void btree_print   (struct btree * tree, void (*print)(void *, void*), void *userdata);
int  btree_find    (struct btree * tree, const char *key);
int btree_isbalance(struct btree * tree);

#endif 

