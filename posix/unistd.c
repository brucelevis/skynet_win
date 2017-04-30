#include "unistd.h"
#include <conio.h>
#include <Windows.h>
#include <WinSock2.h>
#include <stdlib.h>


//////////////////////////////////////////////////////////////////////////
__attribute__((constructor))
static void main_construct() {
	epoll_startup();
}
__attribute__((destructor))
static  void main_destruct() {
	epoll_cleanup();
}
//////////////////////////////////////////////////////////////////////////

LONG _InterlockedAdd_r(LONG volatile *Addend, LONG Value) {
	return InterlockedExchangeAdd(Addend, Value) + Value;
}

pid_t getpid() {
	return GetCurrentProcessId();
}

int kill(pid_t pid, int sig) {
	HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);
	if (h == NULL)
		return errno = EINVAL;
	if (!TerminateProcess(h, 127)) {
		errno = EINVAL;
		CloseHandle(h);
		return -1;
	};
	CloseHandle(h);
	return 0;
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

int sigaction(int sig, struct sigaction *in, struct sigaction *out) {
	if (in->sa_flags & SA_SIGINFO)
		signal(sig, in->sa_sigaction);
	else
		signal(sig, in->sa_handler);
}

void socket_keepalive(int fd) {
	int keepalive = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
	assert(ret != SOCKET_ERROR);
}

int _win_socketpair(int af, int type, int protocol, int socket_vector[2])
{
	int iRet;
	SOCKET listening_socket = INVALID_SOCKET;
	SOCKET client_socket = INVALID_SOCKET;
	SOCKET server_socket = INVALID_SOCKET;
	struct sockaddr_in listen_on;
	struct sockaddr_in connected_as;
	struct sockaddr_in connecting;
	int alen;
	int res;
	unsigned long p;

	do
	{
		listening_socket = INVALID_SOCKET;
		client_socket = INVALID_SOCKET;
		server_socket = INVALID_SOCKET;

		client_socket = socket(af, type, protocol);
		if (client_socket == INVALID_SOCKET)
			goto fail_socketpair;

		listening_socket = socket(af, type, protocol);
		if (listening_socket == INVALID_SOCKET)
			goto fail_socketpair;

		p = 1;
		res = ioctlsocket(client_socket, FIONBIO, &p);
		if (res != 0)
			goto fail_socketpair;

		alen = sizeof(listen_on);
		listen_on.sin_family = af;
		listen_on.sin_port = 0;
		listen_on.sin_addr.S_un.S_un_b.s_b1 = 127;
		listen_on.sin_addr.S_un.S_un_b.s_b2 = 0;
		listen_on.sin_addr.S_un.S_un_b.s_b3 = 0;
		listen_on.sin_addr.S_un.S_un_b.s_b4 = 1;
		res = bind(listening_socket, (const struct sockaddr *) &listen_on, alen);
		if (res != 0)
			goto fail_socketpair;

		res = getsockname(listening_socket, (struct sockaddr *) &listen_on, &alen);
		if (res != 0)
			goto fail_socketpair;

		res = listen(listening_socket, 0);
		if (res != 0)
			goto fail_socketpair;

		res = connect(client_socket, (const struct sockaddr *) &listen_on, alen);
		if ((res != 0) && WSAEWOULDBLOCK != GetLastError())
			goto fail_socketpair;

		server_socket = accept(listening_socket, (struct sockaddr *) &connecting, &alen);
		if (server_socket == INVALID_SOCKET)
			goto fail_socketpair;

		closesocket(listening_socket);
		listening_socket = INVALID_SOCKET;

		res = getsockname(client_socket, (struct sockaddr *) &connected_as, &alen);
		if (res != 0)
			goto fail_socketpair;

		if (memcmp(&connected_as, &connecting, alen) != 0)
		{
			closesocket(client_socket);
			closesocket(server_socket);
			continue;
		}

		p = 0;
		ioctlsocket(server_socket, FIONBIO, &p);
		ioctlsocket(client_socket, FIONBIO, &p);
		socket_keepalive(server_socket);
		socket_keepalive(client_socket);

		socket_vector[0] = client_socket;
		socket_vector[1] = server_socket;
		return 0;
	} while (1);

fail_socketpair:
	{
		if (client_socket != INVALID_SOCKET)
			closesocket(client_socket);
		if (listening_socket != INVALID_SOCKET)
			closesocket(listening_socket);
		if (server_socket != INVALID_SOCKET)
			closesocket(server_socket);
		return -1;
	}
}

int pipe(int fds[2])
{
	return _win_socketpair(AF_INET, SOCK_STREAM, 0 ,fds);
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
