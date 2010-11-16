#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <assert.h>

#include "btree.h"
#include "store.h"
#include "info.h"

#define REC_DEPTH 0

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*))
{
	struct btree * tree;

	tree = (struct btree *) malloc(sizeof(struct btree));
	assert(tree);

	memset(tree, 0, sizeof(struct btree));

	tree->store       = store;
	tree->compare     = compare;
	tree->insert_eq   = insert_eq;
	tree->root        = NULL;

	INIT_LIST_HEAD(&tree->node_head);
	tree->node_pool = (struct btree_node_head *)calloc(1, sizeof(struct btree_node_head));

	tree->node_pool->used_id = 0;
	list_add(&tree->node_pool->head, &tree->node_head);

	return tree;
}

static struct btree_node *node_new(struct btree *tree, void *data, const char *key)
{
	struct btree_node * p_node;

	if (tree->node_pool->used_id >= ALLOC_NUM) {
		tree->node_pool = (struct btree_node_head *) calloc(1, sizeof(struct btree_node_head));
		tree->node_pool->used_id = 0;
		list_add(&tree->node_pool->head, &tree->node_head);
	}

	p_node = tree->node_pool->node + tree->node_pool->used_id++;

	p_node->left = p_node->right = p_node->parent = NULL;
	p_node->data = store_new_write(tree->store, data);;

	if (key)
		strncpy(p_node->key, key, 20);
	else
		p_node->key[0] = '\0';

	return p_node;
}

void btree_insert(struct btree * tree, void *data, const char *key, int *add, int *update)
{
	struct btree_node *thiz = tree->root;

	if (thiz == NULL) {
		tree->root = node_new(tree, data, key);
		tree->entries_num++;

		return;
	}

	while  ( thiz != NULL ) {
		int cmp = strcmp(key, thiz->key);

		if (cmp == 0) {
			if (tree->insert_eq) {
				void * pptr_data_ptr = store_read(tree->store, thiz->data);
				if (tree->insert_eq(pptr_data_ptr, data) == 0) {
					store_write(tree->store, thiz->data, pptr_data_ptr);
					(*update)++;
				}
				store_release(tree->store, thiz->data, pptr_data_ptr);
			}
			break;
		}
		if (cmp < 0) {
			if (thiz->left != NULL)
				thiz = thiz->left;
			else {
				thiz->left = node_new(tree, data, key);
				(*add)++;

				break;
			}
		}
		else {
			if (thiz->right != NULL)
				thiz = thiz->right;
			else {
				thiz->right = node_new(tree, data, key);
				(*add)++;

				break;
			}
		}
	}
	tree->entries_num++;
}

static int bintree_find(struct btree * tree, struct btree_node * node, const char *key, int *depth)
{
	int ret = 1;
	int cmp = strncmp(key, node->key, sizeof(node->key));

	if (cmp < 0) {
		if(node->left == NULL) {
			ret = 0;
		} else {
			struct btree_node * left = node->left;
			*depth += 1;
			ret = bintree_find (tree, left, key, depth);
		}
	} 
	else if (cmp > 0){
		if(node->right == NULL) {
			ret = 0; 
		} else {
			struct btree_node * right = node->right;

			*depth += 1;
			ret = bintree_find (tree, right, key, depth);
		}
	}

	return ret; 
}

int btree_find(struct btree *tree, const char *key) 
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

void btree_close( struct btree * tree )
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

static void print_subtree(struct btree * tree, struct btree_node * node, void (*print)(void *, void*), void *userdata)
{
	if  ( node == NULL )
		return;

	if(node->left != NULL)
		print_subtree(tree, node->left, print, userdata);
	if (print)
		print(userdata, store_read(tree->store, node->data));

	if(node->right != NULL)
		print_subtree(tree, node->right, print, userdata);
}

void btree_print(struct btree * tree, void (*print)(void *, void*), void *userdata)
{
	print_subtree(tree, tree->root, print, userdata);
}

int bintree_depth(struct btree *tree, struct btree_node *node)
{
	if ( node == NULL)
		return 0;
	else {
		int ld = bintree_depth(tree, node->left);
		int rd = bintree_depth(tree, node->right);

		return 1 + (ld >rd ? ld : rd);
	}
}

int bintree_isbalance(struct btree *tree, struct btree_node *node)
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

int btree_isbalance(struct btree *tree)
{
	return bintree_isbalance(tree, tree->root) == 0;
}

