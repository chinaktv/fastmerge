#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <assert.h>

#include "btree.h"
#include "store.h"
#include "info.h"

#define REC_DEPTH 0

static void sbtree_print    (struct btree * tree, struct btree_node *node, void (*print)(void *, void*), void *userdata);
static void sbtree_insert   (struct btree * tree, void *data, const char *key, struct dt *time, int *add, int *update);
static void sbtree_close    (struct btree * tree);
static int  sbtree_find     (struct btree *tree, const char *key);
static int  sbtree_isbalance(struct btree *tree);

struct btree *sbtree_new_memory(struct store *store)
{
	struct btree * tree;

	tree = (struct btree *) malloc(sizeof(struct btree));
	assert(tree);

	memset(tree, 0, sizeof(struct btree));

	tree->store       = store;
	tree->root        = NULL;
	tree->entries_num = 0;

	INIT_LIST_HEAD(&tree->node_head);
	tree->node_pool = (struct btree_node_head *)calloc(1, sizeof(struct btree_node_head));

	tree->node_pool->used_id = 0;
	list_add(&tree->node_pool->head, &tree->node_head);

	tree->close     = sbtree_close;
	tree->insert    = sbtree_insert;
	tree->show      = sbtree_print;
	tree->find      = sbtree_find;
	tree->isbalance = sbtree_isbalance;

	return tree;
}

static struct btree_node *node_new(struct btree *tree, void *data, const char *key, struct dt *time)
{
	struct btree_node *p_node;

	if (tree->node_pool->used_id >= ALLOC_NUM) {
		tree->node_pool = (struct btree_node_head *) calloc(1, sizeof(struct btree_node_head));
		tree->node_pool->used_id = 0;
		list_add(&tree->node_pool->head, &tree->node_head);
	}

	p_node = tree->node_pool->node + tree->node_pool->used_id++;

	p_node->left = p_node->right = p_node->parent = NULL;
	p_node->data = store_new_write(tree->store, data);
	p_node->date = time->date;
	p_node->time = time->time;

	if (key)
		strncpy(p_node->key, key, KEY_LEN);
	else
		p_node->key[0] = '\0';

	return p_node;
}

static void sbtree_insert(struct btree * tree, void *data, const char *key, struct dt *time, int *add, int *update)
{
	struct btree_node *thiz = tree->root;

	if (thiz == NULL) {
		tree->root = node_new(tree, data, key, time);

		return;
	}

	while  ( thiz != NULL ) {
		int cmp = strncmp(key, thiz->key, KEY_LEN);

		if (cmp == 0) {
			if (thiz->date < time->date || (thiz->date == time->date && thiz->time < time->time)) {
				void * pptr_data_ptr = store_read(tree->store, thiz->data);
				memcpy(pptr_data_ptr, data, store_blockSize(tree->store));
				store_write(tree->store, thiz->data, pptr_data_ptr);
				store_release(tree->store, thiz->data, pptr_data_ptr);

				thiz->data = time->date;
				thiz->time = time->time;

				(*update)++;
			}
			return;
		}
		if (cmp < 0) {
			if (thiz->left != NULL)
				thiz = thiz->left;
			else {
				thiz->left = node_new(tree, data, key, time);
				(*add)++;

				break;
			}
		}
		else {
			if (thiz->right != NULL)
				thiz = thiz->right;
			else {
				thiz->right = node_new(tree, data, key, time);
				(*add)++;

				break;
			}
		}
	}
}

static int bintree_find(struct btree * tree, struct btree_node * node, const char *key, int *depth)
{
	int ret = 1;
	int cmp = strncmp(key, node->key, KEY_LEN);

	if (cmp < 0) {
		if(node->left == NULL) {
			ret = 0;
		} else {
			struct btree_node * left = node->left;
			*depth += 1;
			ret = bintree_find(tree, left, key, depth);
		}
	} 
	else if (cmp > 0){
		if(node->right == NULL) {
			ret = 0; 
		} else {
			struct btree_node * right = node->right;

			*depth += 1;
			ret = bintree_find(tree, right, key, depth);
		}
	}

	return ret; 
}

int sbtree_find(struct btree *tree, const char *key) 
{ 
	int ret, depth = 0;
#if REC_DEPTH
	static int max_depth = 0;
#endif
	struct btree_node * root = tree->root;
	ret = bintree_find(tree, root, key, &depth);

#if REC_DEPTH
	if (max_depth < depth) {
		max_depth = depth;
		printf("max depth = %d\n", max_depth);
	}
#endif
	return ret;
}

static void sbtree_close(struct btree * tree)
{
	struct list_head *pos, *n, *head;

	head = & tree->node_head;
	list_for_each_safe(pos, n, head) {
		struct btree_node_head *x = list_entry(pos, struct btree_node_head, head);
		list_del(pos);
		free(x);
	}

	free(tree);
}

static void sbtree_print(struct btree * tree, struct btree_node * node, void (*print)(void *, void*), void *userdata)
{
	if  ( node == NULL )
		return;

	if(node->left != NULL)
		sbtree_print(tree, node->left, print, userdata);
	if (print)
		print(userdata, store_read(tree->store, node->data));

	if(node->right != NULL)
		sbtree_print(tree, node->right, print, userdata);
}

static int bintree_depth(struct btree *tree, struct btree_node *node)
{
	if ( node == NULL)
		return 0;
	else {
		int ld = bintree_depth(tree, node->left);
		int rd = bintree_depth(tree, node->right);

		return 1 + (ld >rd ? ld : rd);
	}
}

static int bintree_isbalance(struct btree *tree, struct btree_node *node)
{
	int dis, ret = 0;

	if (node == NULL) 
		return 0;

	dis = bintree_depth(tree, node->left) - bintree_depth(tree, node->right);

	if (dis > 1 || dis <- 1 )
		ret = 1;
	else
		ret = bintree_isbalance(tree, node->left) && bintree_isbalance(tree, node->right);

	return ret;
}

static int sbtree_isbalance(struct btree *tree)
{
	return bintree_isbalance(tree, tree->root) == 0;
}

