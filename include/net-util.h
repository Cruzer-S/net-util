#ifndef NET_UTIL_H__
#define NET_UTIL_H__

#include "Cruzer-S/list/list.h"

#include <stdint.h> // uint32_t
#include <stdbool.h> // bool

#include <sys/socket.h> // struct sockaddr_storage

struct address_data_node {
	struct sockaddr_storage address;
	struct list list;
};

struct list *get_interface_address(int family, int flags, int masks);
void free_interface_address(struct list *head);

char *get_host_from_address(struct sockaddr_storage *storage, int flags);

char *get_hostname(int family);

int make_listener(char *hostname, char *service, int backlog, bool reuseaddr);

int fcntl_set_nonblocking(int fd);

int reuse_address(int fd);

char *get_epoll_event_name(uint32_t events);

int send_file(int fd, char *filename);

#endif
