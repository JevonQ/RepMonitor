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

/* global variables */
repmonconf_t rc;		/* configuration */
log_entity_t le;		/* log entity */
int terminated = 0;		/* used to terminate the repmon process */
int lockfd = 0;			/* file descriptor of lockf */
FILE *logfp = 0;		/* file descriptor of repmon log */
pthread_mutex_t cv_mutex;	/* mutex for condition variable */
pthread_cond_t	work_cv;	/* notify worker thread to work */
pthread_t	workerid;	/* worker thread id*/
pthread_t 	tid; 		/* watchdog thread id */

extern void check_service(log_entity_t *);
extern void check_ha(const char *);
extern void gather_stats(const char *);

/* forward declarations */
void repmon_watchdog(repmonconf_t *, log_entity_t *);
void signal_handler(int);
void daemonize(repmonconf_t *, log_entity_t *);
void *terminating(void *);
void repmon_writelog(char *);

void *
repmon_worker(void *argp)
{
	pthread_mutex_lock(&cv_mutex);
	while (!terminated) {
		pthread_cond_wait(&work_cv, &cv_mutex);
		/* do the job */
		repmon_writelog("gather_stats!");
		gather_stats(rc.rc_rsyncdir);
		pthread_mutex_unlock(&cv_mutex);
	}
	pthread_mutex_unlock(&cv_mutex);
	repmon_writelog("worker is stopped!");
	pthread_exit(NULL);
}

void
repmon_writelog(char *msg)
{
	char log[64];
	char date[11], time[9];

	memset(log, 0, sizeof(log));
	memset(date, 0, sizeof(date));
	memset(time, 0, sizeof(time));

	get_systime_slash(date, time);
	sprintf(log, "[%s %s]: %s\n", date, time, msg);
	fputs(log, logfp);
	fflush(logfp);
}

void
repmon_watchdog(repmonconf_t *rcp, log_entity_t *lep)
{
	/* create the worker thread to flush the statistics */
	pthread_create(&workerid, NULL, repmon_worker, NULL);

	while (!terminated) {
		pthread_mutex_lock(&cv_mutex);
		pthread_cond_signal(&work_cv);
		pthread_mutex_unlock(&cv_mutex);
		repmon_writelog("check_service!");
		check_service(&le);
		repmon_writelog("check_ha!");
		check_ha(rcp->rc_targetip);
		sleep(rcp->rc_interval);
	}
	
	pthread_exit(NULL);
}

void	
signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
		terminated = 1;
		pthread_mutex_destroy(&cv_mutex);
		pthread_cond_destroy(&work_cv);
		repmon_writelog("repmon is stoped!"); 
		fclose(logfp);
		log_flush(&le);
		log_close(&le);
		rconf_close(&rc);
		if (lockfd) {
			lockf(lockfd, F_ULOCK, 0);
		}
		break;
	}
}

/* 
 * Catch the signal SIGTERM
 */
void *
terminating(void *arg)
{
	while (!terminated) {
		signal(SIGTERM, signal_handler);
		sleep(1);
	}
}

/*
 * Enable repmon to run as a daemon
 */
void
daemonize(repmonconf_t *rcp, log_entity_t *lep)
{
	int i;
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
	 * Make sure there is one daemon per server
	 */
	lockfd = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
	if (lockfd < 0) {
		repmon_writelog(gettext("failed to open the lock file"));
		exit (EXIT_FAILURE);
	} else if (lockf(lockfd, F_TLOCK, 0) < 0) {
		repmon_writelog(gettext("failed to lock the file"));
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
		repmon_writelog(gettext("failed to create the process group!"));
		exit(EXIT_FAILURE);
	}

	repmon_writelog("repmon is started successfully!");

	pthread_mutex_init(&cv_mutex, NULL);
	pthread_cond_init(&work_cv, NULL);
	/*
	 * Create a thread to catch the signal, and the thread
	 * will be terminated when signal SIGTERM is received.
	 * There is no need to use pthread_join().
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
	if (access(PATH_CONFIG, F_OK) == -1) {
		warn(gettext("Please provide valid configuration"));
		return (E_ERROR);
	}

	/*
	 * Open the log file for repmon
	 */
	if ((logfp = fopen(REPMON_LOG, "a+")) == NULL) {
		warn(gettext("failed to open stream for %s"), REPMON_LOG);
		return (E_ERROR);
	}

	/*
	 * Now open and read in the initial values from the config file.
	 * If it doesn't exist, the cmd will return with an error.
	 */
	if (rconf_open(&rc, PATH_CONFIG) == -1) {
		repmon_writelog(gettext("failed to open config file\n!"));
		return (E_ERROR);
	}


	if (log_open(&le, rc.rc_logdir)) {
		repmon_writelog(gettext("failed to open log file\n!"));
		return (E_ERROR);
	}

	daemonize(&rc, &le);

        return (0);
}
