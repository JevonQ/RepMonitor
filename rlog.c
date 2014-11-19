#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>
#include <malloc.h>

#include "rlog.h"
#include "utils.h"

#define TEST_LI_SERNAME	"Service: auto-tier-svcA"
#define	TEST_LI_CONTENT	"running"

int
log_open(log_entity_t *le, const char *lpath)
{
	const char * fpmode = "at+";

	if ((le->le_fp = fopen(lpath, fpmode)) == NULL) {
		warn(gettext("failed to open stream for %s"), lpath);
		return (-1);
	}

	le->le_actitem = (log_item_t *)calloc(sizeof(log_item_t), LOG_BUF);
	le->le_actstart = le->le_actend = -1;
	le->le_act = B_FALSE;

	return (0);
}

int
log_close(log_entity_t *le)
{
	if (fclose(le->le_fp) == 0) {
		(void) close(le->le_fd);
		return (0);
	}

	if (le->le_actitem) {
		free(le->le_actitem);
	}
	return (-1);
}

int
log_create_item(log_entity_t *le, int level, const char *sername,
    const char *content)
{
	if ((level < 0) || (level > 2)) {
		warn(gettext("failed to create a log item"
		" as log level %d is invalid"), lpath);
	}

	le->le_actend = (++le->le_actend) % LOG_BUF;
	log_item_t *li = &le->le_actitem[le->le_actend];

#ifdef REP_DEBUG
	strcpy(li->li_level_str, log_item_level_str[0]);
	strcpy(li->li_sername, TEST_LI_SERNAME);
	strcpy(li->li_content, TEST_LI_CONTENT);
#endif 
	strcpy(li->li_level_str, log_item_level_str[level]);
	if (sername != NULL) {
		strcpy(li->li_sername, sername);
	}
	if (content != NULL) {
		strcpy(li->li_content, content);
	}	

	get_systime_slash(li->li_date, li->li_time);

	le->le_act = B_TRUE;
}

int
log_flush(log_entity_t *le)
{
	int i, tmpi;
	FILE *fp = le->le_fp;
	log_item_t *li;
	char logitem[LOG_ITEM_LEN];

	if (le->le_act) {
		i = le->le_actstart;
		/* handle the overflow case */
		if (le->le_actend < le->le_actstart) {
			tmpi = i - LOG_BUF;
		} else {
			tmpi = i;
		}

		i = (++i) % LOG_BUF;
		tmpi++;
		for (; tmpi <= le->le_actend; tmpi++, i = ((++i) % LOG_BUF)) {
			memset(logitem, 0, sizeof(logitem));
			li = &le->le_actitem[i];
			sprintf(logitem, "%s %s %s %s %s\n", li->li_level_str,
			    li->li_sername, li->li_content, li->li_date, li->li_time);
			fputs(logitem, fp);
		}
	}

	le->le_actstart = le->le_actend;
	le->le_act = B_FALSE;

	return (0);
}
