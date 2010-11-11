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

typedef struct algorithm {
	void *_private;
	void* (*init)(void);
	int   (*addfile)(void *info, const char *filename);
	void  (*out)(void *info, const char *file);
	void  (*free)(void *info);
} ui;

int  ui_init   (ui *info);
void ui_free   (ui *info);
int  ui_addfile(ui *info, const char *filename);
void ui_out    (ui *info, const char *filename);


extern ui btree_ui;
extern ui btree_thread_ui;
