/*
 * Copyright (c) 2015 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Nikos Mavrogiannopoulos <nmav@redhat.com>
 */

#define _GNU_SOURCE		/* asprintf */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "ipcalc.h"

/* draft-ietf-dnsop-rfc2317bis-00 replaces that legacy style
 */
#undef USE_RFC2317_STYLE

char *calc_reverse_dns4(struct in_addr ip, unsigned prefix, struct in_addr network, struct in_addr broadcast)
{
	char *str = NULL;
	int ret = -1;

	unsigned byte1 = (ntohl(ip.s_addr) >> 24) & 0xff;
	unsigned byte2 = (ntohl(ip.s_addr) >> 16) & 0xff;
	unsigned byte3 = (ntohl(ip.s_addr) >> 8) & 0xff;
	unsigned byte4 = (ntohl(ip.s_addr)) & 0xff;

#ifdef USE_RFC2317_STYLE
	if (prefix == 32) {
		ret = asprintf(&str, "%u.%u.%u.%u.in-addr.arpa.", byte4, byte3, byte2, byte1);
	} else if (prefix == 24) {
		ret = asprintf(&str, "%u.%u.%u.in-addr.arpa.", byte3, byte2, byte1);
	} else if (prefix == 16) {
		ret = asprintf(&str, "%u.%u.in-addr.arpa.", byte2, byte1);
	} else if (prefix == 8) {
		ret = asprintf(&str, "%u.in-addr.arpa.", byte1);
	} else if (prefix > 24) {
		ret = asprintf(&str, "%u/%u.%u.%u.%u.in-addr.arpa.", byte4, prefix, byte3, byte2, byte1);
	} else if (prefix > 16) {
		ret = asprintf(&str, "%u/%u.%u.%u.in-addr.arpa.", byte3, prefix, byte2, byte1);
	} else if (prefix > 8) {
		ret = asprintf(&str, "%u/%u.%u.in-addr.arpa.", byte2, prefix, byte1);
	}
#else
	if (prefix == 32) {
		ret = asprintf(&str, "%u.%u.%u.%u.in-addr.arpa.", byte4, byte3, byte2, byte1);
	} else if (prefix == 24) {
		ret = asprintf(&str, "%u.%u.%u.in-addr.arpa.", byte3, byte2, byte1);
	} else if (prefix == 16) {
		ret = asprintf(&str, "%u.%u.in-addr.arpa.", byte2, byte1);
	} else if (prefix == 8) {
		ret = asprintf(&str, "%u.in-addr.arpa.", byte1);
	} else if (prefix > 24) {
		unsigned min = (ntohl(network.s_addr)) & 0xff;
		unsigned max = (ntohl(broadcast.s_addr)) & 0xff;
		ret = asprintf(&str, "%u-%u.%u.%u.%u.in-addr.arpa.", min, max, byte3, byte2, byte1);
	} else if (prefix > 16) {
		unsigned min = (ntohl(network.s_addr) >> 8) & 0xff;
		unsigned max = (ntohl(broadcast.s_addr) >> 8) & 0xff;
		ret = asprintf(&str, "%u-%u.%u.%u.in-addr.arpa.", min, max, byte2, byte1);
	} else if (prefix > 8) {
		unsigned min = (ntohl(network.s_addr) >> 16) & 0xff;
		unsigned max = (ntohl(broadcast.s_addr) >> 16) & 0xff;
		ret = asprintf(&str, "%u-%u.%u.in-addr.arpa.", min, max, byte1);
	} else if (prefix > 0) {
		unsigned min = (ntohl(network.s_addr) >> 24) & 0xff;
		unsigned max = (ntohl(broadcast.s_addr) >> 24) & 0xff;
		ret = asprintf(&str, "%u-%u.in-addr.arpa.", min, max);
	} else {
		ret = asprintf(&str, "in-addr.arpa.");
	}
#endif

	if (ret == -1)
	    return NULL;
	return str;
}

static char hexchar(unsigned int val)
{
	if (val < 10)
		return '0' + val;
	if (val < 16)
		return 'a' + val - 10;
	abort();
}

/* Returns the value of the idx-th nibble of ip, counting from the most
 * significant one.
 */
static unsigned nibble(const struct in6_addr *ip, unsigned idx)
{
	unsigned byte = ip->s6_addr[idx / 2];

	return (idx % 2 == 0) ? (byte >> 4) : (byte & 0xf);
}

/* Stores in names the ip6.arpa. domains covering the network ip/prefix and
 * returns their number. The caller owns the returned strings.
 *
 * The nibble format of RFC 3596 can only express prefixes that are a multiple
 * of 4 (the bit-string labels of RFC 2673 that could do otherwise are no
 * longer in use, see RFC 3363). A network with any other prefix is covered by
 * the 2^(4 - prefix % 4) domains at the next nibble boundary, which are
 * returned in ascending order.
 */
unsigned calc_reverse_dns6(struct in6_addr *ip, unsigned prefix, char *names[MAX_REVERSE_DNS])
{
	/* 32 nibbles of "x." followed by "ip6.arpa." */
	char str[32 * 2 + sizeof("ip6.arpa.")];
	unsigned nibbles = (prefix + 3) / 4;
	unsigned host_bits = nibbles * 4 - prefix;
	unsigned count = 1u << host_bits;
	unsigned i, k, j;

	for (k = 0; k < count; k++) {
		j = 0;
		for (i = nibbles; i > 0; i--) {
			unsigned val = nibble(ip, i - 1);

			if (i == nibbles)
				val = (val & ~(count - 1)) + k;
			str[j++] = hexchar(val);
			str[j++] = '.';
		}
		strcpy(&str[j], "ip6.arpa.");
		names[k] = safe_strdup(str);
	}

	return count;
}
