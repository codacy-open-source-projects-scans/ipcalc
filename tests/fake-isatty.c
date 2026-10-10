/*
 * LD_PRELOAD shim for the deprecation hint tests: stderr is reported as
 * a terminal, which the test suite cannot otherwise provide, while
 * stdout is not, so that the output stays uncolored.
 */

#include <unistd.h>

int isatty(int fd)
{
	return fd == STDERR_FILENO;
}
