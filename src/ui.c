/*
 * =====================================================================================
 *
 *       Filename:  ui.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月11日 15时33分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "ui.h"


int ui_init(ui *info)
{
	info->_private = info->init();

	return 0;
}

int  ui_addfile(ui *info, const char *filename, int *add, int *update)
{
	if (info->addfile)
		return info->addfile(info->_private, filename, add, update);

	return -1;
}

void ui_out(ui *info, const char *filename)
{
	if (info->out)
		info->out(info->_private, filename);
}

void ui_free(ui *info)
{
	if (info->free)
		info->free(info->_private);
}

int  ui_find(ui *info, const char *key)
{
	if (info->find)
		return info->find(info->_private, key);
	
	return -1;
}

void ui_end(ui *info)
{
	if (info->end)
		info->end(info->_private);
}
