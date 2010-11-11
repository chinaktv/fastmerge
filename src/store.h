#ifndef SYS_BULK_STORAGE_H
#define SYS_BULK_STORAGE_H 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include <stdio.h>
#include "memwatch.h"

struct store {
	const struct store_functions * functions;
	void * store_p;
};

struct store_functions {
	void   (*close)(void*);
	void * (*read)(void*, off_t);
	void   (*write)(void*, off_t);
	off_t  (*new_store)(void*);
	void   (*release)(void*, off_t);
	void   (*free)(void*, off_t);
	size_t (*blockSize)(void*);
};

#define store_read(stor, block)    (*(stor->functions->read))     (stor->store_p, block)
#define store_write(stor, block)   (*(stor->functions->write))    (stor->store_p, block)
#define store_new(stor)            (*(stor->functions->new_store))(stor->store_p)
#define store_release(stor, block) (*(stor->functions->release))  (stor->store_p, block)
#define store_free(stor, block)    (*(stor->functions->free))     (stor->store_p, block)
#define store_blockSize(stor)      (*(stor->functions->blockSize))(stor->store_p)
struct store *store_open_memory(size_t blockSize, off_t blockCount);
void store_close(struct store * store);

#endif

