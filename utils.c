#include <sys/param.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "utils.h"

static const char PNAME_FMT[] = "%s: ";
static const char ERRNO_FMT[] = ": %s\n";

static const char *pname = "./";

/*PRINTFLIKE1*/
void
warn(const char *format, ...)
{
        int err = errno;
        va_list alist;

        if (pname != NULL)
                (void) fprintf(stderr, gettext(PNAME_FMT), pname);

        va_start(alist, format);
        (void) vfprintf(stderr, format, alist);
        va_end(alist);

        if (strchr(format, '\n') == NULL)
                (void) fprintf(stderr, gettext(ERRNO_FMT), strerror(err));
}

/*PRINTFLIKE1*/
void
die(const char *format, ...)
{
        int err = errno;
        va_list alist;

        if (pname != NULL)
                (void) fprintf(stderr, gettext(PNAME_FMT), pname);

        va_start(alist, format);
        (void) vfprintf(stderr, format, alist);
        va_end(alist);

        if (strchr(format, '\n') == NULL)
                (void) fprintf(stderr, gettext(ERRNO_FMT), strerror(err));

        exit(E_ERROR);
}

int
valid_abspath(const char *p)
{
	if (p == NULL)
		return (0);

	if (p[0] != '/') {
		warn(gettext("pathname is not an absolute path -- %s\n"), p);
		return (0);
	}

	if (strlen(p) > MAXPATHLEN) {
		warn(gettext("pathname is too long -- %s\n"), p);
		return (0);
	}

	return (1);
}

int
valid_ipaddr(const char *ip)
{
	struct in_addr addr;

	if (ip == NULL)
		return (0);
	
	return (inet_pton(AF_INET, ip, &addr) || inet_pton(AF_INET6, ip, &addr));
}

int
exec_shell1(const char *cmd, const char *arg)
{
	int ret = 0, rn = 0;
	char shell_cmd[64];

	if ((cmd == NULL) || (arg == NULL))
		return (-1);

	memset(shell_cmd, 0, strlen(shell_cmd));
	sprintf(shell_cmd, "%s %s > %s", cmd, arg, SH_TMPFILE);
	rn = system(shell_cmd);
	if (!rn) {
		ret = 1;
	}
	
	memset(shell_cmd, 0, strlen(shell_cmd));
	sprintf(shell_cmd, "%s %s", SH_RM, SH_TMPFILE);

	return (ret);
}

int
valid_cmd(const char *cmd)
{
	if (cmd == NULL)
		return (0);

	return (exec_shell1(SH_WHICH, cmd));
}

int
is_target_alive(const char *ip)
{
	if (ip == NULL)
		return (0);

	return (exec_shell1(SH_PING, ip));
}

/*
 * get_systime_slash return the system in the form of %H/%M/%S
 */
void
get_systime_slash(char *tdate, char *ttime)
{
	time_t rawtime;
	struct tm *timeinfo;

	if (tdate == NULL && ttime == NULL) {
		return;
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (tdate) {
		ascftime(tdate, "%m/%d/%Y", timeinfo);
	}
	if (ttime) {
		ascftime(ttime, "%H/%M/%S", timeinfo);
	}
}
