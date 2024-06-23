#include <stdio.h>

#include "net-util.h"

#include <net/if.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
	struct list *address = get_interface_address(
		AF_INET6, IFF_UP, IFF_LOOPBACK
	);

	if (LIST_IS_EMPTY(address))
		return 1;

	printf("Hosts: ");
	LIST_FOREACH_ENTRY(address, addr, struct address_data_node, list) {
		char *host = get_host_from_address(
			&addr->address, NI_NUMERICHOST
		);

		if (host == NULL) continue;

		printf("%s\t", host);
	} putchar('\n');

	free_interface_address(address);

	return 0;
}
