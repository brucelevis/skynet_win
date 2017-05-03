/****************************************************************************
Copyright (c) 2015-2017      dpull.com
http://www.dpull.com
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include <unistd.h>
#include <spinlock.h>
#include <Winsock2.h>
#include <conio.h>
#include <errno.h>
#include <assert.h>
#include <skynet_malloc.h>
#include "epoll.h"

struct fd_t
{
    int fd;
    struct epoll_event event;
};

struct epoll_fd
{
    int max_size;
    struct fd_set readfds;    
    struct fd_set writefds;    
    struct fd_set exceptfds;    
    struct fd_t* fds;
	struct spinlock lock;
};

int epoll_startup()
{
    WSADATA wsadata;
    return WSAStartup(MAKEWORD(2, 2), &wsadata);
}

/*
http://linux.die.net/man/2/epoll_create
*/
int epoll_create(int size)
{
    assert(size <= FD_SETSIZE);
    if(size < 0 || size > FD_SETSIZE) {
        errno = EINVAL;
        return 0;
    }

    struct epoll_fd* epoll = (struct epoll_fd*)skynet_malloc(sizeof(*epoll));
    memset(epoll, 0, sizeof(*epoll));

	epoll->max_size = size;
	epoll->fds = (struct fd_t*)skynet_malloc(sizeof(*(epoll->fds)) * size);
    memset(epoll->fds, 0, sizeof(*(epoll->fds)) * size);
    for (int i = 0; i < size; ++i)
		epoll->fds[i].fd = INVALID_SOCKET;
	SPIN_INIT(epoll);
    return (int)epoll;
}

static struct fd_t* epoll_get_fds(struct epoll_fd* epoll, int fd)
{
	for (int i = 0; i < epoll->max_size; ++i){
		if (epoll->fds[i].fd == fd)
			return &(epoll->fds[i]);
	}
	return NULL;
}

static int epoll_ctl_add(struct epoll_fd* epoll, int fd, struct epoll_event* event)
{
    assert(!(event->events & EPOLLET));
    assert(!(event->events & EPOLLONESHOT));

    unsigned long hl;
    if (WSANtohl(fd, 1, &hl) == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK)
        return EBADF;

	struct fd_t* fds = epoll_get_fds(epoll, fd);
	if (fds != NULL) return EEXIST;

	fds = epoll_get_fds(epoll, INVALID_SOCKET);
	if (fds == NULL) return ENOMEM;

	fds->fd = fd;
	fds->event = *event;
	return 0;
}

static int epoll_ctl_mod(struct epoll_fd* epoll, int fd, struct epoll_event* event)
{
    assert(!(event->events & EPOLLET));
    assert(!(event->events & EPOLLONESHOT));

	struct fd_t* fds = epoll_get_fds(epoll, fd);
	if (fds == NULL) return ENOENT;

	fds->event = *event;
	return 0;
}

static int epoll_ctl_del(struct epoll_fd* epoll, int fd, struct epoll_event* event)
{
	struct fd_t* fds = epoll_get_fds(epoll, fd);
	if (fds == NULL) return ENOENT;

	fds->fd = INVALID_SOCKET;
	return 0;
}

/*
http://linux.die.net/man/2/epoll_ctl
*/
int epoll_ctl(int epfd, int opcode, int fd, struct epoll_event* event)
{
    int error = ENOENT;
    struct epoll_fd* epoll = (struct epoll_fd*)epfd;
	SPIN_LOCK(epoll);
    switch (opcode) {
        case EPOLL_CTL_ADD:
            error = epoll_ctl_add(epoll, fd, event);
            break;
        case EPOLL_CTL_MOD:
            error = epoll_ctl_mod(epoll, fd, event);
            break;  
        case EPOLL_CTL_DEL:
            error = epoll_ctl_del(epoll, fd, event);
            break;           
    }
	SPIN_UNLOCK(epoll);

    if (error != 0)
        errno = error;
    return error != 0 ? -1 : 0;
}

static void epoll_wait_init(struct epoll_fd* epoll)
{
    FD_ZERO(&epoll->readfds);
    FD_ZERO(&epoll->writefds);
    FD_ZERO(&epoll->exceptfds);

    for (int i = 0; i < epoll->max_size; ++i) {
        struct fd_t* fds = epoll->fds + i;
        if (fds->fd == INVALID_SOCKET)
            continue;

        if (fds->event.events & EPOLLIN || fds->event.events & EPOLLPRI)
            FD_SET(fds->fd, &epoll->readfds);

        if (fds->event.events & EPOLLOUT)
            FD_SET(fds->fd, &epoll->writefds);

        if (fds->event.events & EPOLLERR || fds->event.events & EPOLLRDHUP)
            FD_SET(fds->fd, &epoll->exceptfds);
    }    
}

static int epoll_wait_get_result(struct epoll_fd* epoll, struct epoll_event* events, int maxevents)
{
    int evt_idx = 0;
    for (int i = 0; i < epoll->max_size; ++i) {
        struct fd_t* fds = epoll->fds + i;
        if (fds->fd == INVALID_SOCKET)
            continue;

        struct epoll_event* event = events + evt_idx;
        event->events = 0;
        if (FD_ISSET(fds->fd, &epoll->readfds)) {
            if (fds->event.events & EPOLLIN)  
                event->events |= EPOLLIN;
            else if (fds->event.events & EPOLLPRI)  
                event->events |= EPOLLPRI;
        }

        if (FD_ISSET(fds->fd, &epoll->writefds)) {
            event->events |= EPOLLOUT;
        }

        if (FD_ISSET(fds->fd, &epoll->exceptfds)) {
            if (fds->event.events & EPOLLERR)  
                event->events |= EPOLLERR;
            else if (fds->event.events & EPOLLRDHUP)  
                event->events |= EPOLLRDHUP;
        }

        if (event->events != 0) {
            event->data = fds->event.data;
            if (++evt_idx == maxevents)
                break;
        }
    }   
    return evt_idx;
}

/*
http://linux.die.net/man/2/epoll_wait
*/
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
    struct epoll_fd* epoll = (struct epoll_fd*)epfd;

	SPIN_LOCK(epoll);
    epoll_wait_init(epoll);
	SPIN_UNLOCK(epoll);

    struct timeval wait_time = {timeout / 1000, 1000 * (timeout % 1000)};
    int total = select(1, &epoll->readfds, &epoll->writefds, &epoll->exceptfds, timeout >= 0 ? &wait_time : NULL);
    if (total == 0)
        return 0;

    if (total < 0) {
		errno = wsaerr();
        return SOCKET_ERROR;
    }

	SPIN_LOCK(epoll);
    int count = epoll_wait_get_result(epoll, events, maxevents);
	SPIN_UNLOCK(epoll);
    return count;
}

int epoll_close(int epfd)
{
    struct epoll_fd* epoll = (struct epoll_fd*)epfd;
	SPIN_DESTROY(epoll);
    skynet_free(epoll->fds);
	skynet_free(epoll);
    return 0;
}

void epoll_cleanup()
{
    WSACleanup();
}
