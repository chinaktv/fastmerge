#include "store.h"

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <malloc.h>

#define ERR_MSG( fn ) { (void)fflush(stderr); \
	(void)fprintf(stderr, __FILE__ ":%d:" #fn ": %s\n", \
			__LINE__, strerror(errno)); }
#define METAPRINTF( fn, args, exp ) if( fn args exp ) ERR_MSG( fn )

#define PRINTF(args)        METAPRINTF(printf, args, < 0)
#define LSEEK(args)         METAPRINTF(lseek64, args, < 0)
#define CLOSE(args)         METAPRINTF(close, args, < 0)
#define READ_OR_DONT(args)  METAPRINTF(read, args, < 0)
#define WRITE_OR_DONT(args) METAPRINTF(write, args, < 0)
#define READ(args)          METAPRINTF(read, args, <= 0)
#define WRITE(args)         METAPRINTF(write, args, <= 0)

struct fileStore {
	int data_file;
	unsigned char *used_index;
	off64_t blockSize;
	off64_t blockCount;
	off64_t newLastBlock;
	int calledFree;
};


static void unpack_bits        (int *, const unsigned char);
static unsigned char pack_bits (const int *);
static void ds_mark_block      (struct fileStore *, off_t block, int mark);
static void ds_grow_files      (struct fileStore * fs, off_t newSize);
static off_t ds_find_free_block(struct fileStore * fs, off_t startBlock, off_t stopBlock);
static struct fileStore *ds_open(const char *filename, size_t blockSize, off_t blockCount);

static void  ds_close       (struct fileStore * fs);
static void *ds_readBlock   (const struct fileStore * fs, off_t blockNumber);
static void  ds_writeBlock  (struct fileStore * fs, off_t blockNumber, void *value);
static off_t ds_newBlock    (struct fileStore * fs);
static void  ds_releaseBlock(const struct fileStore * fs, off_t blockNumber, void * block) ;
static void  ds_freeBlock   (struct fileStore * fs, off_t blockNumber) ;
static size_t ds_blockSize  (const struct fileStore * fs);

const struct store_functions disk_functions = {
	(void   (*)(void *))         &ds_close,
	(void * (*)(void *, off_t))  &ds_readBlock,
	(void   (*)(void *, off_t))  &ds_writeBlock,
	(off_t  (*)(void *       ))  &ds_newBlock,
	(void   (*)(void *, off_t))  &ds_releaseBlock,
	(void   (*)(void *, off_t))  &ds_freeBlock,
	(size_t (*)(void *))         &ds_blockSize
};

struct store * store_open_disk(const char *filename, size_t blockSize, off_t blockCount) 
{
	struct store * store = malloc(sizeof(struct store));
	memset(store, 0, sizeof(struct store));
	store->functions = &disk_functions;
	store->store_p = ds_open(filename, blockSize, blockCount);
	
	return store;
}

static struct fileStore *ds_open(const char *filename, size_t blockSize, off_t blockCount) 
{
	struct fileStore* fs = (struct fileStore*) malloc(sizeof(struct fileStore));

	/*  fs->data_file = open(filename, O_RDWR|O_CREAT|O_LARGEFILE, S_IRWXU); */
	fs->data_file = open64(filename, O_RDWR|O_CREAT, S_IRWXU);
	fs->used_index = NULL;

	if(fs->data_file < 0) {
		perror("");
	}

	if(blockCount < 8) {
		blockCount = 8;
	}
	blockCount = (blockCount / 8) * 8;  /* Round blockCount down to the nearest multiple of 8. */

	fs->blockSize = blockSize;
	fs->newLastBlock = 0;
	fs->calledFree = 0;

	fs->blockCount = 0;
	ds_grow_files(fs, blockCount);

	assert(!(fs->blockCount % 8));
	return fs;
}

static void ds_close(struct fileStore * fs) 
{
	free(fs->used_index);
	CLOSE((fs->data_file));
	free(fs);
}

static void * ds_readBlock(const struct fileStore * fs, off_t blockNumber) 
{
	void * retVal;
	LSEEK((fs->data_file, blockNumber * fs->blockSize, SEEK_SET));
	retVal = malloc(fs->blockSize /* * sizeof(char)*/);
	//READ((fs->data_file, retVal, fs->blockSize));
	read(fs->data_file, retVal, fs->blockSize);
	return retVal;
} 

static void ds_releaseBlock(const struct fileStore * fs, off_t blockNumber, void * block) 
{
	free(block);
}

static void ds_writeBlock(struct fileStore * fs, off_t blockNumber, void * value) 
{
	LSEEK((fs->data_file, blockNumber * fs->blockSize, SEEK_SET));
	WRITE((fs->data_file, value, fs->blockSize));
}

static off_t ds_newBlock(struct fileStore * fs) 
{
	/* 8 blocks per byte. */
	off_t current_block = fs->newLastBlock; 
	off_t initial_block = fs->newLastBlock; 
	off_t newBlock = -1;

	newBlock = ds_find_free_block(fs, current_block, fs->blockCount);
	if(newBlock != -1) {
		ds_mark_block(fs, newBlock, 1);
		fs->newLastBlock = newBlock+1;
		return newBlock;
	}

	/* seek to beginning of file to reclaim freed space. */

	if(fs->calledFree) {
		fs->calledFree = 0;
		current_block = 0;

		newBlock = ds_find_free_block(fs, current_block, initial_block);
		if(newBlock != -1) {
			ds_mark_block(fs, newBlock, 1);
			fs->newLastBlock = newBlock+1;
			return newBlock;
		}
	}

	/* Nothing before the counter, so the file is 100% utilized. */

	/* Grow file by doubling its capacity. */

	newBlock = fs->blockCount;
	fs->newLastBlock = newBlock+1;
#ifndef GROW_BLOCK_COUNT
	ds_grow_files(fs, fs->blockCount * 2);
	fs->blockCount *= 2;
#else
	ds_grow_files(fs, fs->blockCount + GROW_BLOCK_COUNT);
	fs->blockCount += GROW_BLOCK_COUNT;
#endif
	ds_mark_block(fs, newBlock, 1);

	return newBlock;
} 

static void ds_mark_block(struct fileStore * fs, off_t blockNumber, int used) 
{
	unsigned char c;
	int bitSet[8];

	c = fs->used_index[blockNumber / 8];
	unpack_bits(bitSet, c);
	bitSet[blockNumber % 8] = used;
	c = pack_bits(bitSet);

	fs->used_index[blockNumber / 8] = c;
}

static void ds_freeBlock(struct fileStore * fs, off_t blockNumber) 
{
}

static off_t ds_find_free_block(struct fileStore * fs, off_t startBlock, off_t stopBlock) 
{
	off_t i;
	int j;
	unsigned char c;
	int bitSet[8];
	off_t start_offset, stop_offset;

	/** Expand boundaries so they are multiples of 8. **/
	startBlock = (startBlock / 8) * 8;
	stopBlock = (1+((stopBlock-1) / 8)) * 8;

	if(stopBlock > fs->blockCount) {
		stopBlock = fs->blockCount;
	}

	start_offset = startBlock / 8;
	stop_offset  = stopBlock / 8;

	c = fs->used_index[start_offset];
	for(i = start_offset; i < stop_offset; i++) {
		if(c != 0xFF) {
			unpack_bits(bitSet, c);
			for(j = 0; j < 8; j++) {
				if(! bitSet[j]) {
					return (i*8)+j;
				}
			}
		}
		start_offset++;
		c = fs->used_index[start_offset];
	}

	return -1;
}

static void ds_grow_files(struct fileStore * fs, off_t newCount) 
{
	void * oldStore = fs->used_index;

	assert(newCount > fs->blockCount);

	fs->used_index = calloc(newCount, fs->blockSize);
	fs->used_index = memcpy(fs->used_index, oldStore, fs->blockCount / 8 * sizeof(char));
	fs->blockCount = newCount;
	free(oldStore);
}

static size_t ds_blockSize(const struct fileStore * fs) 
{
	return fs->blockSize;
}

static void unpack_bits(int * bitSet, const unsigned char c) 
{
	int i;
	unsigned char c2 = c;
	for(i = 0; i < 8; i++) {
		bitSet[i] = c2 % 2;
		c2 /= 2;
	}
}

static unsigned char pack_bits(const int*  bitSet) 
{
	int i;
	unsigned char c = 0;
	/*Need to traverse the bitSet backwards.*/
	for(i = 7; i >= 0; i--) {
		c *= 2;
		c += bitSet[i]; /* Each entry of bitSet must be 0 or 1...*/
	}
	return c;
}


