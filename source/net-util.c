#include "net-util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include <netdb.h>
#include <unistd.h>

#include <ifaddrs.h>

#include <sys/fcntl.h>

#include <sys/epoll.h>

char *get_epoll_event_name(uint32_t events)
{
	static uint32_t event_list[] = {
		EPOLLIN, EPOLLOUT, EPOLLRDHUP,
		EPOLLPRI, EPOLLERR, EPOLLHUP,
	};
	static char *event_name[] = {
		"EPOLLIN", "EPOLLOUT", "EPOLLRDHUP",
		"EPOLLPRI", "EPOLLERR", "EPOLLHUP"
	};

	thread_local static char buffer[1024];

	buffer[0] = '\0';

	for (int i = 0; i < sizeof(event_list) / sizeof(uint32_t); i++)
	{
		if ((events & event_list[i]) > 0)
		{
			events ^= event_list[i];
			strcpy(buffer, event_name[i]);
		}
	}

	return buffer;
}

int reuse_address(int fd)
{
	int sockopt = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
		       &sockopt, sizeof(sockopt)) == -1)
		return -1;

	return 0;
}

int fcntl_set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags == -1)
		return -1;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
		return -1;

	return 0;
}

struct list *get_interface_address(int family, int flags, int masks)
{
	struct ifaddrs *ifaddrs;
	struct address_data_node *node;
	struct list *head;

	size_t addrlen;

	head = malloc(sizeof(struct list));
	if (head == NULL)
		goto RETURN_NULL;

	list_init_head(head);

	if (getifaddrs(&ifaddrs) != 0)
		goto FREE_HEAD;

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
		if (node == NULL)
			goto FREE_NODE;

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

	if (gai_ret != 0)
		return NULL;

	return address;
}

int make_listener(char *hostname, char *service, int backlog, bool reuseaddr)
{
	struct addrinfo addr_req, *server_ai;

	int server_fd;
	int gai_ret;
	
	memset(&addr_req, 0x00, sizeof addr_req);
	addr_req.ai_family = AF_UNSPEC;
	addr_req.ai_socktype = SOCK_STREAM;
	addr_req.ai_flags =  AI_ALL;

	gai_ret = getaddrinfo(hostname, service, &addr_req, &server_ai);
	if (gai_ret != 0)
		goto RETURN_ERROR;

	server_fd = socket(
		server_ai->ai_family,
		server_ai->ai_socktype,
		server_ai->ai_protocol
	);
	if (server_fd == -1)
		goto FREEADDRINFO;

	if (reuseaddr && reuse_address(server_fd) == -1)
		goto CLOSE_SERVER;

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

int send_file(int fd, char *filename)
{
	int rfd = open(filename, O_RDWR);
	int total_len = 0;

	if (rfd == -1)
		return -1;

	while (true)
	{
		char buffer[BUFSIZ];
		int readlen = read(rfd, buffer, BUFSIZ);

		if (readlen == 0)
			break;

		if (readlen == -1) {
			close(rfd);
			return -1;
		}

		if (send(fd, buffer, readlen, 0) == -1) {
			close(rfd);
			return -1;
		}

		total_len += readlen;
	}

	close(rfd);

	return total_len;
}
