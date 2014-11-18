#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	time_t rawtime;
	struct tm *timeinfo;
	char ftime[64];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	ascftime(ftime, "%m/%d/%Y %H/%M/%S", timeinfo);
	printf(ftime);
	printf("\n");
	return (0);
}
