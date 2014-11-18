/*
 * This file includes many functions which monitor the rsync service
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum f
{
    F_MINUTE = 0,
    F_HOUR,
    F_DAY,
    F_MONTH,
    F_WEEK,
} f_t;
typedef struct frequency
{
    f_t f;
    int mon;
    int week;
    int day;
    int hour;
    int min;
} frequency_t;

typedef enum stat_s
{
    STAT_IDLE = 0,
    STAT_RUNNING,
} stat_t;

typedef enum status_s
{
    STATUS_ONLINE = 0,
    STATUS_OFFLINE,
} status_t;

typedef struct rservice_s
{
    char instance_name[50];
    status_t status;
    stat_t stat;
    time_t started_time;
    frequency_t freq;
    int enabled;
    struct tm last_time; 
} rservice_t;

static int
get_num_by_mon(char *mon_str)
{
    int ret = 0;
    char mon[12][3] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for(ret = 0; ret < 12; ret ++) {
        if (strncmp(mon_str, mon[ret], 3) == 0) {
            return (ret + 1);
        }
    }
}

static int
get_num_by_day(char *day_str)
{
    int ret = 0;
    char day[7][3] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for(ret = 0; ret < 7; ret ++) {
        if (strncmp(day_str, day[ret], 3) == 0) {
            return (ret + 1);
        }
    }
}
static int
get_frequency_by_content(char *line, frequency_t *f)
{
    int ret = 0;
    char *tmp_p;
    char day[3];
    memset(f, 0, sizeof (frequency_t));

    /* match the month rsync job */
    if ((tmp_p = strstr(line, "day of the month at")) != NULL) {
        /* find the cron job is monthly */
        f->f = F_MONTH;
        if ((tmp_p = strstr(line, "every")) != NULL) {
            sscanf(line, ": every %d months on %dsth day of the month at %2d:%2d", &f->mon, &f->day, &f->hour, &f->min);
        } else {
            sscanf(line, ": monthly on %dsth day of the month at %2d:%2d", &f->day, &f->hour, &f->min);
            f->mon = 1;
        }
    } else if ((tmp_p = strstr(line, "weekly on")) != NULL) {
        /* find the cron job is weekly */
        f->f = F_DAY;
        sscanf(line, ": weekly on %3s at %2d:%2d", day, &f->hour, &f->min);
        f->day = get_num_by_day(day) + 7;
        f->week = 1;
    } else if ((tmp_p = strstr(line, "weeks on")) != NULL) {
        f->f = F_DAY;
        sscanf(line, ": every %d weeks on %3s at %2d:%2d", &f->week, day, &f->hour, &f->min);
        f->day = get_num_by_day(day) + 7 * f->week;
    } else if ((tmp_p = strstr(line, "daily at")) != NULL) {
        /* find the cron job is daily */
        f->f = F_DAY;
        sscanf(line, ": daily at %2d:%2d", &f->hour, &f->min);
        f->day = 1;
    } else if ((tmp_p = strstr(line, "days at")) != NULL) {
        f->f = F_DAY;
        sscanf(line, ": every %d days at %2d:%2d", &f->day, &f->hour, &f->min);
    } else if ((tmp_p = strstr(line, "hourly at start of the hour")) != NULL) {
        /* find the cron job is hourly */
        f->f = F_HOUR;
        f->hour = 1;
    } else if ((tmp_p = strstr(line, "hours at start of the hour")) != NULL) {
        f->f = F_HOUR;
        sscanf(line, ": every %d hours at start of the hour", &f->hour);
    } else if ((tmp_p = strstr(line, "hours on")) != NULL) {
        f->f = F_HOUR;
        sscanf(line, ": every %d hours on %d%2s minute of the hour", &f->hour, &f->min, day);
    } else if ((tmp_p = strstr(line, "minutes")) != NULL) {
        f->f = F_MINUTE;
        sscanf(line, ": every %d minutes", &f->min);
    } else {
        /* unknown cron job type */
        ret = 1;
    }
    return (ret);
}

static int
get_rservice_list(rservice_t *service_list)
{
    FILE *cmdFD;
    char cmd[20] = "cat /root/rs.txt";
    char line[1024];
    char *temp;
    char temp_name[20];
    char month[3];
    int i = 0, ret = 0;

    memset(line, '\0', sizeof (line));
    cmdFD = popen(cmd, "r");
    while (fgets(line, 1024, cmdFD) != NULL) {
        if ((temp = strstr(line, "instance")) != NULL) {
            temp = strstr(line, ":");
            sscanf(temp, ": %s", service_list->instance_name);
            continue;
        } else if ((temp = strstr(line, "status")) != NULL) {
            temp = strstr(line, ":");
            memset(temp_name, '\0', sizeof (temp_name));
            sscanf(temp, ": %s", temp_name);
            if (strcmp(temp_name, "online") == 0) {
                service_list->status = STATUS_ONLINE;
            } else {
                service_list->status = STATUS_OFFLINE;
            }
            continue;
        } else if ((temp = strstr(line, "frequency")) != NULL) {
            temp = strstr(line, ": ");
            if ((ret = get_frequency_by_content(temp, &service_list->freq)) != 0)
            {
                printf("Error: ...");
                return  -ret;
            }
            continue;
        } else if ((temp = strstr(line, "stat")) != NULL) {
            temp = strstr(line, ":");
            memset(temp_name, '\0', sizeof (temp_name));
            sscanf(temp, ": %s", temp_name);
            if (strcmp(temp_name, "idle") == 0) {
                service_list->stat = STAT_IDLE;
            } else {
                service_list->stat = STAT_RUNNING;
            }
            continue;
        } else if ((temp = strstr(line, "enabled")) != NULL) {
            temp = strstr(line, ":");
            memset(temp_name, '\0', sizeof (temp_name));
            sscanf(temp, ": %s", temp_name);
            if (strcmp(temp_name, "true") == 0) {
                service_list->stat = 1;
            } else {
                service_list->stat = 0;
            }
            continue;
        } else if ((temp = strstr(line, "time_started")) != NULL) {
            temp = strstr(line, ":");
            sscanf(temp, ": %2d:%2d:%2d,%s %2d", &service_list->last_time.tm_hour, &service_list->last_time.tm_min, &service_list->last_time.tm_sec, month, &service_list->last_time.tm_mday);
            service_list->last_time.tm_mon = get_num_by_mon(month);
            continue;
        }
    }

    pclose(cmdFD);
    return 0;
}

static void
log_err(int level, char *msg)
{
    if (level == 0) {
        printf("Error: %s\n", msg);
    } else if (level == 1) {
        printf("WARNING: %s\n", msg);
    } else {
        printf("NOTE: %s\n", msg);
    }
}

/*
 * rservice_t properties:
 *  char instance_name[50];
 *  status_t status;
 *  stat_t stat;
 *  time_t started_time;
 *  frequency_t freq;
 *  int enabled;
 *  struct tm last_time; 
 */
int
get_service_status(rservice_t *rs)
{
    char msg[64];
    memset(msg, '\0', sizeof (msg));
    if (rs->status == STATUS_ONLINE) {
       (void) snprintf(msg, sizeof (msg), "Auto-tier(%s) service is online\n", rs->instance_name);
       log_err(0, msg); // Error
    } else {
       (void) snprintf(msg, sizeof (msg), "Auto-tier(%s) service is offline\n", rs->instance_name);
       log_err(2, msg); // Note
    }

    return (0);
}

static int
get_worked_time(struct tm *last_time, frequency_t *freq, struct tm* worktime)
{
    int days_of_mon[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (last_time->tm_min + freq->min >= 60) {
        worktime->tm_min = last_time->tm_min + freq->min - 60;
        worktime->tm_hour += 1;
    } else {
        worktime->tm_min = last_time->tm_min + freq->min;
    }
    if (last_time->tm_hour + freq->hour + worktime->tm_hour >= 24) {
        worktime->tm_hour = last_time->tm_hour + freq->hour + worktime->tm_hour - 24;
        worktime->tm_mday += 1;
    } else {
        worktime->tm_hour = last_time->tm_hour + freq->hour + worktime->tm_hour;
    }

    if (worktime->tm_mday + freq->day + last_time->tm_mday > days_of_mon[(last_time->tm_mon + freq->mon)%12]) {
        worktime->tm_mday = worktime->tm_mday + freq->day + last_time-> tm_mday - days_of_mon[(last_time->tm_mon + freq->mon) % 12 -1];
        worktime->tm_mon += 1;
    } else {
        worktime->tm_mday = worktime->tm_mday + last_time->tm_mday + freq->day;
    }

    if (worktime->tm_mon + freq->mon + last_time->tm_mon > 12) {
        worktime->tm_mon = worktime->tm_mon + freq->mon + last_time->tm_mon - 12;
        worktime->tm_year = 1;
    } else {
        worktime->tm_mon = worktime->tm_mon + freq->mon + last_time->tm_mon;
    }
}

/*
 * Check whether the replication service start at the scheduled time
 */
int
check_service_cron_job(rservice_t *rs)
{
    char msg[64];
    struct tm worktime;
    struct tm *current_time;
    time_t nowtime;

    memset(&worktime, 0, sizeof (worktime));
    time(&nowtime);
    current_time = localtime(&nowtime);
    current_time->tm_mon += 1;

    memset(msg, '\0', sizeof (msg));
    get_worked_time(&rs->last_time, &rs->freq, &worktime);
    /*
    printf("worktime.tm_mon = %d, current_time->tm_mon = %d, last_time->tm_mon = %d\n", worktime.tm_mon, current_time->tm_mon, rs->last_time.tm_mon);
    printf("worktime.tm_mday = %d, current_time->tm_mday = %d, last_time->tm_mday = %d\n", worktime.tm_mday, current_time->tm_mday, rs->last_time.tm_mday);
    printf("worktime.tm_hour = %d, current_time->tm_hour = %d, last_time->tm_hour = %d\n", worktime.tm_hour, current_time->tm_hour, rs->last_time.tm_hour);
    printf("worktime.tm_min = %d, current_time->tm_min = %d, last_time->tm_min = %d\n", worktime.tm_min, current_time->tm_min, rs->last_time.tm_min);
    printf("worktime.tm_sec = %d, current_time->tm_sec = %d, last_time->tm_sec = %d\n", worktime.tm_sec , current_time->tm_sec, rs->last_time.tm_sec);
    */
    if (worktime.tm_mon < current_time->tm_mon) {
        if (worktime.tm_year == 0) {
            (void) sprintf(msg, "Auto-tier(%s) did not start at the scheduled time 1", rs->instance_name);
            log_err(0, msg);
        }
    } else if (worktime.tm_mon > current_time->tm_mon) {
        // PASS
    } else {
        if (worktime.tm_mday < current_time->tm_mday) {
            (void) sprintf(msg, "Auto-tier(%s) did not start at the scheduled time 2", rs->instance_name);
            log_err(0, msg);
        } else if (worktime.tm_mday == current_time->tm_mday) {
            if (worktime.tm_hour < current_time->tm_hour) {
                (void) sprintf(msg, "Auto-tier(%s) did not start at the scheduled time 3", rs->instance_name);
                log_err(0, msg);
            } else if (worktime.tm_hour == current_time->tm_hour) {
                if (worktime.tm_min < current_time->tm_min) {
                    (void) sprintf(msg, "Auto-tier(%s) did not start at the scheduled time 4", rs->instance_name);
                    log_err(0, msg);
                } else if (worktime.tm_min == current_time->tm_min) {
                    if (worktime.tm_sec < current_time->tm_sec) {
                        (void) sprintf(msg, "Auto-tier(%s) did not start at the scheduled time 5", rs->instance_name);
                        log_err(0, msg);
                    } else {
                        // PASS
                    }
                } else {
                    // PASS
                }
            } else {
                //PASS
            }
        } else {
            // PASS
        }
    }
    return (0);
}

int
check_running_time(rservice_t *rs)
{

}

int
main()
{
    rservice_t rs;
    get_rservice_list(&rs);
    get_service_status(&rs);
    check_service_cron_job(&rs);
}
