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

struct dt {
	unsigned int date;
	unsigned int time;
};

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
#if STR_KEY
	char key[19];
#else
	user_id userid;
#endif

	struct dt update;
	size_t seek;
#if 1
	char name[32];
	char email[32];
	char sex;
	char mobile[12];
#endif
};

inline int FAST_HASH(const char *P) ;

int  userinfo_parser (struct user_info *info, char *info_str, size_t seek);
int  userinfo_compare(const struct user_info *a, const struct user_info *b);
void userinfo_print  (FILE *fp, struct user_info *info);
int tm_compare(struct dt *t1, struct dt *t2);

#endif

