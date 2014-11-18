/* 
 * References:
 * 	usr/src/cmd/dumpadm/main.c
 * 	usr/src/cmd/iscsid/iscsid.c
 * 	usr/src/cmd/fcinfo/fcinfo.c
 */
#include <sys/stat.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <priv.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "repmon.h"
#include "rconf.h"
#include "utils.h"
#include "rlog.h"

/* forward declarations */
void repmon_watchdog(union sigval);
/*
 * main calls a parser that checks syntax of the input command against
 * various rules tables.
 *
 * The parser provides usage feedback based upon same tables by calling
 * two usage functions, usage and subUsage, handling command and subcommand
 * usage respectively.
 *
 * The parser handles all printing of usage syntactical errors
 *
 * When syntax is successfully validated, the parser calls the associated
 * function using the subcommands table functions.
 *
 * Syntax is as follows:
 *	command subcommand [options] resource-type [<object>]
 *
 * The return value from the function is placed in funcRet
*/

#define	PATH_CONFIG "repmon.conf"
#define CLOCKID CLOCK_REALTIME 

void repmon_watchdog(union sigval v)
{
	printf("timer expiration\n");
}


int
main(int argc, char *argv[])
{       
        int ret;
        int funcRet;
	repmonconf_t rc;	/* current configuration */
	int modified = 0;	/* have we modified the repmon config */
	timer_t timerid;
	struct sigevent evp;
	struct itimerspec it;
	struct timespec spec;

        (void) setlocale(LC_ALL, "");
#if     !defined(TEXT_DOMAIN)   /* Should be defined by cc -D */
#define TEXT_DOMAIN "SYS_TEST"  /* Use this only if it weren't */
#endif
        (void) textdomain(TEXT_DOMAIN);
/*
        if (!(priv_ineffect(PRIV_FILE_DAC_READ) &&
            priv_ineffect(PRIV_SYS_DEVICES))) {
                fprintf(stderr, gettext("insufficient privilege\n"));
                return (1);
        }
*/
	/*
	 * If no config file exists yet, we're going to create an empty
	 * one, so set the modified flag to force writing out the file.
	 */
	if (access(PATH_CONFIG, F_OK) == -1)
		modified++;

	/*
	 * Now open and read in the initial values from the config file.
	 * If it doesn't exist, the cmd will return with an error.
	 */
	if (rconf_open(&rc, PATH_CONFIG) == -1)
		return (E_ERROR);

	/*
	 * Create a timer to start watching over the replication services
	 */
	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = repmon_watchdog;

	if (timer_create(CLOCKID, &evp, &timerid) == -1) {
		warn(gettext("failed to creat the timer!"));
		return (E_ERROR);
	}

	clock_gettime(CLOCKID, &spec);
	it.it_interval.tv_sec = 1;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = spec.tv_sec + 2;
	it.it_value.tv_nsec = spec.tv_nsec + 0;

	if (timer_settime(timerid, TIMER_ABSTIME, &it, NULL) == -1) {
		warn(gettext("failed to set the timer"));
		return (E_ERROR);
	}

        return (0);
}
