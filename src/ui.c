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

int  ui_addfile(ui *info, const char *filename)
{
	return info->addfile(info->_private, filename);
}

void ui_out(ui *info, const char *filename)
{
	info->out(info->_private, filename);
}

void ui_free(ui *info)
{
	info->free(info->_private);
}

int  ui_find(ui *info, const char *key)
{
	return info->find(info->_private, key);
}
