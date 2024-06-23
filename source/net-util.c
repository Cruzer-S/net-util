#include "net-util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <netdb.h>
#include <unistd.h>

#include <ifaddrs.h>

static thread_local char error_buffer[BUFSIZ];
static thread_local char *error;

const char * const net_util_error(void)
{
	return error;
}

struct list *get_interface_address(int family, int flags, int masks)
{
	struct ifaddrs *ifaddrs;
	struct address_data_node *node;
	struct list *head;

	size_t addrlen;

	head = malloc(sizeof(struct list));
	if (head == NULL) {
		error = "failed to malloc()";
		goto RETURN_NULL;
	}

	list_init_head(head);

	if (getifaddrs(&ifaddrs) != 0) {
		error = "failed to getifaddrs()";
		goto FREE_HEAD;
	}

	addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in)
		                      : sizeof(struct sockaddr_in6);

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
		if (node == NULL) {
			error = "failed to malloc()";
			goto FREE_NODE;
		}

		memcpy(&node->address, ifa->ifa_addr, addrlen);

		list_add(head, &node->list);
	}

	freeifaddrs(ifaddrs);

	return head;

FREE_NODE:	LIST_FOREACH_ENTRY_SAFE(head, ptr, 
				   	struct address_data_node, list)
			free(ptr);
FREE_IFADDRS:	freeifaddrs(ifaddrs);
FREE_HEAD:	free(head);
RETURN_NULL: 	return NULL;
};

void free_interface_address(struct list *head)
{
	LIST_FOREACH_ENTRY_SAFE(head, ptr, struct address_data_node, list)
		free(ptr);

	free(head);
}

char *get_host_from_address(struct sockaddr_storage *storage, int flags)
{
	thread_local static char address[NI_MAXHOST];

	size_t addrlen;

	if (storage->ss_family == AF_INET)
		addrlen = sizeof(struct sockaddr_in);
	else
		addrlen = sizeof(struct sockaddr_in6);

	int gai_ret = getnameinfo(
		(struct sockaddr *) storage, addrlen,
		address, NI_MAXHOST,
		NULL, 0, flags
	);

	if (gai_ret != 0) {
		sprintf(error_buffer, "failed to getnameinfo(): %s", 
	  		gai_strerror(gai_ret));
		error = error_buffer;
		return NULL;
	}

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
	if (gai_ret != 0) {
		sprintf(error_buffer, "failed to getaddrinfo(): %s",
	  		gai_strerror(gai_ret));
		error = error_buffer;
		goto RETURN_ERROR;
	}

	server_fd = socket(
		server_ai->ai_family,
		server_ai->ai_socktype,
		server_ai->ai_protocol
	);
	if (server_fd == -1) {
		error = "failed to socket()";
		goto FREEADDRINFO;
	}

	if (bind(server_fd, server_ai->ai_addr, server_ai->ai_addrlen) == -1) {
		error = "failed to bind()";
		goto CLOSE_SERVER;
	}

	if (listen(server_fd, backlog) == -1) {
		error = "failed to listen()";
		goto CLOSE_SERVER;
	}

	freeaddrinfo(server_ai);

	return server_fd;

CLOSE_SERVER:	close(server_fd);
FREEADDRINFO:	freeaddrinfo(server_ai);
RETURN_ERROR:	return -1;
}
