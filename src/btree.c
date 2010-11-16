#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <assert.h>

#include "btree.h"
#include "store.h"
#include "info.h"

#define ISNULL    -1
#define REC_DEPTH 0

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*))
{
	struct btree * tree = (struct btree *) malloc(sizeof(struct btree));
	assert(tree);
	memset(tree, 0, sizeof(struct btree));

	tree->oStore    = store;
	tree->nStore    = store_open_memory(sizeof(struct btree_node), 100);
	tree->compare   = compare;
	tree->insert_eq = insert_eq;

	tree->root = ISNULL;

	return tree;
}

static struct btree_node *node_new(struct btree *tree, off_t new_idx, off_t data, const char *key)
{
	struct btree_node * p_node = store_read(tree->nStore, new_idx);
	p_node->left = ISNULL;
	p_node->right = ISNULL;
	p_node->data = data;

	if (key)
		strncpy(p_node->key, key, 20);
	else
		p_node->key[0] = '\0';

	return p_node;
}

static void bintree_insert(struct btree * tree, void *data, const char *key, int *add, int *update)
{
	struct btree_node * thiz;

	off_t thiz_idx = tree->root;
	off_t *next_read;

	thiz = store_read(tree->nStore, thiz_idx);

	while  ( thiz != NULL ) {
		int cmp = strcmp(key, thiz->key);
		if (cmp == 0) {
			if (tree->insert_eq) {
				void * pptr_data_ptr = store_read(tree->oStore, thiz->data);
				if (tree->insert_eq(pptr_data_ptr, data) == 0) {
					store_write(tree->oStore, thiz->data, pptr_data_ptr);
					(*update)++;
				}
				store_release(tree->oStore, thiz->data, pptr_data_ptr);
			}
			break;
		}
		else if (cmp > 0) {
			next_read = &thiz->right;
		}
		else {
			next_read = &thiz->left;
		}
		if (*next_read == ISNULL) {
			struct btree_node * new;
			off_t data_idx = store_new_write(tree->oStore, data);
			off_t new_idx = store_new(tree->nStore);

			new = node_new(tree, new_idx, data_idx, key);
			store_write(tree->nStore, new_idx, new);
			store_release(tree->nStore, new_idx, new);

			*next_read = new_idx;
			store_write(tree->nStore, thiz_idx, thiz);
			(*add)++;
		}
		store_release(tree->nStore, thiz_idx, thiz);

		thiz = store_read(tree->nStore, *next_read);
		thiz_idx = *next_read;
	}
}

void btree_insert(struct btree * tree, void *data, const char *key, int *add, int *update)
{
	if(tree->root == ISNULL) {
		struct btree_node * node;
		off_t node_idx = store_new(tree->nStore);
		off_t data_idx = store_new_write(tree->oStore, data);

		node = node_new(tree, node_idx, data_idx, key);
		store_write(tree->nStore, node_idx, node);
		store_release(tree->nStore, node_idx, node);

		tree->root = node_idx;
		(*add)++;

	} else
		bintree_insert( tree, data, key, add, update);

	tree->entries_num++;
}

static int bintree_find(struct btree * tree, struct btree_node * node, const char *key, int *depth)
{
	int ret = 1;
	int cmp = strncmp(key, node->key, sizeof(node->key));

	if (cmp < 0) {
		if(node->left == ISNULL) {
			ret = 0;
		} else {
			struct btree_node * left = store_read(tree->nStore, node->left);
			*depth += 1;
			ret = bintree_find (tree, left, key, depth);
			store_release(tree->nStore, node->left, left);
		}
	} 
	else if (cmp > 0){
		if(node->right == ISNULL) {
			ret = 0; 
		} else {
			struct btree_node * right = store_read(tree->nStore, node->right);

			*depth += 1;
			ret = bintree_find (tree, right, key, depth);
			store_release(tree->nStore, node->right, right);
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
	struct btree_node * root = store_read(tree->nStore, tree->root);
	ret = bintree_find(tree, root, key, &depth);
	store_release(tree->nStore, tree->root, root);

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
	struct btree_node * root = store_read(tree->nStore, 0);

	root->data = tree->root;
	root->left = tree->entries_num;

	store_write  (tree->nStore, 0, root);
	store_release(tree->nStore, 0, root);

	store_close(tree->nStore);

	free(tree);
}

static void print_subtree(struct btree * tree, struct btree_node * node, void (*print)(void *, void*), void *userdata)
{
	if  ( node == NULL )
		return;

	if(node->left != ISNULL) {
		struct btree_node * left = store_read(tree->nStore, node->left);

		print_subtree(tree, left, print, userdata);
		store_release(tree->nStore, node->left, left);
	}
	if (print)
		print(userdata, store_read(tree->oStore, node->data));

	if(node->right != ISNULL) {
		struct btree_node * right = store_read(tree->nStore, node->right);

		print_subtree(tree, right, print, userdata);
		store_release(tree->nStore, node->right, right);
	}
}

void btree_print(struct btree * tree, void (*print)(void *, void*), void *userdata)
{
	struct btree_node *node;
	if  ( tree->root == ISNULL ) 
		return;

	node = store_read(tree->nStore, tree->root);
	print_subtree(tree, node, print, userdata);
	store_release(tree->nStore, tree->root, node);
}

#if 0
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

#endif

