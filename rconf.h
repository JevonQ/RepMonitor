#ifndef	_REPMON_RCONF_H
#define	_REPMON_RCONF_H

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	CMDNAMELEN 32

typedef struct repmonconf {
	char rc_logdir[MAXPATHLEN];	/* Saved log directory */
	char rc_targetip[MAXPATHLEN];	/* Monitoring target IP */
	char rc_targetcmd[CMDNAMELEN];	/* Monitoring target cmd */
	long rc_interval;		/* Working interval */
	FILE *rc_conf_fp;		/* File pointer for config file */
	int rc_conf_fd;			/* File descriptor for config file */
	boolean_t rc_readonly;		/* Readonly config file */
} repmonconf_t;

typedef struct rc_token {
	const char *tok_name;
	int (*tok_parse)(repmonconf_t *, char *);
	int (*tok_print)(const repmonconf_t *, FILE *);
} rc_token_t;

extern int rconf_open(repmonconf_t *, const char *);
extern int rconf_close(repmonconf_t *);
extern int rconf_update(repmonconf_t *);
extern void rconf_print(repmonconf_t *i, FILE *);

extern int rconf_str2dir(repmonconf_t *, char *);
extern int rconf_str2int(repmonconf_t *, char *);
extern int rconf_str2ip(repmonconf_t *, char *);
extern int rconf_str2cmd(repmonconf_t *, char *);

#ifdef __cplusplus
}
#endif

#endif /* _RCONF_H */
