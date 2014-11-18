#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	FILE *fp;
	char buf[1024];
	int line = 0;
	
	if ((fp = fopen("repmon.conf", "r+")) == NULL)
		printf("failed to open stream for repmon.conf\n");

	for (line = 1; fgets(buf, 1024, fp) != NULL; line++) {
		printf("%s\n", buf);
	}

	return (0);
}
