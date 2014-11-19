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
//void repmon_watchdog(union sigval);
void daemonize(log_entity_t *, repmonconf_t *);


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

/*
void repmon_watchdog(union sigval v)
{
	printf("timer expiration\n");
}
*/

void
daemonize(log_entity_t *lep, repmonconf_t *rcp)
{
	pid_t pid, sid;
	
	if (getppid() == 1) {
		/* already run as a daemon just return */
		return;
	}

	/*
	 * Forking off the parent process
	 */
	pid = fork();
	if (pid < 0) {
		exit (EXIT_FAILURE);
	} else if (pid >0) {
		/* If we got a good PID, then exit the parent process */
		exit (EXIT_SUCCESS);
	}
	
	/*
	 * in order to write to any files created by the daemon, the file
	 * mode mask (umask) must be changed to ensure that they can be
	 * written to or read from properly.
	 */
	umask(0);

	/*
	 * Write a delimiter and a start message into the logfile
	 */
	log_create_item(lep, 0, NULL, "========================");
	log_create_item(lep, 0, "repmon", "Start successfully!"); 
	
	/*
	 * Create a new session id for this daemon
	 */
	sid = setsid();
	if (sid < 0) {
		warn(gettext("failed to create the process group!"));
		exit(EXIT_FAILURE);
	}

	/*
	 * Close the standard file descriptors
	 */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

int
main(int argc, char *argv[])
{       
        int ret;
        int funcRet;
	repmonconf_t rc;	/* current configuration */
	log_entity_t le;
	int modified = 0;	/* have we modified the repmon config */

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

	if (log_open(&le, rc.rc_logdir))
		return (E_ERROR);

	daemonize(&rc, &le);

        return (0);
}
