#ifndef MYCURL_H
#define MYCURL_H

#include <stddef.h>

int parse_url(const char *url, char **host, char **port, char **path);

/*
 * Send all bytes in buf through sockfd.
 * Returns 0 on success, -1 on failure.
 */
int send_all(int sockfd, const char *buf, size_t len);

#endif /* MYCURL_H */