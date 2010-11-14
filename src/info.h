/*
 * =====================================================================================
 *
 *       Filename:  info.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月11日 16时13分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef INFO_H
#define INFO_H

#include <time.h>

#define STR_KEY 1
#define PARSE_INFO 0

typedef union {
	struct {
		int  zipcode;
		int  y, m, d;
		int  order;
		char check;
	};

	int key[5];
}user_id;

struct user_info {
#if PARSE_INFO
#if STR_KEY
	char card[20];
#else
	user_id userid;
#endif

	char name[32];
	char email[32];
	char sex;
	char mobile[12];
	struct tm update;
#else
	char str[128];
#endif
};

inline int FAST_HASH(const char *P) ;

int  userinfo_parser (struct user_info *info, char *info_str);
int  userinfo_update (struct user_info *old, struct user_info *_new);
int  userinfo_compare(const struct user_info *a, const struct user_info *b);
void userinfo_print  (FILE *fp, struct user_info *info);

#endif

