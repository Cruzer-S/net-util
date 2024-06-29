#ifndef NET_UTIL_H__
#define NET_UTIL_H__

#include "Cruzer-S/list/list.h"
#include "Cruzer-S/logger/logger.h"

#include <stdint.h>

#include <sys/socket.h> // struct sockaddr_storage

struct address_data_node {
	struct sockaddr_storage address;
	struct list list;
};

struct list *get_interface_address(int family, int flags, int masks);
void free_interface_address(struct list *head);

char *get_host_from_address(struct sockaddr_storage *storage, int flags);

int server_create(char *hostname, char *service, int backlog, bool reuseaddr);

int fcntl_set_nonblocking(int fd);

int reuse_address(int fd);

void net_util_set_logger(Logger );

char *get_epoll_event_name(uint32_t events);

#endif
