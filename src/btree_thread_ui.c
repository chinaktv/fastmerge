#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>
#include <pthread.h>
#include <semaphore.h>

#include "btree.h"
#include "store.h"
#include "ui.h"
#include "info.h"
#include "list.h"

#define MAX_THREAD 4
#define MAX_STR 128
#define MAX_BUF (MAX_THREAD * 1) 

struct bthread_info;

struct info_node {
	struct list_head head;
	char str[MAX_STR];
};

struct mqueue {
	struct list_head  msg_list;
	sem_t             send_sem, get_sem;
	pthread_mutex_t   lock;
	int               working;
	int               size;
};

static struct mqueue *mqueue_create(int max_size)
{
	struct mqueue* queue = NULL;

	queue = (struct mqueue *)malloc(sizeof(struct mqueue));
	if (queue == NULL) {
		return NULL;
	}
	memset(queue, 0, sizeof(struct mqueue));

	queue->size = max_size;
	sem_init(&queue->get_sem, 0, 0);
	sem_init(&queue->send_sem, 0, max_size);
	pthread_mutex_init(&queue->lock, NULL);

	queue->working = 1;
	INIT_LIST_HEAD(&queue->msg_list);

	return queue;
}

static void mqueue_close(struct mqueue *queue)
{
	int32_t count, i;

	queue->working = 0;

	sem_getvalue(&queue->send_sem, &count);
	for (i=0; i < queue->size - count; i++)
		sem_post(&queue->send_sem);

	/*make the msg scheduler keep running*/
	sem_post(&queue->get_sem);
	sem_destroy(&queue->get_sem);
	sem_destroy(&queue->send_sem);
	pthread_mutex_destroy(&queue->lock);
	free(queue);
}

#define QUEUE_LOCK(queue)    pthread_mutex_lock(&(queue)->lock)
#define QUEUE_UNLOCK(queue)  pthread_mutex_unlock(&(queue)->lock)

static int mqueue_send(struct mqueue *queue, struct info_node *node)
{
	sem_wait(&queue->send_sem);

	if (!queue->working)
		return -1;

	QUEUE_LOCK(queue);
	list_add(&node->head, &queue->msg_list);
	QUEUE_UNLOCK(queue);
	sem_post(&queue->get_sem);

	return 0;
}

static struct info_node *mqueue_get(struct mqueue *queue)
{
	struct info_node* node = NULL;
	struct list_head* pos;
	struct list_head* n;

	sem_wait(&queue->get_sem);

	if (!queue->working)
		return NULL;

	QUEUE_LOCK(queue);
	list_for_each_safe( pos, n, &queue->msg_list ) {
		node = list_entry(pos, struct info_node, head);
		list_del(pos);
		break;
	}
	QUEUE_UNLOCK(queue);

	sem_post(&queue->send_sem);

	return node;
}

struct bthread_node {
	int id;
	struct list_head info_str_head;
	struct mqueue *queue;
	struct bthread_info *bi;

	struct btree *tree;
	struct store *store;
	pthread_mutex_t mutex;
	pthread_t thread;
	int eof;
};

struct bthread_info {
	struct bthread_node node[MAX_THREAD];
	struct mqueue *free_queue;
};


struct info_node *node_list_get(struct list_head *head, int ok)
{
	struct info_node *sta = NULL;

	if (!list_empty(head)) {
		printf("ok\n");
		struct list_head *pos, *n;
		list_for_each_safe(pos, n, head) {
			sta = list_entry(pos, struct info_node, head);
			list_del(pos);
			break;
		}
	}
	else if (ok) {
		static int count = 0;
		sta = (struct info_node *)malloc(sizeof(struct info_node));
		count++;
		printf("count=%d\n", count);
	}

	return sta;
}

void node_list_free(struct list_head *head, struct info_node *node)
{
	list_add(&node->head, head);
}

static int userinfo_insert(struct btree *tree, char *info_str)
{
	off_t new;
	struct store *oStore;
	struct user_info *new_data;

	if (tree == NULL || info_str == NULL)
		return -1;

	oStore = tree->oStore;

	new = store_new(oStore);
	new_data = store_read(oStore, new);

	if (new_data == NULL)
		return -1;

	memset(new_data, 0, sizeof(struct user_info));
	userinfo_parser(new_data, info_str);
	store_write(oStore, new);
	store_release(oStore, new);

	btree_insert(tree, new);

	return 0;
}

void *insert_thread(struct bthread_node *thread_node)
{
	struct info_node *str_node;
	do {
		str_node = mqueue_get(thread_node->queue);

		if (str_node) {
			userinfo_insert(thread_node->tree, str_node->str);
			mqueue_send(thread_node->bi->free_queue, str_node);
		}
	} while (!thread_node->eof); 

	pthread_exit(0);

	return NULL;
}

static struct bthread_info *btree_ui_create(void)
{
	int i;
	struct bthread_info *bi = (struct bthread_info*)malloc(sizeof(struct bthread_info));

	bi->free_queue = mqueue_create(MAX_BUF);
	for (i = 0; i < MAX_BUF; i++) {
		struct info_node *sta = (struct info_node *)malloc(sizeof(struct info_node));
		mqueue_send(bi->free_queue, sta);
	}

	for (i = 0; i < MAX_THREAD; i++) {
		bi->node[i].eof   = 0;
		bi->node[i].bi    = bi;
		bi->node[i].queue = mqueue_create(MAX_BUF / MAX_THREAD);
		bi->node[i].store = store_open_memory(sizeof(struct user_info), 1000);
		bi->node[i].tree  = btree_new_memory(bi->node[i].store, \
						(int(*)(const void *, const void *))userinfo_compare, (int (*)(void*, void*))userinfo_update);
		INIT_LIST_HEAD(&bi->node[i].info_str_head);

		pthread_create(&bi->node[i].thread, NULL, (void *(*)(void*))insert_thread, bi->node + i);
	}

	return bi;
}

static int btree_ui_addfile(struct bthread_info *bi, const char *filename)
{
	if (filename && bi) {
		FILE *fp;
		if ((fp  = fopen(filename, "r")) != NULL) {
			while (!feof(fp)) {
				struct info_node *node = mqueue_get(bi->free_queue);

				if (fgets(node->str, 512, fp)) {
					int mon;

					if (strlen(node->str) < 20)
						continue;
					// 422302197802270338
					mon = (node->str[10] -'0') * 10 + (node->str[11] -'0');
					mon = mon % MAX_THREAD;
					mqueue_send(bi->node[mon].queue, node);
				}
			}
			fclose(fp);

			return 0;
		}
	}

	return -1;
}

static void btree_ui_out(struct bthread_info *ui, const char *filename)
{
	int i;
	FILE *out = stdout;

	if (filename) {
		printf("output %s\n", filename);
		out = fopen(filename, "w+");
		if (out == NULL)
			out = stdout;
	}

	for (i = 0; i < MAX_THREAD; i++)
		btree_print(ui->node[i].tree, (void (*)(void*, void*))userinfo_print, out);

	if (out != stdout)
		fclose(out);
}

static void btree_ui_free(struct bthread_info *ui)
{
	int i;
	void *value_ptr;

	for (i = 0; i < MAX_THREAD; i++) {
		ui->node[i].eof = 1;
	}

	for (i = 0; i < MAX_THREAD; i++) {
		pthread_join(ui->node[i].thread, &value_ptr);
		store_close(ui->node[i].store);
		btree_close(ui->node[i].tree);
	}

	free(ui);
}

ui bthread_ui = {
	.init    = (void *(*)(void))btree_ui_create,
	.addfile = (int (*)(void*, const char *))btree_ui_addfile,
	.out     = (void (*)(void*, const char *))btree_ui_out,
	.free    = (void (*)(void *))btree_ui_free
};

