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

#define MAX_THREAD 4
#define MAX_STR 128
#define MAX_BUF 1024

struct bthread_info;

struct info_node {
	char str[MAX_STR];
};

struct mqueue {
	sem_t             get_sem;
	pthread_mutex_t   lock;
	struct info_node **node;
	int front, rear, count;
};

struct mqueue *mq_create(int size, int full)
{
	struct mqueue* mq;
	int i;

	mq = (struct mqueue *)malloc(sizeof(struct mqueue));
	assert(mq);

	memset(mq, 0, sizeof(struct mqueue));
	mq->count = size;
	mq->node = (struct info_node **)calloc(size, sizeof(struct info_node *));
	assert(mq->node);

	mq->front = mq->rear = 0;

	if (full) {
		for (i = 0; i < size; i++) {
			mq->node[i] = (struct info_node *)malloc(sizeof(struct info_node));
			assert(mq->node[i]);
		}
		mq->rear = size - 1;
		sem_init(&mq->get_sem, 0, size - 1);
	}
	else
		sem_init(&mq->get_sem, 0, 0);


	pthread_mutex_init(&mq->lock, NULL);

	return mq;
}

void mq_free(struct mqueue *mq)
{
	int i;

	for (i = 0; i < mq->count; i++) {
		if (mq->node[i])
			free(mq->node[i]);
	}
	
	sem_destroy(&mq->get_sem);
	pthread_mutex_destroy(&mq->lock);
	free(mq);
}

void mq_append(struct mqueue *mq, struct info_node *node, int lock)
{
	pthread_mutex_lock(&mq->lock);
	if ((mq->rear + 1) % mq->count == mq->front) {
	}

	mq->node[mq->rear] = node;
	mq->rear = (mq->rear + 1) % mq->count;

	pthread_mutex_unlock(&mq->lock);

	sem_post(&mq->get_sem);
}

struct info_node *mq_get(struct mqueue *mq, int lock)
{
	struct info_node *node = NULL;

	sem_wait(&mq->get_sem);

	pthread_mutex_lock(&mq->lock);

	if (mq->front != mq->rear) {
		node = mq->node[mq->front];
		mq->node[mq->front] = NULL;
		mq->front = (mq->front + 1) % mq->count;
	}
	pthread_mutex_unlock(&mq->lock);

	return node;
}

struct bthread_node {
	int id;
	int count;
	struct mqueue *info_queue;
	struct bthread_info *bi;
	int add, update;

	struct btree *tree;
	struct store *store;
	pthread_mutex_t mutex;
	pthread_t thread;
	int eof;
};

#define NODE_LOCK(node)   pthread_mutex_lock(&((node)->mutex))
#define NODE_UNLOCK(node) pthread_mutex_unlock((&(node)->mutex))

struct bthread_info {
	struct bthread_node node[MAX_THREAD];
	struct mqueue *free_queue;
};

static int userinfo_insert(struct btree *tree, char *info_str, int *add, int *update)
{
	struct user_info new_data;
	char key[20] = {0, }, *p;
	int len;

	if (tree == NULL || info_str == NULL)
		return -1;

	p = strchr(info_str, ',');

	len  =p - info_str;
	if (len > 18)
		len = 18;

	memcpy(key, info_str, p - info_str);

	memset(&new_data, 0, sizeof(struct user_info));
	userinfo_parser(&new_data, info_str);

	btree_insert(tree, &new_data, key, add, update);

	return 0;
}

void *insert_thread(struct bthread_node *thread_node)
{
	struct info_node *str_node;

	thread_node->add = thread_node->update = 1;

	while (!thread_node->eof) {
		str_node = mq_get(thread_node->info_queue, 1);

		if (str_node) {
			thread_node->count++;
			userinfo_insert(thread_node->tree, str_node->str, &thread_node->add, &thread_node->update);
			mq_append(thread_node->bi->free_queue, str_node, 1);
		}
		else  {
			break;
		}
	} 

//	printf("insert %d pthread exit\n", thread_node->count);
	pthread_exit(0);

	return NULL;
}

static struct bthread_info *btree_thread_ui_create(void)
{
	int i;
	struct bthread_info *bi = (struct bthread_info*)calloc(1, sizeof(struct bthread_info));

	assert(bi);

	bi->free_queue = mq_create(MAX_BUF, 1);

	for (i = 0; i < MAX_THREAD; i++) {
		bi->node[i].id    = i;
		bi->node[i].count = 0;
		bi->node[i].eof   = 0;
		bi->node[i].bi    = bi; 
		bi->node[i].info_queue = mq_create(MAX_BUF, 0);
		bi->node[i].store = store_open_memory(sizeof(struct user_info), 102400);
		bi->node[i].tree  = btree_new_memory(bi->node[i].store, \
						(int(*)(const void *, const void *))userinfo_compare, (int (*)(void*, void*))userinfo_update);

		pthread_mutex_init(&bi->node[i].mutex, NULL);

		pthread_create(&bi->node[i].thread, NULL, (void *(*)(void*))insert_thread, bi->node + i);
	}

	return bi;
}

int btree_thread_ui_addfile_fopen(struct bthread_info *bi, const char *filename, int *add, int *update)
{
	if (filename && bi) {
		FILE *fp;

		if ((fp  = fopen(filename, "r")) != NULL) {
			while (!feof(fp)) {
				struct info_node *node = mq_get(bi->free_queue, 0);

				if (fgets(node->str, sizeof(node->str) - 1, fp)) {
					int mon;

					if (strlen(node->str) < 8) {
						mq_append(bi->free_queue, node, 1);
						continue;
					}

					mon = FAST_HASH(node->str) % MAX_THREAD;
					mq_append(bi->node[mon].info_queue, node, 1);
				}
				else 
					mq_append(bi->free_queue, node, 1);
			}
			fclose(fp);

			return 0;
		}
	}

	return -1;
}

int btree_thread_ui_addfile_open(struct bthread_info *bi, const char *filename, int *add, int *update)
{
	if (filename && bi) {
		int fd, len, idx = 0;
#define BUFMAX 128
		char buffer[BUFMAX];

		fd = open(filename, O_RDONLY);
		if (fd < 0)
			return -1;

		while (1) {
			char *p;
			len = read(fd, buffer + idx, BUFMAX - idx);
			if (len <= 0)
				break;
			p = buffer;
			while (idx < len) {
				char *x = strchr(p, '\n');
				if (x) {
					*x = 0;
					int mon;
					struct info_node *node = mq_get(bi->free_queue, 0);

					strncpy(node->str, p, sizeof(node->str) - 1);

					if (strlen(node->str) < 8) {
						mq_append(bi->free_queue, node, 1);
						continue;
					}

					mon = FAST_HASH(node->str) % MAX_THREAD;
					mq_append(bi->node[mon].info_queue, node, 1);

					p = x + 1;
				}
				else {
					idx = BUFMAX - (p - buffer);
					memmove(buffer, p, idx);
					break;
				}
			}
		}
		close(fd);
	}

	return -1;
}

static void btree_thread_ui_out(struct bthread_info *ui, const char *filename)
{
	int i;
	FILE *out = stdout;

	if (filename) {
		fprintf(stderr, "output %s\n", filename);
		out = fopen(filename, "w+");
		if (out == NULL)
			out = stdout;
	}

	for (i = 0; i < MAX_THREAD; i++)
		btree_print(ui->node[i].tree, (void (*)(void*, void*))userinfo_print, out);

	if (out != stdout)
		fclose(out);
}

static void btree_thread_ui_end(struct bthread_info *ui)
{
	int i;
	void *value_ptr;

	for (i = 0; i < MAX_THREAD; i++) {
		ui->node[i].eof = 1;
		sem_post(&ui->node[i].info_queue->get_sem);
	}

	for (i = 0; i < MAX_THREAD; i++) {
		pthread_join(ui->node[i].thread, &value_ptr);
		mq_free(ui->node[i].info_queue);
	}
	mq_free(ui->free_queue);
}

static void btree_thread_ui_free(struct bthread_info *ui)
{
	int i;

	for (i = 0; i < MAX_THREAD; i++) {
		store_close(ui->node[i].store);
		btree_close(ui->node[i].tree);
	}

	free(ui);
}

ui bthread_ui = {
	.init    = (void *(*)(void))                           btree_thread_ui_create,
	.addfile = (int (*)(void*, const char *, int*, int*))  btree_thread_ui_addfile_fopen,
//	.addfile = (int (*)(void*, const char *, int*, int*))  btree_thread_ui_addfile_open,
	.out     = (void (*)(void*, const char *))             btree_thread_ui_out,
	.free    = (void (*)(void *))                          btree_thread_ui_free,
	.end     = (void (*)(void *))                          btree_thread_ui_end,
};

