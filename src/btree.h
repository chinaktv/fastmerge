#ifndef  __BTREE__H
#define  __BTREE__H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "list.h"
#include "memwatch.h"

#define ALLOC_NUM 1024
struct btree_node {
	struct btree_node *left, *right, *parent;
	off_t data;
	char key[31];
	char balance;
};

#if 1
struct btree_node_head {
	struct list_head head;

	int used_id;
	struct btree_node node[ALLOC_NUM];
};
#endif

struct btree {
	struct store * store;

	struct list_head node_head;
	struct btree_node_head *node_pool;

	off_t entries_num;
	struct btree_node *root;
	int (*compare)(const void*, const void*);
	int (*insert_eq)(void*, void*);

	void (*close)    (struct btree * tree);
	void (*insert)   (struct btree * tree, void *data, const char *key, int *add, int *update);
	void (*show)     (struct btree * tree, void (*print_func)(void *, void*), void *userdata);
	int  (*find)     (struct btree * tree, const char *key);
	int  (*isbalance)(struct btree * tree);
};

struct btree *sbtree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*));
struct btree *avlbtree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*));

#define btree_close(tree)                          tree->close((tree))
#define btree_insert(tree, data, key, add, update) tree->insert((tree), (data), (key), (add), (update))
#define btree_find(tree, key)                      tree->find((tree), (key))
#define btree_isbalance(tree)                      tree->isbalance((tree))
#define btree_print(tree, print_func, userdata)    tree->show((tree), (void (*)(void *, void*))(print_func), (userdata))

#endif 

