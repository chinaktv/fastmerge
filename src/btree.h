#ifndef  __BTREE__H
#define  __BTREE__H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "info.h"
#include "list.h"
#include "memwatch.h"

#define KEY_LEN 17
#define ALLOC_NUM 1024

struct btree_node {
	struct btree_node *left, *right, *parent;
	size_t        data;
	unsigned int date, time;

	char         key[KEY_LEN + 1];
	char         balance;
};

#if 1
struct btree_node_head {
	struct list_head head;

	int used_id;
	struct btree_node node[ALLOC_NUM];
};
#endif

struct btree {
	struct store *store;

	struct list_head node_head;
	struct btree_node_head *node_pool;
	int avl;

	size_t entries_num;
	struct btree_node *root;
	int (*compare)(const void*, const void*);
	int (*insert_eq)(void*, void*);

	void (*close)    (struct btree * tree);
	void (*insert)   (struct btree * tree, void *data, const char *key, struct dt *time, int *add, int *update);
	void (*show)     (struct btree * tree, struct btree_node *node, void (*print_func)(void *, void*), void *userdata);
	int  (*find)     (struct btree * tree, const char *key);
	int  (*isbalance)(struct btree * tree);
	void (*set)      (struct btree * tree, int key, int value);
};

struct btree *sbtree_new_memory(struct store *store);
struct btree *avlbtree_new_memory(struct store *store);

#define btree_close(tree)                                tree->close(tree)
#define btree_insert(tree, data, key, time, add, update) tree->insert(tree, data, key, time, add, update)
#define btree_find(tree, key)                            tree->find(tree, key)
#define btree_isbalance(tree)                            tree->isbalance(tree)
#define btree_print(tree, print_func, userdata)          tree->show(tree, (tree)->root, (void (*)(void *, void*))(print_func), userdata)
#define btree_set(tree, k, v) \
	do {                                      \
		if ((tree)->set)                  \
			(tree)->set(tree, k, v);  \
	}while (0);


#endif 

