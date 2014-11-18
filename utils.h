#ifndef _REPMON_UTILS_H
#define _REPMON_UTILS_H

#include <libintl.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	E_SUCCESS	0	/* Exit status for success */
#define	E_ERROR		1	/* Exit status for error */
#define	E_USAGE		2	/* Exit status for usage error */

#define SH_WHICH	"which"	/* shell cmd 'which' */
#define SH_RM		"rm"	/* shell cmd 'rm' */
#define SH_PING		"ping"	/* shell cmd 'ping' */
#define SH_TMPFILE	"tmp.f"	/* tmp file to store the output */

extern void warn(const char *, ...);
extern void die(const char *, ...);
extern void get_systime_slash(char *, char *);

extern int valid_abspath(const char *);
extern int valid_ipaddr(const char*);
extern int exec_shell(const char*, const char*);
extern int valid_cmd(const char*);
extern int is_target_alive(const char*);

#ifdef	__cplusplus
}
#endif

#endif	/* _REPMON_UTILS_H */
