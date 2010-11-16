#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <assert.h>

#include "btree.h"
#include "store.h"
#include "info.h"

#define REC_DEPTH 0

static void avlbtree_print    (struct btree *tree, void (*print)(void *, void*), void *userdata);
static void avlbtree_insert   (struct btree *tree, void *data, const char *key, int *add, int *update);
static void avlbtree_close    (struct btree *tree);
static int  avlbtree_find     (struct btree *tree, const char *key);
static int  avlbtree_isbalance(struct btree *tree);

struct btree *avlbtree_new_memory(struct store *store, int (*compare)(const void*, const void*), int (*insert_eq)(void*, void*))
{
	struct btree * tree;

	tree = (struct btree *) malloc(sizeof(struct btree));
	assert(tree);

	memset(tree, 0, sizeof(struct btree));

	tree->store       = store;
	tree->compare     = compare;
	tree->insert_eq   = insert_eq;
	tree->root        = NULL;
	tree->entries_num = 0;

	INIT_LIST_HEAD(&tree->node_head);
	tree->node_pool = (struct btree_node_head *)calloc(1, sizeof(struct btree_node_head));

	tree->node_pool->used_id = 0;
	list_add(&tree->node_pool->head, &tree->node_head);

	tree->close     = avlbtree_close;
	tree->insert    = avlbtree_insert;
	tree->show      = avlbtree_print;
	tree->find      = avlbtree_find;
	tree->isbalance = avlbtree_isbalance;

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
	p_node->data = store_new_write(tree->store, data);
	p_node->balance = '=';

	if (key)
		strncpy(p_node->key, key, 20);
	else
		p_node->key[0] = '\0';

	return p_node;
}

static void btree_rotate_left(struct btree *tree, struct btree_node *ancestor)
{
	struct btree_node *r = ancestor->right;//旋转元素的右子节点  
        //把旋转元素的右子节点的左子节点接到旋转元素的右子树  
        ancestor->right = r->left;//①  
        //如果旋转元素的右子节点存在左子节点的话，同时修改该节点的父节指针指向  
        if (r->left != NULL) {  
            //把旋转元素的右子节点的左子节点的父设置成旋转元素  
            r->left->parent = ancestor;//②  
        }  
        //将旋转元素的右子节点的父设置成旋转元素的父，这里有可以p为根节点，那么旋转后p就成根节点  
        r->parent = ancestor->parent;//③  
  
        //如果旋转元素为根元素，则旋转后，旋转元素的右子节点r将成为根节点  
        if (ancestor->parent == NULL) {  
            tree->root = r;  
        }//否则p不是根节点，如果p是他父节点的左节点时  
        else if (ancestor->parent->left == ancestor) {  
            //让p的父节点的左指针指向r  
            ancestor->parent->left = r;  
        }//否则如果p是他父节点的右节点时  
        else {  
            //让p的父节点的右指针指向r  
            ancestor->parent->right = r;//④  
        }  
        //让旋转元素的左节点指向旋转元素p  
        r->left = ancestor;//⑤  
        //让旋转元素的父节点指向旋转元素右节点r  
        ancestor->parent = r;//⑥  
}

static void btree_rotate_right(struct btree *tree, struct btree_node *ancestor)
{
        struct btree_node *l = ancestor->left;  
        
	ancestor->left = l->right;  
        if (l->right != NULL) {  
            l->right->parent = ancestor;  
        }  
        l->parent = ancestor->parent;  
  
        if (ancestor->parent == NULL) {  
            tree->root = l;  
        } else if (ancestor->parent->right == ancestor) {
            ancestor->parent->right = l;  
        } else {  
            ancestor->parent->left = l;  
        }  
        l->right = ancestor;  
        ancestor->parent = l;  
}

static void btree_adjust_path(struct btree_node *to, struct btree_node *inserted) 
{  
    struct btree_node *tmp = inserted->parent;  

    //从新增节点的父节点开始向上回溯调整，直到结尾节点to止  
    while (tmp != to) {  
        /* 
         * 插入30，则在25右边插入，这样父节点平衡会被打破，右子树就会比左子树高1，所以平衡因子为R；祖 
         * 先节点50的平衡因子也被打破，因为在50的左子树上插入的，所以对50来说，左子树会比右子树高1，所 
         * 以其平衡因子为L 
         *    50                      50 
         *    /=\       插入30        /L\ 
         *   25  70       →         25  70 
         *   =   =                   R\  = 
         *                            30 
         *                            =  
         */  
        //如果新增元素比祖先节点小，则是在祖先节点的左边插入，则自然该祖先的左比右子树会高1           
        if (strcmp(inserted->key, tmp->key) < 0) {  
  
            tmp->balance = 'L';  
        } else {  
            //否则会插到右边，那么祖先节点的右就会比左子树高1  
            tmp->balance = 'R';  
        }  
        tmp = tmp->parent;//再调整祖先的祖先  
    }  
}  

void adjustLeftRigth(struct btree_node *ancestor, struct btree_node *inserted) 
{
    if (ancestor->parent == inserted) {  
        /* 
         * 第1种，新增的节点在旋转完成后为ancestor父节点情况： 
         *  
         * 新增40                          绕30左旋                绕50右旋 
         *   →      50 ← ancestor         →         50         → 
         *          /L                              /L          
         *         30                              40   
         *         =\                             /= 
         *          40                           30 
         *          =                            = 
         *           
         *                    调整平衡因子 
         *          40            →            40 
         *          /=\                        /=\ 
         *         30 50                      30 50 
         *         =  L                       =   = 
         *          
         * 注，这里的 左-右旋 是在fixAfterInsertion方法中的第5种情况中完成的，在这里只是平衡因子的 
         * 调整，图是为了好说明问题而放在这个方法中的，下面的两个分支也是一样       
         */  
        ancestor->balance = '=';  
    } else if (strcmp(inserted->key, ancestor->parent->key) < 0) {
        /* 
         * 第2种，新增的节点在旋转完成后为不为ancestor父节点，且新增节点比旋转后ancestor的父节点要小 
         * 的情况 
         *  
         * 由于插入元素(35)比旋转后ancestor(50)的父节点(40)要小， 所以新增节点会在其左子树中 
         *  
         * 新增35                         绕20左旋 
         *   →      50 ← ancestor        →                 50 
         *          /L\                                    /L\ 
         *        20   90                                40   90 
         *       /=\   /=\                              /=\   /=\ 
         *     10  40 70  100                          20 45 70 100 
         *    /=\  /=\=   =                            /=\     
         *   5  15 30 45                              10  30    
         *   =  =  =\ =                              /=\   =\ 
         *          35                              5  15   35 
         *          =                               =  =    = 
         *           
         *  绕50右旋                      调整平衡因子 
         *     →        40                →                40 
         *              /=\                                /=\ 
         *             20  50                            20   50 
         *            /=\  /L\                          /=\   /R\ 
         *          10 30 45 90                        10 30 45  90 
         *         /=\  =\   /=\                      /=\  R\    /=\ 
         *        5  15  35 70 100                   5  15  35  70  100 
         *        =  =   =  =  =                     =  =   =   =   = 
         *           
         */  
        ancestor->balance = 'R';  
        //调整ancestor兄弟节点到插入点路径上节点平衡因子  
        btree_adjust_path(ancestor->parent->left, inserted);  
    } 
    else {  
        /* 
         * 第2种，新增的节点在旋转完成后为不为ancestor父节点，且新增节点比旋转后ancestor的父节点要大的 
         * 情况 
         *  
         * 由于插入元素(42)比旋转后ancestor(50)的父节点(40)要大，所以新增节点会在其右子树中 
         *  
         * 新增42                           绕20左旋 
         *   →      50 ← ancestor          →               50 
         *          /L\                                    /L\ 
         *        20   90                                40   90 
         *       /=\   /=\                              /=\    /=\ 
         *     10  40 70  100                          20  45 70 100 
         *    /=\  /=\=   =                           /=\  /= 
         *   5  15 30 45                             10 30 42   
         *   =  =  =  /=                             /=\=  = 
         *           42                             5  15    
         *           =                              =  =     
         *           
         * 绕50右旋                        调整平衡因子 
         *   →          40                 →               40 
         *              /=\                                /=\ 
         *             20  50                            20   50 
         *            /=\  /L\                          /L\   /=\ 
         *          10 30 45 90                        10 30 45  90 
         *         /=\   /=  /=\                      /=\    /L  /=\ 
         *        5  15 42  70 100                    5 15  42  70 100 
         *        =  =  =   =  =                      =  =  =   =  = 
         *           
         */  
        ancestor->parent->left->balance = 'L';  
  
        ancestor->balance = '=';  
        //调整ancestor节点到插入点路径上节点平衡因子  
        btree_adjust_path(ancestor, inserted);  
    }  
}  
  
/** 
 * 进行 右-左旋转 后平衡因子调整 
 *  
 * 与adjustLeftRigth方法一样，也有三种情况，这两个方法是对称的 
 * @param ancestor 
 * @param inserted 
 */  
void adjustRigthLeft(struct btree_node *ancestor, struct btree_node *inserted) 
{
    if (ancestor->parent == inserted) {  
        /* 
         * 第1种，新增的节点在旋转完成后为ancestor父节点情况：  
         *  
         * 新增40                          绕50右旋                绕30左旋  
         *   →       30 ← ancestor        →        30          → 
         *           R\                            R\          
         *            50                            40   
         *           /=                              =\ 
         *          40                                50 
         *          =                                 = 
         *           
         *          40         调整平衡因子         40 
         *          /=\           →            /=\ 
         *         30 50                      30  50 
         *         R  =                       =   = 
         *          
         * 注，这里的 右-左旋 是在fixAfterInsertion方法中的第6种情况中完成的，这里只是 平衡因子的调 
         * 整，图是为了好说明问题而放在这个方法中的，下面的两个分支也是一样       
         */  
        ancestor->balance = '=';  
    } else if (strcmp(inserted->key, ancestor->parent->key) > 0) {  
        /* 
         * 第2种，新增的节点在旋转完成后为不为ancestor父节点，且新增节点比旋转后 
         * ancestor的父节点要大的情况 
         *  
         * 由于插入元素(73)比旋转后ancestor(50)的父节点(70)要大， 所以新增节点会 
         * 在其右子树中 
         *  
         * 新增73                          绕90右旋 
         *   →       50 ← ancestor       →                  50 
         *          /R\                                    /R\ 
         *        20   90                                20   70 
         *       /=\   /=\                              /=\   /=\ 
         *     10  40 70  95                          10  40 65 90 
         *     =   = /=\  /=\                         =   =  =  /=\  
         *          65 75 93 97                                75  95 
         *          =  /= =  =                                 /=  /=\ 
         *            73                                      73  93 97 
         *            =     
         *                           
         * 绕50左旋                       调整平衡因子 
         *   →          70                →                70 
         *              /=\                                /=\ 
         *            50   90                            50   90 
         *           /R\   /=\                          /L\   /=\ 
         *          20 65 75  95                       20 65 75  95 
         *         /=\ =  /=  /=\                     /=\ =  /L  /=\ 
         *        10  40 73  93 97                   10  40 73  93 97 
         *        =   =   =   =  =                   =   =  =   =   = 
         *           
         */  
        ancestor->balance = 'L';  
        btree_adjust_path(ancestor->parent->right, inserted);  
    } else {  
        /* 
         * 第2种，新增的节点在旋转完成后为不为ancestor父节点，且新增节点比旋转后ancestor的父节点要小 
         * 的情况 
         *  
         * 由于插入元素(73)比旋转后ancestor(50)的父节点(70)要大， 所以新增节点会在其右子树中 
         *  
         * 新增63                          绕90右旋 
         *   →       50 ← ancestor       →                 50 
         *          /R\                                    /R\ 
         *        20   90                                20   70 
         *       /=\   /=\                              /=\   /=\ 
         *     10  40 70  95                          10  40 65 90 
         *     =   = /=\  /=\                         =   =  /= /=\  
         *          65 75 93 97                             63 75 95 
         *          /= =  =  =                              =  =  /=\ 
         *         63                                            93 97 
         *         =                                             =  = 
         *                           
         * 绕50左旋                       调整平衡因子 
         *   →          70                →                70 
         *              /=\                                /=\ 
         *            50   90                            50   90 
         *           /R\   /=\                          /=\   /R\ 
         *         20  65 75 95                       20  65 75 95 
         *        /=\  /= =  /=\                     /=\  /L    /=\ 
         *       10 40 63   93 97                   10 40 63   93 97 
         *       =  =  =    =  =                    =  =  =    =  =          
         */  
        ancestor->parent->right->balance = 'R';  
        ancestor->balance = '=';  
        btree_adjust_path(ancestor, inserted);  
    }  
}  

static int bintree_fixAfterInsertion(struct btree *tree, struct btree_node *ancestor, struct btree_node *inserted)
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

		struct btree_node *root = store_read(tree->store, tree->root->data);
		if (strcmp(inserted->key, root->key) < 0)
			root->balance = 'L';
		else
			root->balance = 'R';

		//再调用adjustPath方法调整新增节点的父节点到root的某子节点路径上的所有祖先节点的平衡因子  
//		adjustPath(root, inserted);  
	} 
	else if ((ancestor->balance == 'L' && strcmp(inserted->key, ancestor->key) > 0)  
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
//		adjustPath(ancestor, inserted);  
	} 
	else if (ancestor->balance == 'R' && strcmp(inserted->key, ancestor->right->key) > 0) {  
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
		btree_rotate_left(tree, ancestor);  
		//再调整ancestor->paraent（90）到新增节点路径上祖先节点平衡  
//		adjustPath(ancestor->parent, inserted);  
	} 
	else if (ancestor->balance == 'L' && strcmp(inserted->key, ancestor->left->key) > 0) { 
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
		btree_rotate_right(tree, ancestor);  
		//再调整ancestor->paraent（20）到新增节点路径上祖先节点平衡  
//		adjustPath(ancestor->parent, inserted);  
	} 
	else if (ancestor->balance == 'L' && strcmp(inserted->key, ancestor->left->key) > 0) { 
		/* 
		 * 情况5： 
		 * ancestor->balance的值为 L，并且被插入的Entry位于ancestor的左子树的右子树上。此 
		 * 种情况也会导致树不平衡，此种与第6种一样都需要执行两次旋转（执行一次绕ancestor的左子节点左 
		 * 旋，接着执行一次绕ancestor执行一次右旋）后，树才平衡，最后还需调用 左-右旋 专有方法进行平衡 
		 * 因子的调整 
		 */  
		btree_rotate_left(tree, ancestor->left);  
		btree_rotate_right(tree, ancestor);  
		//旋转后调用 左-右旋 专有方法进行平衡因子的调整  
//		adjustLeftRigth(ancestor, inserted);  
	} 
	else if (ancestor->balance == 'R' && strcmp(inserted->key, ancestor->right->key) < 0) { 

		/* 
		 * 情况6： 
		 * ancestor->balance的值为 R，并且被插入的Entry位于ancestor的右子树的 左子树上，此 
		 * 种情况也会导致树不平衡，此种与第5种一样都需要执行两次旋转（执行一次绕ancestor的右子节点右旋 
		 * ，接着执行一次绕ancestor执行一次左旋）后，树才平衡，最后还需调用 右-左旋 专有方法进行平衡因 
		 * 子的调整 
		 */  
		btree_rotate_right(tree, ancestor->right);  
		btree_rotate_left(tree, ancestor);  
		//旋转后调用 右-左旋 专有方法进行平衡因子的调整  
//		adjustRigthLeft(ancestor, inserted);  
	}  

	return 0;
}

static void avlbtree_insert(struct btree * tree, void *data, const char *key, int *add, int *update)
{
	struct btree_node *thiz = tree->root, *ancestor = NULL; 

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

		//记录不平衡的祖先节点  
		if (thiz->balance != '=') {  
			//如果哪个祖先节点不平衡，则记录，当循环完后，ancestor指向的就是最近一个  
			//不平衡的祖先节点  
			ancestor = thiz;  
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

				bintree_fixAfterInsertion(tree, ancestor, thiz->right);

				break;
			}
		}
	}
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

static int avlbtree_find(struct btree *tree, const char *key) 
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

static void avlbtree_close( struct btree * tree )
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

static void avlbtree_print(struct btree * tree, void (*print)(void *, void*), void *userdata)
{
	print_subtree(tree, tree->root, print, userdata);
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

static int avlbtree_isbalance(struct btree *tree)
{
	return bintree_isbalance(tree, tree->root) == 0;
}

