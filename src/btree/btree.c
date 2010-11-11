#include  <stdio.h>
#include  <stdlib.h>
#include  <assert.h>
#include  <memory.h>
#include  <stdarg.h>

#include  "btree.h"
#include  "store.h"

#define ISNULL -1

static void node_init(struct btree_node * node, off_t data );
int bintree_find     (struct btree * tree, const struct btree_node * node, off_t node_idx, void * find_data);

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

/*-- Code for inserting new node -------------------------------*/
static void node_init( struct btree_node * p_node, off_t data )
{
	p_node->left = ISNULL;
	p_node->right = ISNULL;
	p_node->father = ISNULL;

	p_node->data = data;
}

static void bintree_insert(struct btree * tree, off_t data)
{
	struct btree_node * pptr;//, * father;

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

	assert( tree != NULL );
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

static off_t write_ptr_to_ptr( struct btree * tree, struct btree_node * ptr, off_t ptr_idx, off_t new )
{

	off_t ret = ISNULL;
	struct btree_node * ptr_father;

	assert( ptr != NULL );

	if ( ptr->father == ISNULL ) {
		ret =  tree->root;
		tree->root = new;
	} else {  
		ptr_father = store_read(tree->nStore, ptr->father);
		if  ( ptr_father->left == ptr_idx ) {
			ret =  ptr_father->left;
			ptr_father->left = new;
		} else if  ( ptr_father->right == ptr_idx ) {
			ret =  ptr_father->right;
			ptr_father->right = new;
		}
		store_write(tree->nStore, ptr->father);
		store_release(tree->nStore, ptr->father);
	}

	assert(ret != ISNULL);

	return  ret;
}

static void btree_rotate_left( struct btree  * tree, struct btree_node * a, off_t a_idx )
{
	struct btree_node /* ** pp_a, */ * b, * c, * d, * e;
	off_t b_idx, c_idx, d_idx, e_idx;
	off_t father_a;

	//   a= store_read(tree->nStore, a_idx);    
	assert(a != NULL  &&  a->right != ISNULL);

	b_idx = a->left;
	c_idx = a->right;

	if(b_idx == ISNULL) {
		b = NULL; 
	} else {
		b = store_read(tree->nStore, b_idx);
	}
	if(c_idx == ISNULL) {
		c = NULL; 
	} else {
		c = store_read(tree->nStore, c_idx);
	}


	d_idx = c->left;
	e_idx = c->right;

	if(d_idx == ISNULL) {
		d = NULL; 
	} else {
		d = store_read(tree->nStore, d_idx);
	}
	if(e_idx == ISNULL) {
		e = NULL; 
	} else {
		e = store_read(tree->nStore, e_idx);
	}

	write_ptr_to_ptr(tree, a, a_idx, c_idx);

	c->left = a_idx;
	c->right = e_idx;

	a->left = b_idx;
	a->right = d_idx;

	/* fix father pointers */
	father_a = a->father;

	a->father = c_idx;
	if ( e != NULL )
		e->father = c_idx;

	if ( b != NULL )
		b->father = a_idx;
	if ( d != NULL )
		d->father = a_idx;

	c->father = father_a;

	if(a_idx != ISNULL){ store_write(tree->nStore, a_idx); }
	if(b_idx != ISNULL){ store_write(tree->nStore, b_idx); store_release(tree->nStore, b_idx); }
	if(c_idx != ISNULL){ store_write(tree->nStore, c_idx); store_release(tree->nStore, c_idx); }
	if(d_idx != ISNULL){ store_write(tree->nStore, d_idx); store_release(tree->nStore, d_idx); }
	if(e_idx != ISNULL){ store_write(tree->nStore, e_idx); store_release(tree->nStore, e_idx); }

}

static void btree_rotate_right( struct btree * tree, struct btree_node * a, off_t a_idx )
{
	struct btree_node /* ** pp_a, */ * b, * c, * d, * e;

	off_t father_a;

	off_t b_idx, c_idx, d_idx, e_idx;

	assert( a != NULL  &&  a->left != ISNULL );

	b_idx = a->right;
	c_idx = a->left;

	if(b_idx == ISNULL) {
		b = NULL; 
	} else {
		b = store_read(tree->nStore, b_idx);
	}
	if(c_idx == ISNULL) {
		c = NULL; 
	} else {
		c = store_read(tree->nStore, c_idx);
	}

	d_idx = c->right;
	e_idx = c->left;


	if(d_idx == ISNULL) 
		d = NULL; 
	else 
		d = store_read(tree->nStore, d_idx);
	if(e_idx == ISNULL) 
		e = NULL; 
	else 
		e = store_read(tree->nStore, e_idx);

	write_ptr_to_ptr( tree, a, a_idx, c_idx);
	c->right = a_idx;
	c->left = e_idx;

	a->right = b_idx;
	a->left = d_idx;

	/* fix father pointers */
	father_a = a->father;

	a->father = c_idx;
	if ( e != NULL )
		e->father = c_idx;

	if ( b != NULL )
		b->father = a_idx;
	if ( d != NULL )
		d->father = a_idx;

	c->father = father_a;

	if(a_idx != ISNULL) {store_write(tree->nStore, a_idx); }
	if(b_idx != ISNULL) {store_write(tree->nStore, b_idx); store_release(tree->nStore, b_idx);}
	if(c_idx != ISNULL) {store_write(tree->nStore, c_idx); store_release(tree->nStore, c_idx);}
	if(d_idx != ISNULL) {store_write(tree->nStore, d_idx); store_release(tree->nStore, d_idx);}
	if(e_idx != ISNULL) {store_write(tree->nStore, e_idx); store_release(tree->nStore, e_idx);}

}

static void btree_rotate_up( struct btree * tree, struct btree_node * p_node, off_t p_node_idx )
{
	struct btree_node * father_ptr;
	off_t last_read;
	assert( p_node != NULL );

	/* is p_node the root of the tree? */
	if  ( p_node->father == ISNULL ) {
		//        assert( tree->root == p_node );

		return;
	}

	father_ptr = store_read(tree->nStore, p_node->father);
	last_read = p_node->father;

	if  ( father_ptr->left == p_node_idx ) {
		btree_rotate_right( tree, father_ptr, p_node->father);

	} else if  ( father_ptr->right == p_node_idx ) {
		btree_rotate_left( tree, father_ptr, p_node->father );
	} else {
		assert(FALSE); /* something is wrong ! */
	} 
	store_release(tree->nStore, last_read);
}

static off_t get_valid_son( struct btree_node * p_node )
{
	off_t ret;

	if ( p_node->right == ISNULL )
		ret = p_node->left;
	else 
		ret = p_node->right;

	return ret;
}

void btree_delete_node( struct btree * tree, struct btree_node * p_node, off_t p_node_idx )
{
	off_t son_idx;

	assert( p_node != NULL );
	while  ( p_node->left != ISNULL  ||  p_node->right != ISNULL ) {
		son_idx = get_valid_son( p_node );

		btree_rotate_up( tree, store_read(tree->nStore, son_idx), son_idx );
		store_release(tree->nStore, son_idx);
	}


	write_ptr_to_ptr(tree, p_node, p_node_idx, ISNULL);

	tree->entries_num--;
}

int btree_delete(struct btree * tree, void* data)
{
	int ret = FALSE;
	off_t last_read;
	struct btree_node * p_node;

	assert( tree != NULL );
	if(tree->root == ISNULL) {
		return FALSE;
	}

	p_node = store_read(tree->nStore, tree->root);

	last_read = tree->root;
	while  ( p_node != NULL ) {
		void * p_node_data_ptr = store_read(tree->oStore, p_node->data);
		int cmp = tree->compare(data, p_node_data_ptr);
		store_release(tree->oStore, p_node->data);
		if  ( cmp == 0 ) {
			btree_delete_node( tree, p_node, last_read );
			store_release(tree->nStore, last_read);
			ret = TRUE;
			p_node = NULL;
		} else {
			off_t next_read;
			if(cmp == GT) {
				if(p_node->right != ISNULL) {
					next_read = p_node->right;
				} else {
					next_read = ISNULL;
				}
			} else {
				if(p_node->left != ISNULL) {
					next_read = p_node->left;
				} else {
					next_read = ISNULL;
				}
			}
			store_release(tree->nStore, last_read);
			if(next_read != ISNULL) {
				p_node = store_read(tree->nStore, next_read);
				last_read = next_read;
			} else {
				p_node = NULL;
			}
		}

	}

	return ret;
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

int btree_find (struct btree * tree, void* find_data) 
{ 
	int ret;
	const struct btree_node * root = store_read(tree->nStore, tree->root);
	ret = bintree_find(tree, root, tree->root, find_data);
	store_release(tree->nStore, tree->root);
	return ret;
}

int bintree_find (struct btree * tree, const struct btree_node * node, off_t node_idx, void * find_data)
{
	int ret = -1;
	int cmp ;
	void * node_data_ptr;

	if (node == NULL) {
		printf ("Item not found in tree\n");
	}

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
			ret = bintree_find (tree, left, node->left, find_data /*, depth+1*/);
			store_release(tree->nStore, node->left);
		}
	} else {
		if(node->right == ISNULL) {
			ret = 0; 
		} else {
			struct btree_node * right = store_read(tree->nStore, node->right);

			assert(right->father == node_idx);
			ret = bintree_find (tree, right, node->right, find_data /*, depth+1*/);
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
	if  ( tree->root == ISNULL ) {
		printf( "@    <<EMPTY BTREE>>\n" );
		return;
	}
	print_subtree(tree, store_read(tree->nStore, tree->root), print, userdata);
	store_release(tree->nStore, tree->root);
	printf( "\n" );

	fflush( stdout );
	fflush( stderr );
}
