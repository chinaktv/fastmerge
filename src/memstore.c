#include "store.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

struct memStore {
	void ** store;
	size_t blockSize;
	off_t blockCount;
	off_t newLastBlock;
	int calledFree;
	long total;
};

static struct memStore *ms_open(size_t blockSize, off_t blockCount);
static void   ms_close       (struct memStore * ms);
static void * ms_readBlock   (struct memStore * ms, off_t blockNumber);
static void   ms_writeBlock  (const struct memStore * mem, off_t blockNumber);
static off_t  ms_newBlock    (struct memStore * ms);
static void   ms_releaseBlock(const struct memStore * ms, off_t blockNumber);
static void   ms_freeBlock   (struct memStore * ms, off_t blockNumber);
static size_t ms_blockSize   (struct memStore * ms);

off_t ms_blockCount(struct memStore * ms);
void ms_print(struct memStore * ms);

const struct store_functions memory_functions = {
	.close     = (void   (*)(void *))         ms_close,
	.read      = (void * (*)(void *, off_t))  ms_readBlock,
	.write     = (void   (*)(void *, off_t))  ms_writeBlock,
	.new_store = (off_t  (*)(void *       ))  ms_newBlock,
	.release   = (void   (*)(void *, off_t))  ms_releaseBlock,
	.free      = (void   (*)(void *, off_t))  ms_freeBlock,
	.blockSize = (size_t (*)(void *))         ms_blockSize
};

static void   ms_grow_store     (struct memStore * ms, off_t newSize);
static off_t  ms_find_free_block(struct memStore * ms, off_t startBlock, off_t Stopblock);

struct store * store_open_memory(size_t blockSize, off_t blockCount) 
{
	struct store * store = malloc(sizeof(struct store));
	memset(store, 0, sizeof(struct store));
	store->functions = &memory_functions;
	store->store_p = ms_open(blockSize, blockCount);
	
	return store;
}

static struct memStore * ms_open(size_t blockSize, off_t blockCount) 
{
	struct memStore * ms = (struct memStore *) malloc(sizeof(struct memStore));

	memset(ms, 0, sizeof(struct memStore));

	ms->store = (void **)calloc(blockCount, sizeof(void **));
	ms->blockSize = blockSize;
	ms->blockCount = blockCount;
	ms->newLastBlock = 0;
	ms->calledFree = 0;
	ms->total = 0;

	return ms;
}

static void ms_close(struct memStore * ms) 
{
	off_t i;
	int free_count = 0;

	for(i = 0; i < ms->blockCount; i++) {
		if(ms->store[i] != 0) {
			ms_freeBlock(ms, i);
			free_count++;
		}
	}
	free(ms->store);
	free(ms);
}

static void *ms_readBlock(struct memStore * ms, off_t blockNumber) 
{
	ms->total++;
	assert ((blockNumber >= 0) && (blockNumber < ms->blockCount));
	return ms->store[blockNumber];
}

static void ms_releaseBlock(const struct memStore * ms, off_t blockNumber) 
{
	/* This function intentionally left blank. */
}

static void ms_writeBlock(const struct memStore * ms, off_t blockNumber) 
{
	/* This function intentionally left blank. */
}

static off_t ms_newBlock(struct memStore * ms) 
{
	off_t newBlock = -1;
	off_t current_block = ms->newLastBlock;
	off_t initial_block = ms->newLastBlock;

	newBlock = ms_find_free_block(ms, current_block, ms->blockCount);

	if(newBlock != -1) {
		ms->store[newBlock] = malloc(ms->blockSize);
		ms->newLastBlock = newBlock + 1;
		return newBlock;
	}

	if(ms->calledFree) {
		ms->calledFree = 0;
		current_block = 0;
		newBlock = ms_find_free_block(ms, current_block, initial_block+1);
		if(newBlock != -1) {
			ms->store[newBlock] = malloc(ms->blockSize);
			ms->newLastBlock = newBlock +1;
			return newBlock;
		}

		assert(0);
	}

	newBlock = ms->blockCount;
	ms->newLastBlock = newBlock+1;

	ms_grow_store(ms, ms->blockCount * 2);

	ms->store[newBlock] = malloc(ms->blockSize);

	return newBlock;
}

void ms_freeBlock(struct memStore * ms, off_t blockNumber) 
{
	void * block = ms_readBlock(ms, blockNumber);
	assert(block != 0);

	ms->store[blockNumber] = (void*)0; /* Remove soon-to-be dangling reference. */
	ms->calledFree = 1;

	free(block);
}

static off_t ms_find_free_block(struct memStore * ms, off_t startBlock, off_t stopBlock) 
{
	off_t i;

	if(startBlock < 0) startBlock = 0;
	if(stopBlock < 0) stopBlock = 0;
	if(stopBlock > ms->blockCount) stopBlock = ms->blockCount;
	if(startBlock > stopBlock) startBlock = stopBlock;
	for(i = startBlock; i < stopBlock; i++) {
		if(ms->store[i] == 0) {
			return i;
		}
	}
	return -1;
}

static void ms_grow_store(struct memStore * ms, off_t newCount) 
{
	void * oldStore = ms->store;

	assert(newCount > ms->blockCount);

	ms->store = calloc(newCount, ms->blockSize);
	ms->store = memcpy(ms->store, oldStore, ms->blockCount * sizeof(void**));
	ms->blockCount = newCount;
	free(oldStore);
}

static size_t ms_blockSize(struct memStore * ms) 
{
	return ms->blockSize;
}

off_t ms_blockCount(struct memStore * ms) 
{
	return ms->blockCount;
}

void ms_print(struct memStore * ms) 
{
	printf("Memory requests: %ld\n", ms->total);
}

void store_close(struct store * store) 
{
	(*(store->functions->close))(store->store_p);
	free(store);
}
