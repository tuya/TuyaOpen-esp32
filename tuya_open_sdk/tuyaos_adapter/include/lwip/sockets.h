/**
 * @file sockets.h
 * @brief ESP32 lwIP socket wrapper for the TuyaOpen root build.
 *
 * ESP-IDF's lwipopts.h sets LWIP_COMPAT_SOCKETS to 0, so the BSD names used
 * by core_network.c are not declared. Pull in the ESP-IDF header and add them
 * as inline functions (not macros, so structure members named send/recv are
 * unaffected).
 */
#ifndef ESP32_LWIP_SOCKETS_WRAPPER_H
#define ESP32_LWIP_SOCKETS_WRAPPER_H

#include_next "lwip/sockets.h"
#include "lwip/netdb.h"

#if !LWIP_COMPAT_SOCKETS

static inline int socket(int domain, int type, int protocol)
{
    return lwip_socket(domain, type, protocol);
}

static inline int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_bind(s, name, namelen);
}

static inline int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    return lwip_connect(s, name, namelen);
}

static inline int listen(int s, int backlog)
{
    return lwip_listen(s, backlog);
}

static inline int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return lwip_accept(s, addr, addrlen);
}

static inline int shutdown(int s, int how)
{
    return lwip_shutdown(s, how);
}

static inline ssize_t send(int s, const void *dataptr, size_t size, int flags)
{
    return lwip_send(s, dataptr, size, flags);
}

static inline ssize_t sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to,
                             socklen_t tolen)
{
    return lwip_sendto(s, dataptr, size, flags, to, tolen);
}

static inline ssize_t recv(int s, void *mem, size_t len, int flags)
{
    return lwip_recv(s, mem, len, flags);
}

static inline ssize_t recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    return lwip_recvfrom(s, mem, len, flags, from, fromlen);
}

static inline int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    return lwip_setsockopt(s, level, optname, optval, optlen);
}

static inline int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    return lwip_getsockopt(s, level, optname, optval, optlen);
}

static inline int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getsockname(s, name, namelen);
}

static inline int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    return lwip_getpeername(s, name, namelen);
}

static inline struct hostent *gethostbyname(const char *name)
{
    return lwip_gethostbyname(name);
}

#endif /* !LWIP_COMPAT_SOCKETS */

#endif /* ESP32_LWIP_SOCKETS_WRAPPER_H */
