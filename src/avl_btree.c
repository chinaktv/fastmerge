#include  <stdio.h>
#include  <stdlib.h>
#include  <memory.h>
#include  <stdarg.h>

#include  "btree.h"
#include  "store.h"
#include  "info.h"

#define REC_DEPTH 0

struct btree *btree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*))
{
	struct btree * tree = (struct btree *) malloc(sizeof(struct btree));
	memset(tree, 0, sizeof(struct btree));

	tree->oStore    = store;
	tree->nStore    = store_open_memory(sizeof(struct btree_node), 1024);
	tree->compare   = compare;
	tree->insert_eq = insert_eq;

	tree->root = NULL;

	return tree;
}

static struct btree_node *node_new(struct btree *tree, void *data, const char *key)
{
	struct btree_node * p_node = (struct btree_node *)calloc(1, sizeof(struct btree_node));

	p_node->left = p_node->right = p_node->parent = NULL;
	p_node->data = store_new_write(tree->oStore, data);;

	p_node->balance = '=';

	if (key)
		strncpy(p_node->key, key, 20);
	else
		p_node->key[0] = '\0';

	return p_node;
}

static int btree_rotate_left(struct btree *tree, off_t node_idx)
{
	int r
}

static int btree_rotate_right(struct btree *tree, off_t node_idx)
{
}

static int bintree_fixAfterInsertion(struct btree *tree, struct tree_node *ancestor, struct tree_node *inserted)
{
	if (ancestor == NULL) {
		/* 
		* 情况1：ancestor的值为null，即被插入Entry对象的每个祖先的平衡因子都是 =，此时新增节点后 
		* ，树的高度不会发生变化，所以不需要旋转，我们要作的就是调整从根节点到插入节点的路径上的所有 
		* 节点的平衡因子罢了 
		*  
		*                       新增55后调整 
		*             50            →           50 
		*             /=\                       /R\ 
		*           25   70                   25   70 
		*          /=\   /=\                 /=\   /L\ 
		*         15 30 60 90               15 30 60 90 
		*         =  =  =   =               =  =  /L = 
		*                                        55 
		*                                        = 
		*/  
		//根节点的平衡因子我们单独拿出来调整，因为adjustPath的参数好比是一个开区间，它不包括两头参数  
		//本身，而是从nserted.paraent开始到to的子节点为止，所以需单独调整，其他分支也一样  
		
		struct tree_node *root = store_read(tree->nStore, tree->root);
		if (strcmp(inserted->key, root->key) < 0)
			root->balance = 'L';
		else
			root->balance = 'R';

		//再调用adjustPath方法调整新增节点的父节点到root的某子节点路径上的所有祖先节点的平衡因子  
		adjustPath(root, inserted);  
        } else if ((ancestor->balance == 'L' && strcmp(inserted->key, ancestor->key) > 0)  
                || (ancestor->balance == 'R' && strcmp(inserted->key, ancestor->key) < 0)) {  
            /* 
             * 情况2： 
             * ancestor->balance的值为 L，且在ancestor对象的右子树插入，或ancestor->balanceFac 
             * tor的值为 R，且在ancestor对象的左子树插入，这两插入方式都不会引起树的高度发生变化，所以我们 
             * 也不需要旋转，直接调整平衡因子即可 
             *                      新增55后调整 
             *  ancestor → 50           →              50 
             *            /L\                         /=\ 
             *          25   70                     25   70 
             *         /R\   /=\                   /R\   /L\ 
             *        15 30 60 90                 15 30 60 90 
             *           /L                          /L /L 
             *          28                          28 55 
             *                      新增28后调整 
             *  ancestor → 50            →             50 
             *            /R\                         /=\ 
             *          25   70                     25   70 
             *         /=\   /L\                   /R\   /L\ 
             *        15 30 60 90                 15 30 60 90 
             *              /L                       /L /L 
             *             55                       28 55 
             */  
            //先设置ancestor的平衡因子为 平衡  
            ancestor->balance = '=';  
            //然后按照一般策略对inserted与ancestor间的元素进行调整  
            adjustPath(ancestor, inserted);  
        } else if (ancestor->balance == 'R' && elem.compareTo(ancestor->right.elem) > 0) {  
            /* 
             * 情况3： 
             * ancestor->balance的值为 R，并且被插入的Entry位于ancestor的右子树的右子树上， 此 
             * 种情况下会引起树的不平衡，所以先需绕ancestor进行旋转，再进行平衡因子的调整 
             * 
             * 新增93                          先调整因子再绕70左旋 
             *   →         50                    →           50 
             *            /R\                                /R\ 
             *          25   70  ← ancestor                25  90 
             *         /L    /R\                          /L   /=\ 
             *        15    60 90                        15  70   98 
             *        =     =  /=\                       =  /=\   /L 
             *                80  98                       60 80 93 
             *                =   /=                       =  =  = 
             *                   93 
             *                   = 
             */  
            ancestor->balance = '=';  
            //围绕ancestor执行一次左旋  
            rotateLeft(ancestor);  
            //再调整ancestor->paraent（90）到新增节点路径上祖先节点平衡  
            adjustPath(ancestor->parent, inserted);  
        } else if (ancestor->balance == 'L'  
                && elem.compareTo(ancestor->left.elem) < 0) {  
            /* 
             * 情况4： 
             * ancestor->balance的值为 L，并且被插入的Entry位于ancestor的左子树的左子树上， 
             * 此种情况下会引起树的不平衡，所以先需绕ancestor进行旋转，再进行平衡因子的调整 
             *  
             * 新增13                         先调整因子再绕50右旋 
             *   →         50 ← ancestor        →            20 
             *            /L\                                /=\ 
             *          20   70                            10   50 
             *         /=\   /=\                          /R\   /=\ 
             *        10 30 60 90                        5  15 30  70 
             *       /=\ /=\=  =                         = /L /=\  /=\ 
             *      5 15 25 35                            13 25 35 60 90 
             *      = /= = =                              =  =  =  =  =  
             *       13          
             *       =         
             */  
            ancestor->balance = '=';  
            //围绕ancestor执行一次右旋  
            rotateRight(ancestor);  
            //再调整ancestor->paraent（20）到新增节点路径上祖先节点平衡  
            adjustPath(ancestor->parent, inserted);  
        } else if (ancestor->balance == 'L'  
                && elem.compareTo(ancestor->left.elem) > 0) {  
            /* 
             * 情况5： 
             * ancestor->balance的值为 L，并且被插入的Entry位于ancestor的左子树的右子树上。此 
             * 种情况也会导致树不平衡，此种与第6种一样都需要执行两次旋转（执行一次绕ancestor的左子节点左 
             * 旋，接着执行一次绕ancestor执行一次右旋）后，树才平衡，最后还需调用 左-右旋 专有方法进行平衡 
             * 因子的调整 
             */  
            rotateLeft(ancestor->left);  
            rotateRight(ancestor);  
            //旋转后调用 左-右旋 专有方法进行平衡因子的调整  
            adjustLeftRigth(ancestor, inserted);  
        } else if (ancestor->balance == 'R'  
                && elem.compareTo(ancestor->right.elem) < 0) {  
  
            /* 
             * 情况6： 
             * ancestor->balance的值为 R，并且被插入的Entry位于ancestor的右子树的 左子树上，此 
             * 种情况也会导致树不平衡，此种与第5种一样都需要执行两次旋转（执行一次绕ancestor的右子节点右旋 
             * ，接着执行一次绕ancestor执行一次左旋）后，树才平衡，最后还需调用 右-左旋 专有方法进行平衡因 
             * 子的调整 
             */  
            rotateRight(ancestor->right);  
            rotateLeft(ancestor);  
            //旋转后调用 右-左旋 专有方法进行平衡因子的调整  
            adjustRigthLeft(ancestor, inserted);  
        }  
}

void btree_insert(struct btree * tree, void *data, const char *key)
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
				void * pptr_data_ptr = store_read(tree->oStore, thiz->data);
				if (tree->insert_eq(pptr_data_ptr, data) == 0)
					store_write(tree->oStore, thiz->data, pptr_data_ptr);
				store_release(tree->oStore, thiz->data, pptr_data_ptr);
			}
			break;
		}
		if (cmp < 0) {
			if (thiz->left != NULL)
				thiz = thiz->left;
			else {
				thiz->left = node_new(tree, data, key);

				break;
			}
		}
		else {
			if (thiz->right != NULL)
				thiz = thiz->right;
			else {
				thiz->right = node_new(tree, data, key);

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
			if (depth)
				*depth += 1;
			ret = bintree_find (tree, left, key, depth);
		}
	} 
	else if (cmp > 0){
		if(node->right == NULL) {
			ret = 0; 
		} else {
			struct btree_node * right = node->right;

			if (depth)
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
//	store_close(tree->nStore);

	free(tree);
}

static void print_subtree(struct btree * tree, struct btree_node * node, void (*print)(void *, void*), void *userdata)
{
	if  ( node == NULL )
		return;

	if(node->left != NULL)
		print_subtree(tree, node->left, print, userdata);
	if (print)
		print(userdata, store_read(tree->oStore, node->data));

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

