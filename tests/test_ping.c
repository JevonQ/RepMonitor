#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int
valid_ipaddr(const char* ip)
{
        struct in_addr addr;
        return (inet_pton(AF_INET, ip, &addr) || inet_pton(AF_INET6, ip, &addr));
}

int
main(int argc, char *argv[])
{
	char *ip = "10.113.197.2171";
	char shell_cmd[256];

	if (valid_ipaddr(ip)) {
		memset(shell_cmd, 0, strlen(shell_cmd));
		sprintf(shell_cmd,"%s %s", "ping", ip);
		system(shell_cmd);
	} else {
		printf("ipaddr is not valid\n");
	}
	return (0);
}
