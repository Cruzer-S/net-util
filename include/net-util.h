#ifndef NET_UTIL_H__
#define NET_UTIL_H__

#include "Cruzer-S/list/list.h"

#include <sys/socket.h> // struct sockaddr_storage

struct address_data_node {
	struct sockaddr_storage address;
	struct list list;
};

struct list get_interface_address(int family, int flags, int masks);

char *get_host_from_address(struct sockaddr_storage *storage, int flags);

int server_create(char *hostname, char *service, int backlog);

#endif
