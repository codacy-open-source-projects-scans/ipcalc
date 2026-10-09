/*
 * LD_PRELOAD shim for the JSON escaping test: every reverse lookup
 * resolves to a name containing characters that JSON requires to be
 * escaped, which no real resolver configuration available to the test
 * suite can provide.
 */

#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
		char *host, socklen_t hostlen,
		char *serv, socklen_t servlen, int flags)
{
	static const char name[] = "a\"b\\c\td";

	(void)sa;
	(void)salen;
	(void)serv;
	(void)servlen;
	(void)flags;

	if (host == NULL || hostlen < sizeof(name))
		return EAI_OVERFLOW;

	memcpy(host, name, sizeof(name));
	return 0;
}
