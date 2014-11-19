#ifndef _REPMON_LOG_H
#define _REPMON_LOG_H

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	LOG_SERNAME_LEN	64
#define LOG_CONTENT_LEN	128
#define LOG_FILE_SIZE	32*1024*1024
#define LOG_BUF		32
#define LOG_ITEM_LEN	256

typedef enum log_type {
	LOG_TYPE_STDOUT,
	LOG_TYPE_SYSLOG,
	LOG_TYPE_FILE
} log_type_t;

typedef enum log_item_level {
	LOG_ITEM_OK,
	LOG_ITEM_WARN,
	LOG_ITEM_CRITICAL
} log_item_level_t;

static const char * log_item_level_str[] = {
	"OK",
	"WARNING",
	"CRITICAL"
};

/* The structure of the log data */
typedef struct log_item {
	char li_level_str[9];	/* log level string */
	char li_sername[LOG_SERNAME_LEN]; /* service name */
	char li_content[LOG_CONTENT_LEN]; /* log content */
	char li_date[11];	/* the format is mon/date/year */
	char li_time[9];	/* the format is hou/min/sec */
} log_item_t;

typedef struct log_entity {
	log_type_t 	le_type;
	int		le_fd;
	FILE		*le_fp;
	log_item_t	*le_actitem; /* store active item */
	int		le_actstart;
	int		le_actend;
	boolean_t	le_act;/* indicate if there is active item */
} log_entity_t;

extern int log_open(log_entity_t *, const char *);
extern int log_close(log_entity_t *);
extern int log_create_item(log_entity_t *, int, const char *, const char *);
extern int log_flush(log_entity_t *);

#ifdef  __cplusplus
}
#endif

#endif  /* _REPMON_LOG_H */
