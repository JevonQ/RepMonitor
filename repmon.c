#include <sys/stat.h>
#include <sys/types.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <priv.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <thread.h>
#include <signal.h>

#include "repmon.h"
#include "rconf.h"
#include "utils.h"
#include "rlog.h"

#define	PATH_CONFIG	"repmon.conf"
#define LOCK_FILE 	"repmon.lock"
#define REPMON_LOG	"repmon.log"
#define CLOCKID CLOCK_REALTIME 

repmonconf_t rc;	/* current configuration */
log_entity_t le;	/* log entity */
int terminated = 0;	/* used to terminate the repmon process */
int lockfd = 0;		/* file descriptor of lockf */

/* forward declarations */
void repmon_watchdog(repmonconf_t *, log_entity_t *);
void signal_handler(int);
void daemonize(repmonconf_t *, log_entity_t *);
void *terminating(void *);

void
repmon_watchdog(repmonconf_t *rcp, log_entity_t *lep)
{
	while (!terminated) {
		sleep(rcp->rc_interval);
	}
}

void
signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
		printf("repmon is stoped!"); 
		log_flush(&le);
		log_close(&le);
		rconf_close(&rc);
		if (lockfd) {
			lockf(lockfd, F_ULOCK, 0);
		}
		terminated = 1;
		break;
	}
}

void *
terminating(void *arg)
{
	while (!terminated) {
		signal(SIGTERM, signal_handler);
		sleep(10);
	}
}

void
daemonize(repmonconf_t *rcp, log_entity_t *lep)
{
	int i;
	pid_t pid, sid;
	thread_t tid;
	
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
	 * Make sure there is one daemon per server
	 */
	lockfd = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
	if (lockfd < 0) {
		warn(gettext("failed to open the lock file"));
		exit (EXIT_FAILURE);
	} else if (lockf(lockfd, F_TLOCK, 0) < 0) {
		warn(gettext("failed to lock the file"));
		exit (EXIT_FAILURE);
	}

	/*
	 * Close the standard file descriptors
	 */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	/*
	 * Create a new session id for this daemon
	 */
	sid = setsid();
	if (sid < 0) {
		warn(gettext("failed to create the process group!"));
		exit(EXIT_FAILURE);
	}

	/*
	 * Create a thread to catch the signal, and the thread
	 * will be terminated when signal SIGTERM is received.
	 */
	pthread_create(&tid, NULL, terminating, NULL);
	repmon_watchdog(rcp, lep);
}

int
main(int argc, char *argv[])
{       
        int ret;
        int funcRet;
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
	if (rconf_open(&rc, PATH_CONFIG) == -1) {
		warn(gettext("failed to open config file\n!"));
		return (E_ERROR);
	}

	if (log_open(&le, rc.rc_logdir)) {
		warn(gettext("failed to open log file\n!"));
		return (E_ERROR);
	}

	daemonize(&rc, &le);

        return (0);
}
