#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	int rn = system("which rsyncaa > tmp.log");
	printf("rn=%d\n", rn);
	system("rm tmp.log");
	return (0);
}
