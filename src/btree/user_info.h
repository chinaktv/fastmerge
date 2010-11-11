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
	int  (*init)(void);
	int  (*addfile)(const char *filename);
	void (*out)(const char *file);
	void (*free)(void);
} ui;

int  userinfo_init   (ui *info);
int  userinfo_addfile(ui *info, const char *filename);
void userinfo_out    (ui *info, const char *filename);
void userinfo_free   (ui *info);
