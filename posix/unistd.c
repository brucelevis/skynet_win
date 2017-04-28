#include "unistd.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <conio.h>
#include <Windows.h>
#include <WinSock2.h>
#include <stdlib.h>

LONG _InterlockedAdd_r(LONG volatile *Addend, LONG Value) {
	return InterlockedExchangeAdd(Addend, Value) + Value;
}

pid_t getpid() {
	return GetCurrentProcess();
}

int kill(pid_t pid, int exit_code) {
	return TerminateProcess(pid, exit_code);
}

void usleep(__int64 usec)
{
	HANDLE timer;
	LARGE_INTEGER ft;
	ft.QuadPart = -(10 * usec);
	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

void sleep(size_t ms) {
	usleep(ms * (1000 * 1000));
}

int clock_gettime(int what, struct timespec *ti) {
	return timespec_get(ti, TIME_UTC);
}

void sigfillset(int *flag) {
	// Not implemented
}

void sigaction(int flag, struct sigaction *action, int param) {
	// Not implemented
	//__asm int 3;
}

int pipe(int fds[2])
{
	struct sockaddr_in name;
	int namelen = sizeof(name);
	SOCKET server = INVALID_SOCKET;
	SOCKET client1 = INVALID_SOCKET;
	SOCKET client2 = INVALID_SOCKET;

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
		goto failed;

	if (bind(server, (struct sockaddr*)&name, namelen) == SOCKET_ERROR)
		goto failed;

	if (listen(server, 5) == SOCKET_ERROR)
		goto failed;

	if (getsockname(server, (struct sockaddr*)&name, &namelen) == SOCKET_ERROR)
		goto failed;

	client1 = socket(AF_INET, SOCK_STREAM, 0);
	if (client1 == INVALID_SOCKET)
		goto failed;

	if (connect(client1, (struct sockaddr*)&name, namelen) == SOCKET_ERROR)
		goto failed;

	client2 = accept(server, (struct sockaddr*)&name, &namelen);
	if (client2 == INVALID_SOCKET)
		goto failed;

	closesocket(server);
	fds[0] = client1;
	fds[1] = client2;
	return 0;

failed:
	if (server != INVALID_SOCKET)
		closesocket(server);

	if (client1 != INVALID_SOCKET)
		closesocket(client1);

	if (client2 != INVALID_SOCKET)
		closesocket(client2);
	return -1;
}

int write(int fd, const void *ptr, size_t sz) {

	WSABUF vecs[1];
	vecs[0].buf = ptr;
	vecs[0].len = sz;

    DWORD bytesSent;
    if(WSASend(fd, vecs, 1, &bytesSent, 0, NULL, NULL))
        return -1;
    else
        return bytesSent;
}

int read(int fd, void *buffer, size_t sz) {

	// read console input
	if(fd == 0) {
		char *buf = (char *) buffer;
		while(buf - (char *) buffer < sz) {

			if(!_kbhit())
				break;
			char ch = _getch();
			*buf++ = ch;
			_putch(ch);
			if(ch == '\r') {
				if(buf - (char *) buffer >= sz)
					break;
				*buf++ = '\n';
				_putch('\n');
			}
		}
		return buf - (char *) buffer;
	}

	WSABUF vecs[1];
	vecs[0].buf = buffer;
	vecs[0].len = sz;

    DWORD bytesRecv = 0;
    DWORD flags = 0;
    if(WSARecv(fd, vecs, 1, &bytesRecv, &flags, NULL, NULL)) {
		if(WSAGetLastError() == WSAECONNRESET)
			return 0;
        return -1;
	} else
        return bytesRecv;
}

int close(int fd) {
	shutdown(fd, SD_BOTH);
	return closesocket(fd);
}

int daemon(int a, int b) {
	// Not implemented
	//__asm int 3;
	return 0;
}

char *strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

int fcntl(int fd, int cmd, long arg) {
	if (cmd == F_GETFL)
		return 0;

	if (cmd == F_SETFL && arg == O_NONBLOCK) {
		u_long ulOption = 1;
		ioctlsocket(fd, FIONBIO, &ulOption);
	}
	return 1;
}

void socket_keepalive(int fd) {
	int keepalive = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));

	assert(ret != SOCKET_ERROR);
}
