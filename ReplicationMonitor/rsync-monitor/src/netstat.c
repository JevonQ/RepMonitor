/*
 * Fetch and process the network stat data
 * Author: Lei Xue
 * Version: 0.1
 */

#include "netstat.h"

/*
 * Global variables
 */
static char *g_network[] = { "be", "bge", "ce", "ci", "dmfe", "e1000g", "el",
    "eri", "elxl", "fa", "ge", "hme", "ipge", "ipdptp", "iprb", "lane",
    "le", "nf", "ppp", "qe", "qfe", "rtls", "sppp", "vge", NULL };

/*
 * die - print stderr message and exit.
 *
 * This subroutine prints an error message and exits with a non-zero
 * exit status.
 */
static void
die(char *message, int status)
{
        (void) fprintf(stderr, "%s\n", message);
            exit(status);
}

/*
 * fetch64 - return a uint64_t value from kstat.
 *
 * The arguments are a kstat pointer, the value name,
 * and a default value in case the lookup fails.
 */
static uint64_t
fetch64(kstat_t *ksp, char *value64, uint64_t def)
{
	kstat_named_t *knp;	/* Kstat named pointer */

	/* try a lookup and return */
	if ((knp = kstat_data_lookup(ksp, value64)) != NULL)
		return (knp->value.ui64);
	return (def);
}

/*
 * fetch32 - return a uint32_t value from kstat.
 *
 * The arguments are a kstat pointer, the value name,
 * and a default value in case the lookup fails.
 */
static uint32_t
fetch32(kstat_t *ksp, char *value, uint32_t def)
{
	kstat_named_t *knp;	/* Kstat named pointer */

	/* try a lookup and return */
	if ((knp = kstat_data_lookup(ksp, value)) != NULL)
		return (knp->value.ui32);
	return (def);
}

/*
 * fetch6432 - return a uint64_t or a uint32_t value from kstat.
 *
 * The arguments are a kstat pointer, a potential ui64 value name,
 * a potential ui32 value name, and a default value in case both
 * lookup fails. The ui64 value is attempted first.
 */
static uint64_t
fetch6432(kstat_t *ksp, char *value64, char *value, uint64_t def)
{
	kstat_named_t *knp;	/* Kstat named pointer */

	/* try lookups and return */
	if ((knp = kstat_data_lookup(ksp, value64)) != NULL)
		return (knp->value.ui64);
	if ((knp = kstat_data_lookup(ksp, value)) != NULL)
		return (knp->value.ui32);
	return (def);
}

/*
 * fetch_nocanput - return nocanput value, whose name(s) are driver-dependent.
 *
 * Most drivers have a kstat "nocanput", but the ce driver
 * at least has "rx_nocanput" and "tx_nocanput"
 */
static uint32_t
fetch_nocanput(kstat_t *ksp, uint32_t def)
{
	kstat_named_t *knp;	/* Kstat named pointer */
	uint32_t sum;

	/* Check "nocanput" first */
	if ((knp = kstat_data_lookup(ksp, "nocanput")) != NULL) {
		return (knp->value.ui32);
	} else {
		if ((knp = kstat_data_lookup(ksp, "rx_nocanput")) != NULL) {
			sum = knp->value.ui32;
			if ((knp = kstat_data_lookup(ksp, "tx_nocanput"))
			    != NULL) {
				sum += knp->value.ui32;
				return (sum);
			}
		}
	}
	return (def);
}

/*
 * fetch_boot_time - return the boot time in secs.
 *
 * This takes a kstat control pointer and looks up the boot time
 * from unix:0:system_misc:boot:time. If found, this is returned,
 * else 0.
 */
static time_t
fetch_boot_time(kstat_ctl_t *kc)
{
	kstat_t *ksp;		/* Kstat struct pointer */
	kstat_named_t *knp;	/* Kstat named pointer */

	if ((ksp = kstat_lookup(kc, "unix", 0, "system_misc")) == NULL) {
		die("ERROR2: Can't read boot_time.\n", 2);
    }
	if ((kstat_read(kc, ksp, NULL) != -1) &&
	    ((knp = kstat_data_lookup(ksp, "boot_time")) != NULL)) {
		/* summary since boot */
		return (knp->value.ui32);
	} else {
		/* summary since, erm, epoch */
		return (0);
	}
}

/*
 * return the NIC data count
 */
static int
get_latest_data(kstat_ctl_t *kc, nicdata l_data[])
{
    kstat_t *ksp;		/* Kstat struct pointer */
	int ok, i;
	int num = 0;

	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
		/* Search all modules */
		for (ok = 0, i = 0; g_network[i] != NULL; i++) {
			if (strcmp(ksp->ks_module, g_network[i]) == 0)
				ok = 1;
		}

		/* Skip if this isn't a network module */
		if (ok == 0) continue;
		if (kstat_read(kc, ksp, NULL) == -1) continue;
		if ((kstat_data_lookup(ksp, "obytes") == NULL) &&
		    (kstat_data_lookup(ksp, "obytes64") == NULL)) continue;

		/* Save network values */
		l_data[num].rbytes = fetch6432(ksp, "rbytes64", "rbytes", 0);
		l_data[num].wbytes = fetch6432(ksp, "obytes64", "obytes", 0);
		l_data[num].rpackets = fetch6432(ksp, "ipackets64", "ipackets", 0);
		l_data[num].wpackets = fetch6432(ksp, "opackets64", "opackets", 0);
		l_data[num].sat = fetch32(ksp, "defer", 0);
		l_data[num].sat += fetch_nocanput(ksp, 0);
		l_data[num].sat += fetch32(ksp, "norcvbuf", 0);
		l_data[num].sat += fetch32(ksp, "noxmtbuf", 0);
		l_data[num].time = time(0);
		/* if the speed can't be fetched, this makes %util 0.0 */
		l_data[num].speed = fetch64(ksp, "ifspeed", 1LL << 48);
		(void) strcpy(l_data[num].name, ksp->ks_name);

		num++;
	}
	return (num);
}

/*
 * return:
 * 0~MAX_NIC_COUNT if successful
 * -1 if fail
 */
int get_interface_name(nicname *interface_list)
{
    kstat_ctl_t *kc;    /* Kstat controller */
    kstat_t *ksp;		/* Kstat struct pointer */
	int i;
	int num = 0;

    if ((kc = kstat_open()) == NULL) {
        die ("Can not open /dev/kstat!\n", -1);
    }

	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
		/* Search all modules */
		for (i = 0; g_network[i] != NULL; i++) {
			if (strcmp(ksp->ks_module, g_network[i]) == 0) {
				(void) strcpy(interface_list[num++], ksp->ks_name);
            }
		}
    }
    kstat_close();
    return (num);
}

/*
 * get the BytesOut per second
 */
int get_bytesout(double bytesout[])
{
    kstat_ctl_t *kc;
    kstat_t *ksp;
    int i;
    int nic_num = 0;
    nicdata new_data[MAX_NIC_COUNT];

    if ((kc = kstat_open()) == NULL) {
        die ("Can not open /dev/kstat!\n", -1)'
    }

    if ((nic_num = get_latest_data(kc, new_data)) < 0) {
        die ("Can not get the latest data!\n", -1);
    }

    for (i = 0; i < nic_num; i ++) {
        bytesout[i] = new_data[i].wbytes;
    }
    kstat_close();
    return 0;
}

/*
 * get the OutErrors per second
 */
int get_outerrs(double outerrs[])
{
    kstat_ctl_t *kc;
    kstat_t *ksp;
    int i;
    int nic_num = 0;
    nicdata new_data[MAX_NIC_COUNT];

    if ((kc = kstat_open()) == NULL) {
        die ("Can not open /dev/kstat!\n", -1)'
    }

    if ((nic_num = get_latest_data(kc, new_data)) < 0) {
        die ("Can not get the latest data!\n", -1);
    }

    for (i = 0; i < nic_num; i ++) {
        outerrs[i] = new_data[i].sat;
    }
    kstat_close();
    return 0;
}

