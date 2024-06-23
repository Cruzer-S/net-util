#include "net-util.h"

#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <netdb.h>
#include <unistd.h>

#include <ifaddrs.h>

struct list get_interface_address(int family, int flags, int masks)
{
	struct ifaddrs *ifaddrs;
	struct address_data_node *node;
	struct list head = LIST_HEAD_INIT(head);

	if (getifaddrs(&ifaddrs) != 0) 
		goto RETURN_ERR;

	for (struct ifaddrs *ifa = ifaddrs; ifa; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family != family)
			continue;

		if ( !(ifa->ifa_flags & flags) )
			continue;

		if (ifa->ifa_flags & masks)
			continue;
		
		node = malloc(sizeof(struct address_data_node));
		if (node == NULL)
			goto FREE_NODE;

		memcpy(&node->address, &ifa->ifa_addr,
	 	       sizeof(struct sockaddr_storage));

		list_add(&head, &node->list);
	}

	freeifaddrs(ifaddrs);

	return head;

FREE_NODE:	LIST_FOREACH_ENTRY_SAFE(&head, ptr, 
				   	struct address_data_node, list)
			free(ptr);
FREE_IFADDRS:	freeifaddrs(ifaddrs);
RETURN_ERR: 	return (struct list){ NULL, NULL };
};

char *get_host_from_address(struct sockaddr_storage *storage, int flags)
{
	thread_local static char address[NI_MAXHOST];

	size_t addrlen;

	if (storage->ss_family == AF_INET)
		addrlen = sizeof(struct sockaddr_in);
	else
		addrlen = sizeof(struct sockaddr_in6);

	int gai_ret = getnameinfo(
		(struct sockaddr *) &storage, addrlen,
		address, NI_MAXHOST,
		NULL, 0, flags
	);

	if (gai_ret != 0)
		return NULL;

	return address;
}

int server_create(char *hostname, char *service, int backlog)
{
	struct addrinfo addr_req, *server_ai;

	int server_fd;
	int gai_ret;
	
	memset(&addr_req, 0x00, sizeof addr_req);
	addr_req.ai_family = AF_INET6;
	addr_req.ai_socktype = SOCK_STREAM;
	addr_req.ai_flags =  AI_ALL;

	gai_ret = getaddrinfo(hostname, service, &addr_req, &server_ai);
	if (gai_ret == -1)
		goto RETURN_ERROR;

	server_fd = socket(
		server_ai->ai_family,
		server_ai->ai_socktype,
		server_ai->ai_protocol
	);

	if (server_fd == -1)
		goto FREEADDRINFO;

	if (bind(server_fd, server_ai->ai_addr, server_ai->ai_addrlen) == -1)
		goto CLOSE_SERVER;

	if (listen(server_fd, backlog) == -1)
		goto CLOSE_SERVER;

	freeaddrinfo(server_ai);

	return server_fd;

CLOSE_SERVER:	close(server_fd);
FREEADDRINFO:	freeaddrinfo(server_ai);
RETURN_ERROR:	return -1;
}
