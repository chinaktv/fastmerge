/*
 * =====================================================================================
 *
 *       Filename:  store.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月12日 11时55分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "store.h"

void store_close(struct store *stor)  
{
	stor->functions->close(stor->store_p); 
	free(stor);
}

size_t store_new_write(struct store *stor, void *data)
{
	size_t idx = store_new(stor);                     
	void *data_ptr = store_read(stor, idx);              
	memcpy(data_ptr, data, store_blockSize(stor)); 
	store_write(stor, idx, data_ptr);              
	store_release(stor, idx, data_ptr);

	return idx;
}
