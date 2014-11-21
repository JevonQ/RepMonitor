#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libintl.h>

#include "rconf.h"
#include "utils.h"

/*
 * Permission and ownership for the configuration file
 */
#define RC_OWNER	0	/* Uid 0 (root) */
#define RC_GROUP	1	/* Gid 1 (other) */
#define RC_PERM	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)	/* Mode 0644 */

static int print_logdir(const repmonconf_t *, FILE *);
static int print_interval(const repmonconf_t *, FILE *);
static int print_targetip(const repmonconf_t *, FILE *);
static int print_cmd(const repmonconf_t *, FILE *);

static const rc_token_t tokens[] = {
	{ "REPMON_LOGDIR", rconf_str2dir, print_logdir },
	{ "REPMON_INTERVAL", rconf_str2int, print_interval },
	{ "REPMON_TARGETIP", rconf_str2ip, print_targetip },
	{ "REPMON_TARGETOBJ", rconf_str2cmd, print_cmd },
	{ NULL, NULL, NULL }
};

/*
 * repmon provides two forms of output, human readable and machine parseable
 * (-p option). In both of them the output consists of multiple lines in the
 * form of a name-value pair. The following structure is used to store the
 * strings used for the name component in each of these two forms fo output
 * respectively
 */
typedef struct rc_print_mode_str {
	const char *parseable_str;
	const char *human_readable_str;
} rc_print_mode_str_t;

static const rc_print_mode_str_t rc_print_mode_strs[] = {
	{ "REPMON_LOGDIR", "Log directory" },
	{ "REPMON_INTERVAL", "Working interval" },
	{ "REPMON_TARGETIP", "IP address of the target" },
	{ "REPMON_TARGETOBJ", "Target command to be monitored" },
	{ NULL, NULL}
};

int
rconf_open(repmonconf_t *rcp, const char *cpath)
{
	char buf[BUFSIZ];
	int line;
	int toknum = 0;
	const char *fpmode = "r+";
	
	if((rcp->rc_conf_fp = fopen(cpath, fpmode)) == NULL) {
		warn(gettext("failed to open stream for %s"), cpath);
		return (-1);
	}
	
	for (line = 1; fgets(buf, BUFSIZ, rcp->rc_conf_fp) != NULL; line++) {
		char name[BUFSIZ], value[BUFSIZ];
		const rc_token_t *tokp;
		int len;

		if (buf[0] == '#' || buf[0] == '\n'){
			continue;
		}

		/*
		 * Look for "name=value", with optional whitespace on either
		 * side, terminated by a newline, and consuming the whole line.
		 */
		if (sscanf(buf, " %[^=]=%s \n%n", name, value, &len) == 2 &&
		    name[0] != '\0' && value[0] != '\0' &&
		    len == (int)(strlen(buf))) {
			/*
			 * Locate a matching token in the tokens[] table,
			 * and invoke its parsing function.
			 */
			for (tokp = tokens, toknum = 0; tokp->tok_name != NULL; tokp++, toknum++) {
				if (strcmp(name, tokp->tok_name) == 0) {
					if (tokp->tok_parse(rcp, value) == -1) {
						warn(gettext("\"%s\", line %d: "
						    "warning: invalid %s\n"),
						    cpath, line, name);
					}
					switch (toknum) {
						case 0:
							strcpy(rcp->rc_logdir, value);
							break;
						case 1:
							rcp->rc_interval = atoi(value);
							break;
						case 2:
							strcpy(rcp->rc_targetip, value);
							break;
						case 3:
							strcpy(rcp->rc_targetcmd, value);
							break;

						default:
							break;
					}

					break;
				}
			}

			/*
			 * If we hit the end of the tokens[] table,
			 * no matching token was found.
			 */
			if (tokp->tok_name == NULL) {
				warn(gettext("\"%s\", line %d: warning: "
				    "invalid token: %s\n"), cpath, line, name);
			}

		} else {
			warn(gettext("\"%s\", line %d: syntax error\n"),
			    cpath, line);
		}
	}

	return (0);
}


int
rconf_close(repmonconf_t *rcp)
{
	if (fclose(rcp->rc_conf_fp) == 0) {
		return (0);
	}
	return (-1);
}

/* Only absolute path is supported */
int
rconf_str2dir(repmonconf_t *rcp, char *buf)
{
	if (valid_abspath(buf)){
		(void) strcpy(rcp->rc_logdir, buf);
		return (0);
	}

	return (-1);
}

int
rconf_str2int(repmonconf_t *rcp, char *buf)
{
	rcp->rc_interval = strtol(buf, (char **)NULL, 10);
	return (0);
}

int
rconf_str2ip(repmonconf_t *rcp, char *buf)
{
	if (valid_ipaddr(buf)) {
		(void) strcpy(rcp->rc_targetip, buf);
		return (0);
	}

	return (-1);
}

int
rconf_str2cmd(repmonconf_t *rcp, char *buf)
{
	if (valid_cmd(buf)) {
		(void) strcpy(rcp->rc_targetcmd, buf);
		return (0);
	}

	return (0);
}

static int
print_logdir(const repmonconf_t *rcp, FILE *fp)
{
	return (0);
}

static int
print_interval(const repmonconf_t *rcp, FILE *fp)
{
	return (0);
}

static int
print_targetip(const repmonconf_t *rcp, FILE *fp)
{
	return (0);
}

static int
print_cmd(const repmonconf_t *rcp, FILE *fp)
{
	return (0);
}
