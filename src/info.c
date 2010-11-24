#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>

#include "info.h"

#define KEY (key[x++] - '0')
static inline void str2time(struct dt *t, const char *key)
{
	int x = 0;

	int year, mon, mday, hour, min, sec;

	year  = KEY * 1000;
	year += KEY * 100;
	year += KEY * 10;
	year += KEY * 1;
	year -= 1900;

	x++;
	mon  = KEY * 10;
	mon += KEY * 1;
	
	x++;
	mday  = KEY * 10;
	mday += KEY * 1;
	
	x++;
	hour  = KEY * 10;
	hour += KEY * 1;
	
	x++;
	min   = KEY * 10;
	min  += KEY * 1;
	
	x++;
	sec   = KEY * 10;
	sec  += KEY * 1;

	t->date = (year << 16) + (mon << 8) + mday;
	t->time = (hour << 16) + (min << 8) + sec;
}

static inline int str2userid(user_id *userid, const char *key)
{
	int x = 0, len;

	len = strlen(key);

	if (len < 15  || len > 18)
		return -1;

	userid->zipcode  = KEY * 100000;
	userid->zipcode += KEY * 10000;
	userid->zipcode += KEY * 1000;
	userid->zipcode += KEY * 100;
	userid->zipcode += KEY * 10;
	userid->zipcode += KEY * 1;

	if (len == 15) {
		userid->y = 1900;
	}
	else {
		userid->y = KEY * 1000;
		userid->y += KEY * 100;
	}

	userid->y += KEY * 10;
	userid->y += KEY;

	userid->m  = KEY * 10;
	userid->m += KEY;

	userid->d  = KEY * 10;
	userid->d += KEY * 1;

	userid->order  = KEY * 100;
	userid->order += KEY * 10;
	userid->order += KEY * 1;

	userid->check = key[17];

	return 0;
}

int userinfo_compare(const struct user_info *a, const struct user_info *b) 
{
#if STR_KEY
	return strncmp(a->key, b->key, 17);
#else
	int i = 0, x;

	for (; i < 5; i++) {
		x = a->userid.key[i] - b->userid.key[i];
		if (x != 0)
			return (x < 0) ? -1: 1;
	}

	return EQ;
#endif
	return 0;
}

int tm_compare(struct dt *t1, struct dt *t2)
{
	if (t1->date == t2->date)
		return t1->time - t2->time;
	else
		return t1->date - t2->time;
}

int userinfo_parser(struct user_info *info, char *info_str, size_t seek)
{
	char *p, *key =  info_str;
	int i = 0;

	info->seek = seek;
	while (key) {
		if ((p = strchr(key, ',')))
			*p = 0;
		switch (i) {
			case 0:
#if STR_KEY
				strncpy(info->key, key, sizeof(info->key));
#else
				str2userid(&info->userid, key);
#endif
				break;
			case 1:
//				strncpy(info->name, key, sizeof(info->name));
				break;
			case 2:
//				info->sex = key[0];
				break;
			case 3:
//				strncpy(info->email, key, sizeof(info->email));
				break;
			case 4:
//				strncpy(info->mobile, key, sizeof(info->mobile));
				break;
		
			case 5: 
#if 0
				sscanf(key, "%d-%d-%d %d:%d:%d", &info->update.tm_year, \
						&info->update.tm_mon, \
						&info->update.tm_mday,\
						&info->update.tm_hour,\
						&info->update.tm_min,\
						&info->update.tm_sec);
				info->update.tm_year -= 1900;
#endif

				str2time(&info->update, key);
				break;
		}

		if (p)
			key = p + 1;
		else
			break;
		i++;
	}

#if 0
	printf("%d-%d-%d-%d-%d-%c, %s,%s,%s, %s", 
			info->userid.zipcode, info->userid.y, info->userid.m, info->userid.d, info->userid.order, info->userid.check,
			info->name, info->email, info->mobile, asctime(&info->update));
#endif
	return 0;
}

void userinfo_print(FILE *fp, struct user_info *i)
{
#if STR_KEY
	char buf[128];

	char *p = index_read_line_pos(i->seek, buf, 128 -1);
	if (p)
		fprintf(fp, "%s", p);
#if 0	
	fprintf(fp, "%s,"
			"%d-%02d-%0d %02d:%02d:%02d\n",
			i->key,
			(i->update.date >> 16) + 1900, 
			(i->update.date >>  8) & 0xFF,
			(i->update.date & 0x00FF) ,
			(i->update.time >> 16),
			(i->update.time >>  8) & 0xFF, 
			(i->update.time & 0x00FF));
#endif
#if 0
	fprintf(fp, "%s,"
			"%s,%s,%s,%s,"
			"%d-%0d-%0d %02d:%02d:%02d\n",
			i->key,
			i->name, i->sex == 'f' ? "female" : "male", i->email, i->mobile,
			i->update.tm_year + 1900, i->update.tm_mon, i->update.tm_mday, i->update.tm_hour, i->update.tm_min, i->update.tm_sec);
#endif
#else
	fprintf(fp, "%d%d%02d%02d%03d%c,"
			"%s,%s,%s,%s,"
			"%d-%0d-%0d %02d:%02d:%02d\n",
			i->userid.zipcode, i->userid.y + 1900, i->userid.m, i->userid.d, i->userid.order, i->userid.check,
			i->name, i->sex == 'f' ? "female" : "male", i->email, i->mobile,
			i->update.tm_year + 1900, i->update.tm_mon, i->update.tm_mday, i->update.tm_hour, i->update.tm_min, i->update.tm_sec);
#endif
}

inline int FAST_HASH(const char *P) 
{
	u_int32_t __h;
	u_int8_t *__cp, *__hp;
	__hp = (u_int8_t *)&__h;
	__cp = (u_int8_t *)(P);
	__hp[3] = __cp[0] ^ __cp[4];
	__hp[2] = __cp[1] ^ __cp[5];
	__hp[1] = __cp[2] ^ __cp[6];
	__hp[0] = __cp[3] ^ __cp[7];

	return __h;                   
}

