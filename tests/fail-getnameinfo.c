/*
 * LD_PRELOAD shim for the -h network test: every reverse lookup fails,
 * so that the test can tell whether ipcalc attempted one.
 */

#include <sys/socket.h>
#include <netdb.h>

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
		char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags)
{
	(void)sa;
	(void)salen;
	(void)host;
	(void)hostlen;
	(void)serv;
	(void)servlen;
	(void)flags;

	return EAI_FAIL;
}
