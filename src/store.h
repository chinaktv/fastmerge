#ifndef SYS_BULK_STORAGE_H
#define SYS_BULK_STORAGE_H 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <stdio.h>
#include "memwatch.h"

struct store {
	const struct store_functions * functions;
	void * store_p;
};

struct store_functions {
	void   (*close)(void*);
	void * (*read)(void*, off_t);
	void   (*write)(void*, off_t, void*);
	off_t  (*new_store)(void*);
	void   (*release)(void*, off_t, void*);
	void   (*free)(void*, off_t);
	size_t (*blockSize)(void*);
};

#define store_read(stor, index)           (*(stor->functions->read))     ((stor)->store_p, index)
#define store_write(stor, index, block)   (*(stor->functions->write))    ((stor)->store_p, index, block)
#define store_new(stor)                   (*(stor->functions->new_store))((stor)->store_p)
#define store_release(stor, index, block) (*(stor->functions->release))  ((stor)->store_p, index, block)
#define store_free(stor, index)           (*(stor->functions->free))     ((stor)->store_p, index)
#define store_blockSize(stor)             (*(stor->functions->blockSize))((stor)->store_p)

void  store_close    (struct store *stor);
off_t store_new_write(struct store *stor, void *data);

struct store *store_open_memory(size_t blockSize, off_t blockCount);
struct store *store_open_disk(const char *filename, size_t blockSize, off_t blockCount);

#endif

