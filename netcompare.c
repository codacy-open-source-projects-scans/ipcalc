/*
 * Copyright (c) 2026 Nikos Mavrogiannopoulos
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ipcalc.h"

struct binary_network {
	unsigned char addr[16];
	unsigned prefix;
	int family;
};

/* Bytewise, so that no shift reaches the width of an integer */
static int same_leading_bits(const unsigned char *a, const unsigned char *b, unsigned nbits)
{
	unsigned bytes = nbits / 8, bits = nbits % 8;

	if (memcmp(a, b, bytes) != 0)
		return 0;
	if (bits == 0)
		return 1;
	return ((a[bytes] ^ b[bytes]) & (0xff << (8 - bits)) & 0xff) == 0;
}

static int network_contains(const struct binary_network *outer, const struct binary_network *inner)
{
	return outer->family == inner->family && outer->prefix <= inner->prefix &&
	       same_leading_bits(outer->addr, inner->addr, outer->prefix);
}

/* ip_info_st keeps the network address only as text */
static int to_binary_network(const ip_info_st *info, unsigned flags, struct binary_network *net)
{
	net->family = (flags & FLAG_IPV6) ? AF_INET6 : AF_INET;
	net->prefix = info->prefix;
	memset(net->addr, 0, sizeof(net->addr));
	return inet_pton(net->family, info->network, net->addr) == 1 ? 0 : -1;
}

static int comparison_holds(enum net_comparison op, const struct binary_network *a,
			    const struct binary_network *b)
{
	switch (op) {
	case CMP_EQUALS:
		return a->family == b->family && a->prefix == b->prefix &&
		       memcmp(a->addr, b->addr, sizeof(a->addr)) == 0;
	case CMP_SUBNET_OF:
		return network_contains(b, a);
	case CMP_OVERLAPS:
	default:
		/* CIDR networks are either nested or disjoint */
		return network_contains(a, b) || network_contains(b, a);
	}
}

static int parse_network(char *str, unsigned *flags, ip_info_st *info)
{
	char *prefixStr;
	int prefix = -1;

	if ((*flags & FLAG_IPV4) == 0 && strchr(str, ':') != NULL)
		*flags |= FLAG_IPV6;

	prefixStr = strchr(str, '/');
	if (prefixStr != NULL) {
		*prefixStr++ = '\0';
		prefix = str_to_prefix(flags, prefixStr, 0);
		if (prefix < 0) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: bad %s prefix: %s\n", (*flags & FLAG_IPV6)?"IPv6":"IPv4", prefixStr);
			return -1;
		}
	}

	if (*flags & FLAG_IPV6)
		return get_ipv6_info(str, prefix, info, *flags);
	return get_ipv4_info(str, prefix, info, *flags);
}

static void print_comparison(enum net_comparison op, int holds, const ip_info_st *network,
			     const ip_info_st *other)
{
	unsigned jsonchain = JSON_FIRST;

	if (op == CMP_SUBNET_OF)
		default_printf(&jsonchain, "", NULL, "%s/%u is %sa subnet of %s/%u",
			       network->network, network->prefix, holds ? "" : "not ",
			       other->network, other->prefix);
	else if (op == CMP_EQUALS)
		default_printf(&jsonchain, "", NULL, "The networks are %sequal",
			       holds ? "" : "not ");
	else
		default_printf(&jsonchain, "", NULL, "The networks %s",
			       holds ? "overlap" : "do not overlap");
}

int compare_networks(enum net_comparison op, const ip_info_st *network, unsigned flags,
		     char *otherStr, unsigned userFlags)
{
	ip_info_st other;
	struct binary_network a, b;
	int holds;

	if (parse_network(otherStr, &userFlags, &other) < 0)
		return exit_failure;

	if (to_binary_network(network, flags, &a) < 0 ||
	    to_binary_network(&other, userFlags, &b) < 0)
		return exit_failure;

	holds = comparison_holds(op, &a, &b);
	/* the result has no JSON or undecorated form; the exit status carries it */
	if (!beSilent && !(flags & (FLAG_JSON | FLAG_NO_DECORATE)))
		print_comparison(op, holds, network, &other);
	return holds ? 0 : 1;
}
