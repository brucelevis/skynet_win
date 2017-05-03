#pragma once
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "sync.h"
#include "spoll.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

typedef int ssize_t;

#define FD_SETSIZE 4096

#define random rand
#define srandom srand
#define snprintf _snprintf

#define connect s_connect
#define send	s_send
#define recv	s_recv
#define sendto	s_sendto
#define recvfrom s_recvfrom

#define O_NONBLOCK 1
#define F_SETFL 0
#define F_GETFL 1

#define fsync _commit
#define ftruncate chsize

#define O_RDWR       _O_RDWR
#define O_CREAT      _O_CREAT

/* For flock() emulation */

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8

__declspec(dllimport) int __stdcall gethostname(char *buffer, int len);

pid_t getpid();
int kill(pid_t pid, int sig);

void usleep(__int64 usec);
void sleep(size_t ms);

enum { CLOCK_THREAD_CPUTIME_ID, CLOCK_REALTIME, CLOCK_MONOTONIC };
int clock_gettime(int what, struct timespec *ti);

#define SIGHUP	 1
#define SIGKILL	 9
#define SIGPIPE 13

#define SA_SIGINFO      0x00000004u
#define SA_RESTART      0x10000000u

struct sigaction {
	int sa_flags;
	int sa_mask;
	void(*sa_handler)(int);
	void(*sa_sigaction)(int);
};

#define sigfillset(pset)   (*(pset) = (unsigned int)-1)
int sigaction(int sig, struct sigaction *in, struct sigaction *out);

int pipe(int fd[2]);
int daemon(int a, int b);

int write(int fd, const void *ptr, size_t sz);
int read(int fd, void *buffer, size_t sz);
int close(int fd);
int fcntl(int fd, int cmd, long arg);
int flock(int fd, int op);

int wsaerr();
char *strsep(char **stringp, const char *delim);
