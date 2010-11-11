#include  <stdio.h>
#include  <stdlib.h>
#include  <assert.h>
#include  <memory.h>
#include  <stdarg.h>

#include  "btree.h"
#include  "store.h"
#include  "info.h"

#define ISNULL -1

static void node_init  (struct btree_node * node, off_t data );
static int bintree_find(struct btree *tree, const struct btree_node * node, off_t node_idx, void * find_data);

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

static void node_init( struct btree_node * p_node, off_t data )
{
	p_node->left = ISNULL;
	p_node->right = ISNULL;
	p_node->father = ISNULL;

	p_node->data = data;
}

static void bintree_insert(struct btree * tree, off_t data)
{
	struct btree_node * pptr;

	off_t new_idx;
	struct btree_node * new;
	off_t last_read = tree->root;
	off_t next_read;

	void * data_ptr = store_read(tree->oStore, data);

	pptr= store_read(tree->nStore, tree->root);

	new_idx = store_new(tree->nStore);
	new = store_read(tree->nStore, new_idx);
	node_init(new, data);

	while  ( pptr != NULL ) {
		void * pptr_data_ptr = store_read(tree->oStore, pptr->data);
		int cmp = tree->compare(data_ptr, pptr_data_ptr);
		store_release(tree->oStore, pptr->data);
		if(cmp == GT) {
			next_read = pptr->right;
			if(next_read == ISNULL) {
				pptr->right = new_idx;
				new->father = last_read;
				store_write(tree->nStore, last_read);
			}

		} else if (cmp == LT){
			next_read = pptr->left;
			if(next_read == ISNULL) {
				pptr->left = new_idx;
				new->father = last_read;
				store_write(tree->nStore, last_read);
			}

		}
		else {
			if (tree->insert_eq)
				tree->insert_eq(pptr_data_ptr, data_ptr);
			pptr = NULL;
			break;
		}
		store_release(tree->nStore, last_read);
		if(next_read != ISNULL) {
			pptr = store_read(tree->nStore, next_read);
			last_read = next_read;
		} else {
			pptr = NULL;
		}
	}

	store_release(tree->oStore, data);

	store_write(tree->nStore, new_idx);
	store_release(tree->nStore, new_idx);
}

void btree_insert(struct btree * tree, off_t data)
{
	if(tree->root == ISNULL) {
		off_t node_idx;
		struct btree_node * node;

		assert(tree->entries_num == 0);

		node_idx = store_new(tree->nStore);
		node = store_read(tree->nStore, node_idx);
		node_init(node,data);
		store_write(tree->nStore, node_idx);
		store_release(tree->nStore, node_idx);

		tree->root = node_idx;

	} else
		bintree_insert( tree, data );

	tree->entries_num++;
}

void btree_close( struct btree * tree )
{
	struct btree_node * root = store_read(tree->nStore, 0);

	root->data = tree->root;
	root->left = tree->entries_num;

	store_write  (tree->nStore, 0);
	store_release(tree->nStore, 0);

	assert( tree != NULL );

	store_close(tree->nStore);

	free(tree);
}

int btree_find(struct btree * tree, void* find_data) 
{ 
	int ret;
	const struct btree_node * root = store_read(tree->nStore, tree->root);
	ret = bintree_find(tree, root, tree->root, find_data);
	store_release(tree->nStore, tree->root);
	return ret;
}

static int bintree_find(struct btree * tree, const struct btree_node * node, off_t node_idx, void * find_data)
{
	int ret = -1;
	int cmp ;
	void * node_data_ptr;

	if (node == NULL) 
		printf ("Item not found in tree\n");

	node_data_ptr = store_read(tree->oStore, node->data);
	cmp = tree->compare(find_data, node_data_ptr);
	store_release(tree->oStore, node->data);

	if(cmp == EQ) {
		ret = 1;
	} else if (cmp == LT) {
		if(node->left == ISNULL) {
			ret = 0;
		} else {
			struct btree_node * left = store_read(tree->nStore, node->left);

			assert(left->father == node_idx);
			ret = bintree_find (tree, left, node->left, find_data);
			store_release(tree->nStore, node->left);
		}
	} 
	else {
		if(node->right == ISNULL) {
			ret = 0; 
		} 
		else {
			struct btree_node * right = store_read(tree->nStore, node->right);

			assert(right->father == node_idx);
			ret = bintree_find (tree, right, node->right, find_data);
			store_release(tree->nStore, node->right);
		}
	}

	return ret; 
} 

static void print_subtree(struct btree * tree, struct btree_node * node, void (*print)(void *, void*), void *userdata)
{
	if  ( node == NULL )
		return;

	if(node->left != ISNULL) {
		struct btree_node * left = store_read(tree->nStore, node->left);

		print_subtree(tree, left, print, userdata);
		store_release(tree->nStore, node->left);
	}
	if (print)
		print(userdata, store_read(tree->oStore, node->data));

	if(node->right != ISNULL) {
		struct btree_node * right = store_read(tree->nStore, node->right);

		print_subtree(tree, right, print, userdata);
		store_release(tree->nStore, node->right);
	}
}

void btree_print(struct btree * tree, void (*print)(void *, void*), void *userdata)
{
	if  ( tree->root == ISNULL ) 
		return;
	print_subtree(tree, store_read(tree->nStore, tree->root), print, userdata);
	store_release(tree->nStore, tree->root);
}

