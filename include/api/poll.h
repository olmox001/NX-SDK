#ifndef _POLL_H
#define _POLL_H

/*
 * Minimal POSIX <poll.h> for the OS1 userspace libc.
 *
 * poll() checks real per-process capability handles. PIPE readiness and EOF
 * use a lock-consistent object control query; FILE handles are immediately
 * ready for authorized operations. The CONSOLE input queue and non-fd service
 * object kinds explicitly return ENOTSUP until a readiness provider exists.
 * No new syscall or host-libc fallback is involved.
 */

typedef unsigned long nfds_t;

struct pollfd {
  int fd;
  short events;
  short revents;
};

#define POLLIN  0x0001
#define POLLPRI 0x0002
#define POLLOUT 0x0004
#define POLLERR 0x0008
#define POLLHUP 0x0010
#define POLLNVAL 0x0020

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

#endif /* _POLL_H */
