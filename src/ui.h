/*
 * =====================================================================================
 *
 *       Filename:  user_info.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月11日 14时18分51秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __UI_H__
#define __UI_H__

typedef struct algorithm {
	void *_private;
	void* (*init)(void);
	int   (*addfile)(void *info, const char *filename, int *add, int *update);
	void  (*out)(void *info, const char *file);
	void  (*end)(void *info);
	void  (*free)(void *info);
	int   (*find)(void *info, const char *key);
} ui;

int  ui_init   (ui *info);
void ui_free   (ui *info);
int  ui_addfile(ui *info, const char *filename, int *add, int *update);
void ui_out    (ui *info, const char *filename);
int  ui_find   (ui *info, const char *key);
void ui_end    (ui *info);

extern ui sbtree_ui;
extern ui avlbtree_ui;
extern ui bthread_ui;

#endif 

