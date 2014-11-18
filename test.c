#include <stdio.h>
#include <stdlib.h>

#include "rconf.h"
#include "utils.h"
#include "rlog.h"
/*
typedef struct repmonconf {
        char rc_logdir[MAXPATHLEN];     / * Saved log directory * /
        char rc_targetip[MAXPATHLEN];   / * Monitoring target IP * /
        char rc_targetcmd[CMDNAMELEN];  / * Monitoring target cmd * /
        long rc_interval;               / * Working interval * /
        FILE *rc_conf_fp;               / * File pointer for config file * /
        int rc_conf_fd;                 / * File descriptor for config file * /
        boolean_t rc_readonly;          / * Readonly config file * /
} repmonconf_t;a
*/

static const char PATH_CONFIG[] = "repmon.conf";

void
print_rc(repmonconf_t *rc)
{
	printf("rc_logdir=%s\n"
	    "rc_targetip=%s\n"
	    "rc_targetcmd=%s\n"
	    "rc_interval=%d\n",
	    rc->rc_logdir,
	    rc->rc_targetip,
	    rc->rc_targetcmd,
	    rc->rc_interval);
}

void
test_rconf()
{
	repmonconf_t rc;
	int rval = 0;

	rval = rconf_open(&rc, PATH_CONFIG);
	if (!rval) {
		print_rc(&rc);
		if (is_target_alive(rc.rc_targetip)) {
			printf("target %s is alive\n", rc.rc_targetip);
		} else {
			printf("target %s is dead\n", rc.rc_targetip);
		}
		rval = rconf_close(&rc);
	}
}

void
test_get_systime_slash()
{
	log_item_t li;

	get_systime_slash(li.li_date, li.li_time);
	printf("date=%s, time=%s\n", li.li_date, li.li_time);
}

void
test_rlog()
{
	log_entity_t le;
	repmonconf_t rc;
	int rval = 0;

	rval = rconf_open(&rc, PATH_CONFIG);
	rval = log_open(&le, rc.rc_logdir);
	rval = log_create_item(&le);
	rval = log_flush(&le);
	rval = log_close(&le);

	rconf_close(&rc);
}


int
main(int argc, char *argv[])
{
	test_rlog();
	return (0);
}
