#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>
#include  <stdarg.h>

#include  "btree.h"
#include  "store.h"
#include  "info.h"

#define ISNULL -1

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*))
{
	struct btree * tree = (struct btree *) malloc(sizeof(struct btree));
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

	strncpy(p_node->key, key, 20);

	return p_node;
}

static int btreen_node_compare(struct btree_node *a, struct btree_node *b)
{
	int x = strncmp(a->key, b->key, 17);
	if (x < 0)
		return LT;
	else if (x > 0)
		return GT;

	return EQ;
}

static void btree_update(struct btree *tree, struct btree_node *node, void *data)
{
	void *pptr_data_ptr = store_read(tree->oStore, node->data);
	
	memcpy(pptr_data_ptr, data, store_blockSize(tree->oStore));
	store_write(tree->oStore, node->data, pptr_data_ptr);
	store_release(tree->oStore, node->data, pptr_data_ptr);
}

static void bintree_insert(struct btree * tree, void *data, const char *key)
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
				if (tree->insert_eq(pptr_data_ptr, data) == 0)
					store_write(tree->oStore, thiz->data, pptr_data_ptr);
				store_release(tree->oStore, thiz->data, pptr_data_ptr);
			}

			thiz = NULL;
			break;
		}
		else if (cmp > 0)
			next_read = &thiz->right;
		else
			next_read = &thiz->left;

		if (*next_read == ISNULL) {
			struct btree_node * new;
			off_t data_idx = store_new_write(tree->oStore, data);
			off_t new_idx = store_new(tree->nStore);

			new = node_new(tree, new_idx, data_idx, key);
			store_write(tree->nStore, new_idx, new);
			store_release(tree->nStore, new_idx, new);

			*next_read = new_idx;
			store_write(tree->nStore, thiz_idx, thiz);

			break;
		}
		store_release(tree->nStore, thiz_idx, thiz);

		thiz = store_read(tree->nStore, *next_read);
		thiz_idx = *next_read;
	}
}

void btree_insert(struct btree * tree, void *data, const char *key)
{
	if(tree->root == ISNULL) {
		struct btree_node * node;
		off_t node_idx = store_new(tree->nStore);
		off_t data_idx = store_new_write(tree->oStore, data);

		node = node_new(tree, node_idx, data_idx, key);
		store_write(tree->nStore, node_idx, node);
		store_release(tree->nStore, node_idx, node);

		tree->root = node_idx;

	} else
		bintree_insert( tree, data, key);

	tree->entries_num++;
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

