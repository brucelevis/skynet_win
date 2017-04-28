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

typedef int ssize_t;

#define FD_SETSIZE 4096

#define random rand
#define srandom srand
#define snprintf _snprintf

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
int kill(pid_t pid, int exit_code);

void usleep(__int64 usec);
void sleep(size_t ms);

enum { CLOCK_THREAD_CPUTIME_ID, CLOCK_REALTIME, CLOCK_MONOTONIC };
int clock_gettime(int what, struct timespec *ti);

struct sigaction {
	void (*sa_handler)(int);
	int sa_flags;
	int sa_mask;
};
enum { SIGPIPE, SIGHUP, SA_RESTART };
void sigfillset(int *flag);
void sigaction(int flag, struct sigaction *action, int param);

int pipe(int fd[2]);
int daemon(int a, int b);

int write(int fd, const void *ptr, size_t sz);
int read(int fd, void *buffer, size_t sz);
int close(int fd);
int fcntl(int fd, int cmd, long arg);
int flock(int fd, int op);

char *strsep(char **stringp, const char *delim);