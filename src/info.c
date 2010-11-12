#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <values.h>
#include <string.h>
#include <time.h>
#include <sys/dir.h>

#include "info.h"


#define KEY (key[x++] - '0')
static inline void str2time(struct tm *t, const char *key)
{
	int x = 0;

	t->tm_year  = KEY * 1000;
	t->tm_year += KEY * 100;
	t->tm_year += KEY * 10;
	t->tm_year += KEY * 1;
	t->tm_year -= 1900;

	x++;
	t->tm_mon  = KEY * 10;
	t->tm_mon += KEY * 1;
	
	x++;
	t->tm_mday  = KEY * 10;
	t->tm_mday += KEY * 1;
	
	x++;
	t->tm_hour  = KEY * 10;
	t->tm_hour += KEY * 1;
	
	x++;
	t->tm_min   = KEY * 10;
	t->tm_min  += KEY * 1;
	
	x++;
	t->tm_sec   = KEY * 10;
	t->tm_sec  += KEY * 1;
}

static inline int str2userid(user_id *userid, const char *key)
{
	int x = 0, len;

	// 422302 1978 0227 033 8
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
#if PARSE_INFO
#if STR_KEY
	return strncmp(a->card, b->card, 17);
#else
	int i = 0, x;

	for (; i < 5; i++) {
		x = a->userid.key[i] - b->userid.key[i];
		if (x != 0)
			return (x < 0) ? -1: 1;
	}

	return EQ;
#endif
#endif
}

int userinfo_update(struct user_info *old, struct user_info *_new)
{
#if PARSE_INFO
	if (old->update.tm_year < _new->update.tm_year)
		goto update;

	if (old->update.tm_mon < _new->update.tm_mon)
		goto update;

	if (old->update.tm_mday < _new->update.tm_mday)
		goto update;

	if (old->update.tm_hour < _new->update.tm_hour)
		goto update;

	if (old->update.tm_min < _new->update.tm_min)
		goto update;

	if (old->update.tm_sec < _new->update.tm_sec)
		goto update;

	return -1;

update:
	fprintf(stderr, "%s -> %s\n", _new->name, old->name);
#else
	char *ap = strrchr(old->str, ',') + 1;
	char *bp = strrchr(_new->str, ',') + 1;

	if (strcmp(ap, bp) != 0)
		return -1;
#endif
	memcpy(old, _new, sizeof(struct user_info));

	return 0;
}



int userinfo_parser(struct user_info *info, char *info_str)
{
#if PARSE_INFO
	char *p, *key =  info_str;
	int i = 0;

	while (key) {
		if ((p = strchr(key, ',')))
			*p = 0;
		switch (i) {
			case 0:
#if STR_KEY
				strncpy(info->card, key, sizeof(info->card));
#else
				str2userid(&info->userid, key);
#endif
				break;
			case 1:
				strncpy(info->name, key, sizeof(info->name));
				break;
			case 2:
				info->sex = key[0];
				break;
			case 3:
				strncpy(info->email, key, sizeof(info->email));
				break;
			case 4:
				strncpy(info->mobile, key, sizeof(info->mobile));
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
#else
	strcpy(info->str, info_str);	
#endif
	return 0;
}

void userinfo_print(FILE *fp, struct user_info *i)
{
#if PARSE_INFO
#if STR_KEY
	fprintf(fp, "%s,"
			"%s,%s,%s,%s,"
			"%d-%0d-%0d %02d:%02d:%02d\n",
			i->card,
			i->name, i->sex == 'f' ? "female" : "male", i->email, i->mobile,
			i->update.tm_year + 1900, i->update.tm_mon, i->update.tm_mday, i->update.tm_hour, i->update.tm_min, i->update.tm_sec);
#else
	fprintf(fp, "%d%d%02d%02d%03d%c,"
			"%s,%s,%s,%s,"
			"%d-%0d-%0d %02d:%02d:%02d\n",
			i->userid.zipcode, i->userid.y + 1900, i->userid.m, i->userid.d, i->userid.order, i->userid.check,
			i->name, i->sex == 'f' ? "female" : "male", i->email, i->mobile,
			i->update.tm_year + 1900, i->update.tm_mon, i->update.tm_mday, i->update.tm_hour, i->update.tm_min, i->update.tm_sec);
#endif
#else
	printf("%s", i->str);
#endif
}

