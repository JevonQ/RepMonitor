/*
 * Header file fo network stat data
 * Author: Lei Xue
 * Version: 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <kstat.h>
#include <time.h>
#include <sys/time.h>

#define INTERVAL 1
#define LOOP_MAX 1
#define NIC_NAME_MAX 64
#define NIC_COUNT_MAX 256

/* nicdata - useful kstat NIC data */
typedef char nicname[NIC_NAME_MAX];
typedef struct nicdata {
    nicname name;           /* name of interface */
    uint64_t rbytes;        /* total read bytes */
    uint64_t wbytes;        /* total written bytes */
    uint64_t rpackets;      /* total read packets */
    uint64_t wpackets;      /* total written packets */
    uint64_t speed;         /* speed of interface */
    uint64_t sat;           /* saturation value */
    time_t time;            /* time of sample */
} nicdata;

int get_interface_name(nicname *interface_list);
/* get the BytesOut per second */
int get_bytesout(double bytesout[]);
/* get the OutErrors per second */
int get_outerrs(double outerrs[]);

