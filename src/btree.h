#ifndef  __BTREE__H
#define  __BTREE__H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "memwatch.h"

struct  btree_node {
	off_t left, right;
	off_t data;
	char key[19];
};

struct btree {
	struct store * oStore;
	struct store * nStore;
	off_t entries_num;
	off_t root;
	int (*compare)(const void*, const void*);
	int (*insert_eq)(void*, void*);
};

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*));
void btree_close (struct btree * tree );
void btree_init  (struct btree * tree );
void btree_insert(struct btree * tree, void *data, const char *key);
void btree_print (struct btree * tree, void (*print)(void *, void*), void *userdata);
int  btree_find  (struct btree * tree, const char *key);

#endif 

