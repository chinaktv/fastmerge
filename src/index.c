/*
 * =====================================================================================
 *
 *       Filename:  index.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月23日 16时18分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "index.h"

struct finfo {
	FILE *fp;
	size_t size;
};

struct index {
	struct finfo *info;
	int count;

	FILE *cur_fp;
	int cur_id;
	size_t seek;
};

struct index _index, *idx;

void index_init()
{
	idx = &_index;

	idx->count = 0;
	idx->info = NULL;
	idx->cur_fp = NULL;
	idx->seek = 0;
	idx->cur_fp = 0;
}

int index_add_file(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (fp) {
		idx->info = (struct finfo*)realloc(idx->info, (idx->count + 1) * sizeof(struct finfo));	
		idx->info[idx->count++].fp = fp;

		if (idx->cur_fp == NULL)
			idx->cur_fp = fp;

		printf("index add filename : %s\n", filename);
		return 0;
	}

	return -1;
}

size_t index_read_line(char *buffer, size_t size)
{
	size_t seek;

	seek = ftell(idx->cur_fp);

	if (!fgets(buffer, size, idx->cur_fp)) {
		if (idx->cur_fp == idx->info[idx->count - 1].fp)
			return -1;
		else {
			idx->seek += seek;
			idx->info[idx->cur_id++].size = idx->seek;
			idx->cur_fp = &idx->info[idx->cur_id].fp;

			index_read_line(buffer, size);
		}
	}

	return idx->seek + seek;
}

void index_free()
{
	int i;
	for (i=0; i<idx->count;i++)
		fclose(idx->info[i].fp);
}

char *index_read_line_pos(size_t pos, char *buffer, size_t size)
{
	int i;

	for (i=0; i < idx->count; i++) {
		if (pos >= idx->info[i].size ) {
			fseek(idx->info[i].fp, pos - idx->info[i].size, SEEK_SET);
			return fgets(buffer, size, idx->info[i].fp);
		}

	}

	return NULL;
}

