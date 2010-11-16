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
	return 0;
}

int userinfo_update(struct user_info *old, struct user_info *_new)
{
	struct tm old_t, new_t;
#if PARSE_INFO
	old_t = old->update;
	new_t = _new->update;
#else
	char *ap = strrchr(old->str, ',') + 1;
	char *bp = strrchr(_new->str, ',') + 1;

	str2time(&old_t, ap);
	str2time(&new_t, bp);
#endif
	if (old_t.tm_year < new_t.tm_year)
		goto update;

	if (old_t.tm_mon < new_t.tm_mon)
		goto update;

	if (old_t.tm_mday < new_t.tm_mday)
		goto update;

	if (old_t.tm_hour < new_t.tm_hour)
		goto update;

	if (old_t.tm_min < new_t.tm_min)
		goto update;

	if (old_t.tm_sec < new_t.tm_sec)
		goto update;

	return -1;

update:
#if 0
#if PARSE_INFO
	printf("%s(%s) -> %s(%s)\n", _new->card, _new->name, old->card, old->name);
#else	
	printf("%s -> %s\n", _new->str, old->str);
#endif
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
	fprintf(fp, "%s", i->str);
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

