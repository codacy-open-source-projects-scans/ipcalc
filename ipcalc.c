/*
 * Copyright (c) 1997-2015 Red Hat, Inc. All rights reserved.
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
 *
 * Authors:
 *   Nikos Mavrogiannopoulos <nmav@redhat.com>
 *   Erik Troan <ewt@redhat.com>
 *   Preston Brown <pbrown@redhat.com>
 *   David Cantrell <dcantrell@redhat.com>
 */

#define _GNU_SOURCE		/* asprintf */
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>		/* open */
#include <fcntl.h>		/* open */
#include <unistd.h>		/* read */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>		/* clock_gettime */
#include "ipcalc.h"

int beSilent = 0;
int exit_failure = 1;
static unsigned colors = 0;
static unsigned flags = 0;

/*!
  \file ipcalc.c
  \brief provides utilities for manipulating IP addresses.

  ipcalc provides utilities and a front-end command line interface for
  manipulating IP addresses, and calculating various aspects of an ip
  address/netmask/network address/prefix/etc.

  Functionality can be accessed from other languages from the library
  interface, documented here.  To use ipcalc from the shell, read the
  ipcalc(1) manual page.

  When passing parameters to the various functions, take note of whether they
  take host byte order or network byte order.  Most take host byte order, and
  return host byte order, but there are some exceptions.
*/

/*!
  \fn uint32_t prefix2mask(int bits)
  \brief creates a netmask from a specified number of bits

  This function converts a prefix length to a netmask.  As CIDR (classless
  internet domain internet domain routing) has taken off, more an more IP
  addresses are being specified in the format address/prefix
  (i.e. 192.168.2.3/24, with a corresponding netmask 255.255.255.0).  If you
  need to see what netmask corresponds to the prefix part of the address, this
  is the function.  See also \ref mask2prefix.

  \param prefix is the number of bits to create a mask for.
  \return a network mask, in network byte order.
*/
uint32_t prefix2mask(int prefix)
{
	struct in_addr mask;
	memset(&mask, 0, sizeof(mask));
	if (prefix) {
		return htonl(~((1U << (32 - prefix)) - 1));
	} else {
		return htonl(0);
	}
}

/*!
  \fn struct in_addr calc_broadcast(struct in_addr addr, int prefix)

  \brief calculate broadcast address given an IP address and a prefix length.

  \param addr an IP address in network byte order.
  \param prefix a prefix length.

  \return the calculated broadcast address for the network, in network byte
  order.
*/
static struct in_addr calc_broadcast(struct in_addr addr, int prefix)
{
	struct in_addr mask;
	struct in_addr broadcast;

	mask.s_addr = prefix2mask(prefix);

	memset(&broadcast, 0, sizeof(broadcast));

	/* Follow RFC3021 and set the limited broadcast address on /31 */
	if (prefix == 31)
		broadcast.s_addr = htonl(0xFFFFFFFF);
	else
		broadcast.s_addr = (addr.s_addr & mask.s_addr) | ~mask.s_addr;

	return broadcast;
}

/*!
  \fn struct in_addr calc_network(struct in_addr addr, int prefix)
  \brief calculates the network address for a specified address and prefix.

  \param addr an IP address, in network byte order
  \param prefix the network prefix
  \return the base address of the network that addr is associated with, in
  network byte order.
*/
struct in_addr calc_network(struct in_addr addr, int prefix)
{
	struct in_addr mask;
	struct in_addr network;

	mask.s_addr = prefix2mask(prefix);

	memset(&network, 0, sizeof(network));
	network.s_addr = addr.s_addr & mask.s_addr;
	return network;
}

/*!
  \fn const char *get_hostname(int family, void *addr)
  \brief returns the hostname associated with the specified IP address

  \param family the address family, either AF_INET or AF_INET6.
  \param addr an IP address to find a hostname for, in network byte order,
  should either be a pointer to a struct in_addr or a struct in6_addr.

  \return a hostname, or NULL if one cannot be determined.  Hostname is stored
  in an allocated buffer.
*/
static char *get_hostname(int family, void *addr)
{
	static char hostname[NI_MAXHOST];
	int ret = -1;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;

	if (family == AF_INET) {
		memset(&addr4, 0, sizeof(addr4));
		addr4.sin_family = AF_INET;
		memcpy(&addr4.sin_addr, addr, sizeof(struct in_addr));
		ret = getnameinfo((struct sockaddr*)&addr4, sizeof(addr4), hostname, sizeof(hostname), NULL, 0, 0);
	} else if (family == AF_INET6) {
		memset(&addr6, 0, sizeof(addr6));
		addr6.sin6_family = AF_INET6;
		memcpy(&addr6.sin6_addr, addr, sizeof(struct in6_addr));
		ret = getnameinfo((struct sockaddr*)&addr6, sizeof(addr6), hostname, sizeof(hostname), NULL, 0, 0);
	}

	if (ret != 0)
		return NULL;

	return safe_strdup(hostname);
}

/*!
  \fn const char *get_ip_address(int family, void *addr)
  \brief returns the IP address associated with the specified hostname

  \param family the requested address family or AF_UNSPEC for any
  \param host a hostname

  \return an IP address, or NULL if one cannot be determined.  The IP is stored
  in an allocated buffer.
*/
static char *get_ip_address(int family, const char *host)
{
	struct addrinfo *res, *rp;
	struct addrinfo hints;
	int err;
	static char ipname[64];
	void *addr;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;

	err = getaddrinfo(host, NULL, &hints, &res);
	if (err != 0)
		return NULL;

	for (rp=res;rp!=NULL;rp=rp->ai_next) {
		if (rp->ai_family == AF_INET)
			addr = (&((struct sockaddr_in *)(rp->ai_addr))->sin_addr);
		else
			addr = (&((struct sockaddr_in6 *)(rp->ai_addr))->sin6_addr);

		if (inet_ntop(rp->ai_family, addr, ipname, sizeof(ipname)) != NULL) {
			freeaddrinfo(res);
			return safe_strdup(ipname);
		}
	}

	freeaddrinfo(res);
	return NULL;
}

static int bit_count(uint32_t i)
{
	int c = 0;

	while (i > 0) {
		if (i & 1)
			c++;
		i >>= 1;
	}

	return c;
}

/*!
  \fn int mask2prefix(struct in_addr mask)
  \brief calculates the number of bits masked off by a netmask.

  This function calculates the significant bits in an IP address as specified by
  a netmask.  See also \ref prefix2mask.

  \param mask is the netmask, specified as an struct in_addr in network byte order.
  \return the number of significant bits.  */
static int mask2prefix(struct in_addr mask)
{
	int c = 0;
	unsigned int seen_one = 0;
	uint32_t i = ntohl(mask.s_addr);

	while (i > 0) {
		if (i & 1) {
			seen_one = 1;
			c++;
		} else {
			if (seen_one) {
				return -1;
			}
		}
		i >>= 1;
	}

	return c;
}

static
int ipv4_mask_to_int(const char *prefix)
{
	int ret;
	struct in_addr in;

	ret = inet_pton(AF_INET, prefix, &in);
	if (ret == 0)
		return -1;

	return mask2prefix(in);
}

/* Returns powers of two in textual format */
static const char *p2_table(unsigned pow)
{
	static const char *pow2[] = {
		"1",
		"2",
		"4",
		"8",
		"16",
		"32",
		"64",
		"128",
		"256",
		"512",
		"1024",
		"2048",
		"4096",
		"8192",
		"16384",
		"32768",
		"65536",
		"131072",
		"262144",
		"524288",
		"1048576",
		"2097152",
		"4194304",
		"8388608",
		"16777216",
		"33554432",
		"67108864",
		"134217728",
		"268435456",
		"536870912",
		"1073741824",
		"2147483648",
		"4294967296",
		"8589934592",
		"17179869184",
		"34359738368",
		"68719476736",
		"137438953472",
		"274877906944",
		"549755813888",
		"1099511627776",
		"2199023255552",
		"4398046511104",
		"8796093022208",
		"17592186044416",
		"35184372088832",
		"70368744177664",
		"140737488355328",
		"281474976710656",
		"562949953421312",
		"1125899906842624",
		"2251799813685248",
		"4503599627370496",
		"9007199254740992",
		"18014398509481984",
		"36028797018963968",
		"72057594037927936",
		"144115188075855872",
		"288230376151711744",
		"576460752303423488",
		"1152921504606846976",
		"2305843009213693952",
		"4611686018427387904",
		"9223372036854775808",
		"18446744073709551616",
		"36893488147419103232",
		"73786976294838206464",
		"147573952589676412928",
		"295147905179352825856",
		"590295810358705651712",
		"1180591620717411303424",
		"2361183241434822606848",
		"4722366482869645213696",
		"9444732965739290427392",
		"18889465931478580854784",
		"37778931862957161709568",
		"75557863725914323419136",
		"151115727451828646838272",
		"302231454903657293676544",
		"604462909807314587353088",
		"1208925819614629174706176",
		"2417851639229258349412352",
		"4835703278458516698824704",
		"9671406556917033397649408",
		"19342813113834066795298816",
		"38685626227668133590597632",
		"77371252455336267181195264",
		"154742504910672534362390528",
		"309485009821345068724781056",
		"618970019642690137449562112",
		"1237940039285380274899124224",
		"2475880078570760549798248448",
		"4951760157141521099596496896",
		"9903520314283042199192993792",
		"19807040628566084398385987584",
		"39614081257132168796771975168",
		"79228162514264337593543950336",
		"158456325028528675187087900672",
		"316912650057057350374175801344",
		"633825300114114700748351602688",
		"1267650600228229401496703205376",
		"2535301200456458802993406410752",
		"5070602400912917605986812821504",
		"10141204801825835211973625643008",
		"20282409603651670423947251286016",
		"40564819207303340847894502572032",
		"81129638414606681695789005144064",
		"162259276829213363391578010288128",
		"324518553658426726783156020576256",
		"649037107316853453566312041152512",
		"1298074214633706907132624082305024",
		"2596148429267413814265248164610048",
		"5192296858534827628530496329220096",
		"10384593717069655257060992658440192",
		"20769187434139310514121985316880384",
		"41538374868278621028243970633760768",
		"83076749736557242056487941267521536",
		"166153499473114484112975882535043072",
		"332306998946228968225951765070086144",
		"664613997892457936451903530140172288",
		"1329227995784915872903807060280344576",
		"2658455991569831745807614120560689152",
		"5316911983139663491615228241121378304",
		"10633823966279326983230456482242756608",
		"21267647932558653966460912964485513216",
		"42535295865117307932921825928971026432",
		"85070591730234615865843651857942052864",
		"170141183460469231731687303715884105728",
		"340282366920938463463374607431768211456",
	};
	if (pow <= 128)
		return pow2[pow];
	return "";
}

struct addr_block {
	const char *net;
	unsigned prefix;
	const char *name;
};

/* Based on IANA's iana-ipv4-special-registry (updated 2025-10-09) and
 * ipv4-address-space. The most specific block that contains the whole
 * network wins, so the order of the rows does not matter. Blocks keep the
 * name ipcalc has always printed when IANA renames them.
 */
static const struct addr_block ipv4_blocks[] = {
	{"0.0.0.0", 32, "This host on this network"},
	{"0.0.0.0", 8, "This network"},
	{"10.0.0.0", 8, "Private Use"},
	{"100.64.0.0", 10, "Shared Address Space"},
	{"127.0.0.0", 8, "Loopback"},
	{"169.254.0.0", 16, "Link Local"},
	{"172.16.0.0", 12, "Private Use"},
	{"192.0.0.0", 29, "IPv4 Service Continuity Prefix"},
	{"192.0.0.8", 32, "IPv4 dummy address"},
	{"192.0.0.9", 32, "Port Control Protocol Anycast"},
	{"192.0.0.10", 32, "Traversal Using Relays around NAT Anycast"},
	{"192.0.0.170", 32, "NAT64/DNS64 Discovery"},
	{"192.0.0.171", 32, "NAT64/DNS64 Discovery"},
	{"192.0.0.0", 24, "IETF Protocol Assignments"},
	{"192.0.2.0", 24, "Documentation (TEST-NET-1)"},
	{"192.31.196.0", 24, "AS112-v4"},
	{"192.52.193.0", 24, "AMT"},
	{"192.88.99.2", 32, "6a44-relay anycast address"},
	{"192.88.99.0", 24, "6 to 4 Relay Anycast (Deprecated)"},
	{"192.168.0.0", 16, "Private Use"},
	{"192.175.48.0", 24, "Direct Delegation AS112 Service"},
	{"198.18.0.0", 15, "Benchmarking"},
	{"198.51.100.0", 24, "Documentation (TEST-NET-2)"},
	{"203.0.113.0", 24, "Documentation (TEST-NET-3)"},
	{"224.0.0.0", 4, "Multicast"},
	{"255.255.255.255", 32, "Limited Broadcast"},
	{"240.0.0.0", 4, "Reserved"},
};

static const char *ipv4_net_to_type(struct in_addr net, unsigned prefix)
{
	const struct addr_block *best = NULL;
	struct in_addr block;
	unsigned i;

	for (i = 0; i < sizeof(ipv4_blocks) / sizeof(ipv4_blocks[0]); i++) {
		if (prefix < ipv4_blocks[i].prefix)
			continue;
		if (best && best->prefix >= ipv4_blocks[i].prefix)
			continue;
		if (inet_pton(AF_INET, ipv4_blocks[i].net, &block) != 1)
			abort();
		if ((net.s_addr & prefix2mask(ipv4_blocks[i].prefix)) == block.s_addr)
			best = &ipv4_blocks[i];
	}

	return best ? best->name : "Internet";
}

static
const char *ipv4_net_to_class(struct in_addr net)
{
	unsigned byte1 = (ntohl(net.s_addr) >> 24) & 0xff;

	if (byte1 < 128) {
		return "Class A";
	}

	if (byte1 >= 128 && byte1 < 192) {
		return "Class B";
	}

	if (byte1 >= 192 && byte1 < 224) {
		return "Class C";
	}

	if (byte1 >= 224 && byte1 < 240) {
		return "Class D";
	}

	return "Class E";
}

static
unsigned default_ipv4_prefix(struct in_addr net)
{
	unsigned byte1 = (ntohl(net.s_addr) >> 24) & 0xff;

	if (byte1 < 128) {
		return 8;
	}

	if (byte1 >= 128 && byte1 < 192) {
		return 16;
	}

	if (byte1 >= 192 && byte1 < 224) {
		return 24;
	}

	return 24;
}

char *ipv4_prefix_to_hosts(char *hosts, unsigned hosts_size, unsigned prefix)
{
	if (prefix >= 31) {
		snprintf(hosts, hosts_size, "%s", p2_table(32 - prefix));
	} else {
		uint64_t tmp = (UINT64_C(1) << (32 - prefix)) - 2;
		snprintf(hosts, hosts_size, "%" PRIu64, tmp);
	}
	return hosts;
}

char *ipv6_prefix_to_hosts(char *hosts, unsigned hosts_size, unsigned prefix)
{
	snprintf(hosts, hosts_size, "%s", p2_table(128 - prefix));
	return hosts;
}


int get_ipv4_info(const char *ipStr, int prefix, ip_info_st * info,
		  unsigned flags)
{
	struct in_addr ip, netmask, wildcard, network, broadcast, minhost, maxhost;
	char namebuf[INET_ADDRSTRLEN + 1];
	char errBuf[250];

	memset(info, 0, sizeof(*info));

	if (inet_pton(AF_INET, ipStr, &ip) <= 0) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: bad IPv4 address: %s\n",
				ipStr);
		return -1;
	}

	/* Handle CIDR entries such as 172/8 */
	if (prefix >= 0) {
		char *tmp = (char *)ipStr;
		int i;

		for (i = 3; i > 0; i--) {
			tmp = strchr(tmp, '.');
			if (!tmp)
				break;
			else
				tmp++;
		}

		tmp = NULL;
		for (; i > 0; i--) {
			if (asprintf(&tmp, "%s.0", ipStr) == -1) {
				fprintf(stderr,
					"Memory allocation failure line %d\n",
					__LINE__);
				exit(exit_failure);
			}
			ipStr = tmp;
		}
	} else { /* assume good old days classful Internet */
		if (flags & FLAG_ASSUME_CLASS_PREFIX)
			prefix = default_ipv4_prefix(ip);
		else
			prefix = 32;
	}

	if (prefix > 32) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: bad IPv4 prefix %d\n", prefix);
		return -1;
	}

	if (inet_ntop(AF_INET, &ip, namebuf, sizeof(namebuf)) == 0) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: error calculating the IPv4 network\n");
		return -1;
	}
	info->ip = safe_strdup(namebuf);

	netmask.s_addr = prefix2mask(prefix);
	memset(namebuf, '\0', sizeof(namebuf));

	if (inet_ntop(AF_INET, &netmask, namebuf, INET_ADDRSTRLEN) == NULL) {
		fprintf(stderr, "inet_ntop failure at line %d\n",
			__LINE__);
		exit(exit_failure);
	}
	info->netmask = safe_strdup(namebuf);
	info->prefix = prefix;

	wildcard.s_addr = ~netmask.s_addr;
	if (inet_ntop(AF_INET, &wildcard, namebuf, INET_ADDRSTRLEN) == NULL) {
		fprintf(stderr, "inet_ntop failure at line %d\n",
			__LINE__);
		exit(exit_failure);
	}
	info->wildcard = safe_strdup(namebuf);

	broadcast = calc_broadcast(ip, prefix);

	memset(namebuf, '\0', sizeof(namebuf));
	if (inet_ntop(AF_INET, &broadcast, namebuf, INET_ADDRSTRLEN) == NULL) {
		fprintf(stderr, "inet_ntop failure at line %d\n",
			__LINE__);
		exit(exit_failure);
	}
	info->broadcast = safe_strdup(namebuf);

	network = calc_network(ip, prefix);

	info->reverse_dns[0] = calc_reverse_dns4(network, prefix, network, broadcast);
	info->reverse_dns_count = info->reverse_dns[0] ? 1 : 0;

	memset(namebuf, '\0', sizeof(namebuf));
	if (inet_ntop(AF_INET, &network, namebuf, INET_ADDRSTRLEN) == NULL) {
		fprintf(stderr, "inet_ntop failure at line %d\n",
			__LINE__);
		exit(exit_failure);
	}

	info->network = safe_strdup(namebuf);

	info->type = ipv4_net_to_type(network, prefix);
	info->class = ipv4_net_to_class(network);

	if (prefix < 32) {
		memcpy(&minhost, &network, sizeof(minhost));

		if (prefix <= 30)
			minhost.s_addr = htonl(ntohl(minhost.s_addr) | 1);
		if (inet_ntop(AF_INET, &minhost, namebuf, INET_ADDRSTRLEN) ==
		    NULL) {
			fprintf(stderr, "inet_ntop failure at line %d\n",
				__LINE__);
			exit(exit_failure);
		}
		info->hostmin = safe_strdup(namebuf);

		memcpy(&maxhost, &network, sizeof(minhost));
		maxhost.s_addr |= ~netmask.s_addr;
		if (prefix <= 30) {
			maxhost.s_addr = htonl(ntohl(maxhost.s_addr) - 1);
		}
		if (inet_ntop(AF_INET, &maxhost, namebuf, sizeof(namebuf)) == 0) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: error calculating the IPv4 network\n");
			return -1;
		}

		info->hostmax = safe_strdup(namebuf);
	} else {
		info->hostmin = info->network;
		info->hostmax = info->network;
	}

	ipv4_prefix_to_hosts(info->hosts, sizeof(info->hosts), prefix);

#ifdef USE_MAXMIND
	if (flags & FLAG_GET_GEOIP) {
		geo_ip_lookup(ipStr, &info->geoip_country, &info->geoip_ccode, &info->geoip_city, &info->geoip_coord);
	}
#endif

	/* show_info_fields() rejects a network without the lookup */
	if ((flags & FLAG_RESOLVE_HOST) && prefix == 32) {
		info->hostname = get_hostname(AF_INET, &ip);
		if (info->hostname == NULL) {
			if (!beSilent) {
				sprintf(errBuf,
					"ipcalc: cannot find hostname for %s",
					ipStr);
				herror(errBuf);
			}
			return -1;
		}
	}
	return 0;
}

int ipv6_prefix_to_mask(unsigned prefix, struct in6_addr *mask)
{
	struct in6_addr in6;
	int i, j;

	if (prefix > 128)
		return -1;

	memset(&in6, 0x0, sizeof(in6));
	for (i = prefix, j = 0; i > 0; i -= 8, j++) {
		if (i >= 8) {
			in6.s6_addr[j] = 0xff;
		} else {
			in6.s6_addr[j] = (unsigned long)(0xffU << (8 - i));
		}
	}

	memcpy(mask, &in6, sizeof(*mask));
	return 0;
}

static char *ipv6_mask_to_str(const struct in6_addr *mask)
{
	char buf[128];

	if (inet_ntop(AF_INET6, mask, buf, sizeof(buf)) == NULL)
		return NULL;

	return safe_strdup(buf);
}

/* Based on IANA's iana-ipv6-special-registry (updated 2025-10-09)
 * followed by the unicast and multicast blocks of ipv6-address-space.
 * As for IPv4, the most specific matching block wins and renamed blocks
 * keep the name ipcalc has always printed.
 */
static const struct addr_block ipv6_blocks[] = {
	{"::1", 128, "Loopback Address"},
	{"::", 128, "Unspecified Address"},
	{"::ffff:0:0", 96, "IPv4-mapped Address"},
	{"64:ff9b::", 96, "IPv4-IPv6 Translat."},
	{"64:ff9b:1::", 48, "IPv4-IPv6 Translat."},
	{"100::", 64, "Discard-Only Address Block"},
	{"100:0:0:1::", 64, "Dummy IPv6 Prefix"},
	{"2001::", 32, "TEREDO"},
	{"2001:1::1", 128, "Port Control Protocol Anycast"},
	{"2001:1::2", 128, "Traversal Using Relays around NAT Anycast"},
	{"2001:1::3", 128, "DNS-SD Service Registration Protocol Anycast"},
	{"2001:2::", 48, "Benchmarking"},
	{"2001:3::", 32, "AMT"},
	{"2001:4:112::", 48, "AS112-v6"},
	{"2001:10::", 28, "Deprecated (previously ORCHID)"},
	{"2001:20::", 28, "ORCHIDv2"},
	{"2001:30::", 28, "Drone Remote ID Protocol Entity Tags (DETs) Prefix"},
	{"2001::", 23, "IETF Protocol Assignments"},
	{"2001:db8::", 32, "Documentation"},
	{"2002::", 16, "6to4"},
	{"2620:4f:8000::", 48, "Direct Delegation AS112 Service"},
	{"3fff::", 20, "Documentation"},
	{"5f00::", 16, "Segment Routing (SRv6) SIDs"},
	{"2000::", 3, "Global Unicast"},
	{"fc00::", 7, "Unique Local Unicast"},
	{"fe80::", 10, "Link-Scoped Unicast"},
	{"ff00::", 8, "Multicast"},
};

static const char *ipv6_net_to_type(struct in6_addr *net, int prefix)
{
	const struct addr_block *best = NULL;
	struct in6_addr block, mask;
	unsigned i, j;

	for (i = 0; i < sizeof(ipv6_blocks) / sizeof(ipv6_blocks[0]); i++) {
		if ((unsigned)prefix < ipv6_blocks[i].prefix)
			continue;
		if (best && best->prefix >= ipv6_blocks[i].prefix)
			continue;
		if (inet_pton(AF_INET6, ipv6_blocks[i].net, &block) != 1 ||
		    ipv6_prefix_to_mask(ipv6_blocks[i].prefix, &mask) < 0)
			abort();
		for (j = 0; j < 16; j++)
			if ((net->s6_addr[j] & mask.s6_addr[j]) != block.s6_addr[j])
				break;
		if (j == 16)
			best = &ipv6_blocks[i];
	}

	return best ? best->name : "Reserved";
}

static
char *expand_ipv6(struct in6_addr *ip6)
{
	char buf[128];
	char *p;
	unsigned i;

	p = buf;
	for (i = 0; i < 16; i++) {
		sprintf(p, "%.2x", (unsigned)ip6->s6_addr[i]);
		p += 2;
		if (i % 2 != 0 && i != 15) {
			*p = ':';
			p++;
		}
	}
	*p = 0;

	return safe_strdup(buf);
}

static int iid_is_zero(const unsigned char *iid)
{
	unsigned i;

	for (i = 0; i < 8; i++)
		if (iid[i] != 0)
			return 0;
	return 1;
}

/* RFC 4291 Appendix A: a Modified EUI-64 built from a 48-bit MAC has
 * ff:fe inserted between the OUI and the NIC-specific octets.
 */
static int iid_is_mac_derived(const unsigned char *iid)
{
	return iid[3] == 0xff && iid[4] == 0xfe;
}

/* RFC 4291 section 2.5.1: unicast addresses outside ::/3 have a 64-bit
 * interface identifier. The u/g bits only carry meaning when the
 * identifier was derived from an IEEE MAC (RFC 7136).
 */
static void get_ipv6_iid_info(const struct in6_addr *ip6, ip_info_st *info)
{
	const unsigned char *iid = &ip6->s6_addr[8];
	unsigned char mac0;

	if ((ip6->s6_addr[0] & 0xe0) == 0 || ip6->s6_addr[0] == 0xff)
		return;

	/* The all-zero identifier is the Subnet-Router anycast address
	 * (RFC 4291 section 2.6.1), not a host's identifier.
	 */
	if (iid_is_zero(iid))
		return;

	safe_asprintf(&info->iid, "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
		      iid[0], iid[1], iid[2], iid[3],
		      iid[4], iid[5], iid[6], iid[7]);

	if (!iid_is_mac_derived(iid))
		return;

	/* Modified EUI-64 inverts the universal/local bit */
	mac0 = iid[0] ^ 0x02;
	safe_asprintf(&info->eui64, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		      mac0, iid[1], iid[2], iid[3],
		      iid[4], iid[5], iid[6], iid[7]);
	safe_asprintf(&info->mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		      mac0, iid[1], iid[2], iid[5], iid[6], iid[7]);
	info->mac_scope = (mac0 & 0x02) ? "local" : "universal";
	info->mac_type = (mac0 & 0x01) ? "group" : "individual";
}

int get_ipv6_info(const char *ipStr, int prefix, ip_info_st * info,
		  unsigned flags)
{
	struct in6_addr ip6, mask, network;
	char errBuf[250];
	unsigned i;

	memset(info, 0, sizeof(*info));

	if (inet_pton(AF_INET6, ipStr, &ip6) <= 0) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: bad IPv6 address: %s\n",
				ipStr);
		return -1;
	}

	/* expand  */
	info->expanded_ip = expand_ipv6(&ip6);

	if (inet_ntop(AF_INET6, &ip6, errBuf, sizeof(errBuf)) == 0) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: error calculating the IPv6 network\n");
		return -1;
	}

	info->ip = safe_strdup(errBuf);

	if (prefix > 128) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: bad IPv6 prefix: %d\n",
				prefix);
		return -1;
	} else if (prefix < 0) {
		prefix = 128;
	}

	info->prefix = prefix;

	if (ipv6_prefix_to_mask(prefix, &mask) == -1) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: error converting IPv6 prefix: %d\n",
				prefix);
		return -1;
	}

	info->netmask = ipv6_mask_to_str(&mask);

	for (i = 0; i < sizeof(struct in6_addr); i++)
		network.s6_addr[i] = ip6.s6_addr[i] & mask.s6_addr[i];

	if (inet_ntop(AF_INET6, &network, errBuf, sizeof(errBuf)) == 0) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: error calculating the IPv6 network\n");
		return -1;
	}

	info->network = safe_strdup(errBuf);

	info->expanded_network = expand_ipv6(&network);
	info->type = ipv6_net_to_type(&network, prefix);

	info->reverse_dns_count = calc_reverse_dns6(&network, prefix, info->reverse_dns);
	get_ipv6_iid_info(&ip6, info);

	if (prefix < 128) {
		info->hostmin = safe_strdup(errBuf);

		for (i = 0; i < sizeof(struct in6_addr); i++)
			network.s6_addr[i] |= ~mask.s6_addr[i];
		if (inet_ntop(AF_INET6, &network, errBuf, sizeof(errBuf)) == 0) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: error calculating the IPv6 network\n");
			return -1;
		}

		info->hostmax = safe_strdup(errBuf);
	} else {
		info->hostmin = info->network;
		info->hostmax = info->network;
	}

	ipv6_prefix_to_hosts(info->hosts, sizeof(info->hosts), prefix);

#ifdef USE_MAXMIND
	if (flags & FLAG_GET_GEOIP) {
		geo_ip_lookup(ipStr, &info->geoip_country, &info->geoip_ccode, &info->geoip_city, &info->geoip_coord);
	}
#endif

	/* show_info_fields() rejects a network without the lookup */
	if ((flags & FLAG_RESOLVE_HOST) && prefix == 128) {
		info->hostname = get_hostname(AF_INET6, &ip6);
		if (info->hostname == NULL) {
			if (!beSilent) {
				sprintf(errBuf,
					"ipcalc: cannot find hostname for %s",
					ipStr);
				herror(errBuf);
			}
			return -1;
		}
	}
	return 0;
}

static int randomize(void *ptr, unsigned size)
{
	int fd, ret;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return -1;

	ret = read(fd, ptr, size);
	close(fd);

	if (ret != size) {
		return -1;
	}

	return 0;
}

static char *generate_ip_network(unsigned prefix, unsigned flags)
{
	struct timespec ts;
	char ipbuf[64];
	char *p = NULL;

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) < 0)
		return NULL;

	if (flags & FLAG_IPV6) {
		struct in6_addr net;

		net.s6_addr[0] = 0xfd;
		if (randomize(&net.s6_addr[1], 15) < 0)
			return NULL;

		if (inet_ntop(AF_INET6, &net, ipbuf, sizeof(ipbuf)) == NULL)
			return NULL;
	} else {
		struct in_addr net;
		unsigned c = ts.tv_nsec % 4;
		uint8_t bytes[4];

		if (randomize(bytes, 4) < 0)
			return NULL;

		if (prefix >= 16 && c < 2) {
			if (c == 1) {
				bytes[0] = 192;
				bytes[1] = 168;
			} else {
				bytes[0] = 172;
				bytes[1] = 16 | ((ts.tv_nsec >> 4) & 0x0f);
			}
		} else {
			bytes[0] = 10;
		}

		memcpy(&net.s_addr, bytes, 4);

		if (inet_ntop(AF_INET, &net, ipbuf, sizeof(ipbuf)) == NULL)
			return NULL;
	}

	if (asprintf(&p, "%s/%u", ipbuf, prefix) == -1)
		return NULL;

	return p;
}

int str_to_prefix(unsigned *flags, const char *prefixStr, unsigned fix)
{
	int prefix = -1;
	if (!((*flags) & FLAG_IPV6) && strchr(prefixStr, '.')) {	/* prefix is 255.x.x.x */
		prefix = ipv4_mask_to_int(prefixStr);
	} else {
		int r = safe_atoi(prefixStr, &prefix);
		if (r != 0) {
			return -1;
		}
	}

	if (fix && (prefix > 32 && !((*flags) & FLAG_IPV6)))
		*flags |= FLAG_IPV6;

	if (prefix < 0 || ((((*flags) & FLAG_IPV6) && prefix > 128) || (!((*flags) & FLAG_IPV6) && prefix > 32))) {
		return -1;
	}
	return prefix;
}

/* [COUNT:]PREFIX, where PREFIX may also be a netmask */
static int parse_split_req(unsigned *flags, const char *str, struct split_req *req)
{
	const char *prefixStr = str;
	const char *sep = strchr(str, ':');
	int prefix;

	req->count = 0;
	if (sep) {
		char *end;

		errno = 0;
		/* strtoull() accepts leading spaces and a sign, and wraps
		 * "-1" to UINT64_MAX */
		if (!isdigit((unsigned char)str[0]))
			goto bad_count;
		req->count = strtoull(str, &end, 10);
		if (end != sep || errno != 0 || req->count == 0) {
 bad_count:
			if (!beSilent)
				fprintf(stderr, "ipcalc: bad split count: %s\n", str);
			return -1;
		}
		prefixStr = sep + 1;
	}

	prefix = str_to_prefix(flags, prefixStr, 1);
	if (prefix < 0) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: bad %s prefix: %s\n", ((*flags) & FLAG_IPV6)?"IPv6":"IPv4", str);
		return -1;
	}
	req->prefix = prefix;

	return 0;
}

static int parse_split_hosts(unsigned flags, const char *str,
			     struct split_req **reqs, unsigned *nreqs)
{
	unsigned width = (flags & FLAG_IPV6) ? 128 : 32;
	const char *p = str;

	while (1) {
		struct split_req *req;
		uint64_t hosts, size;
		unsigned bits = 0;
		char *end = NULL;

		errno = 0;
		/* see parse_split_req() */
		if (!isdigit((unsigned char)*p))
			goto bad_count;
		hosts = strtoull(p, &end, 10);
		if (errno != 0 || hosts == 0 || (*end != ',' && *end != 0)) {
 bad_count:
			if (!beSilent)
				fprintf(stderr, "ipcalc: bad host count: %s\n", str);
			return -1;
		}

		/* IPv4 subnets also need the network and broadcast addresses */
		size = hosts;
		if (!(flags & FLAG_IPV6)) {
			if (hosts > UINT32_MAX - 1) {
				if (!beSilent)
					fprintf(stderr, "ipcalc: too many hosts requested: %" PRIu64 "\n", hosts);
				return -1;
			}
			size += 2;
		}
		while (bits < 64 && (UINT64_C(1) << bits) < size)
			bits++;

		*reqs = realloc(*reqs, (*nreqs + 1) * sizeof((*reqs)[0]));
		if (*reqs == NULL)
			exit(exit_failure);
		req = &(*reqs)[(*nreqs)++];
		req->prefix = width - bits;
		req->count = 1;
		req->hosts = hosts;

		if (*end == 0)
			break;
		p = end + 1;
	}

	return 0;
}

#define OPT_ALLINFO 1
#define OPT_MINADDR 2
#define OPT_MAXADDR 3
#define OPT_ADDRESSES 4
#define OPT_ADDRSPACE 5
#define OPT_USAGE 6
#define OPT_REVERSE 7
#define OPT_CLASS_PREFIX 8
#define OPT_NO_DECORATE 9
#define OPT_HELP 10
#define OPT_SPLIT_HOSTS 11
#define OPT_EQUALS 12
#define OPT_SUBNET_OF 13
#define OPT_OVERLAPS 14
#define OPT_CIDR 15
#define OPT_WILDCARD 16
#define OPT_FORMAT 17

#define IS_COMPARE_OPT(c) ((c) == OPT_EQUALS || (c) == OPT_SUBNET_OF || (c) == OPT_OVERLAPS)

#ifdef USE_MAXMIND
# define GEO_SHORT_OPTION "g"
#else
# define GEO_SHORT_OPTION ""
#endif

static const struct option long_options[] = {
	{"check", 0, 0, 'c'},
	{"equals", 1, 0, OPT_EQUALS},
	{"subnet-of", 1, 0, OPT_SUBNET_OF},
	{"overlaps", 1, 0, OPT_OVERLAPS},
	{"random-private", 1, 0, 'r'},
	{"split", 1, 0, 'S'},
	{"split-hosts", 1, 0, OPT_SPLIT_HOSTS},
	{"deaggregate", 1, 0, 'd'},
	{"info", 0, 0, 'i'},
	{"all-info", 0, 0, OPT_ALLINFO},
	{"ipv4", 0, 0, '4'},
	{"ipv6", 0, 0, '6'},
	{"address", 0, 0, 'a'},
	{"broadcast", 0, 0, 'b'},
	{"hostname", 0, 0, 'h'},
	{"lookup-host", 1, 0, 'o'},
	{"reverse-dns", 0, 0, OPT_REVERSE},
#ifdef USE_MAXMIND
	{"geoinfo", 0, 0, 'g'},
#endif
	{"netmask", 0, 0, 'm'},
	{"network", 0, 0, 'n'},
	{"cidr", 0, 0, OPT_CIDR},
	{"wildcard", 0, 0, OPT_WILDCARD},
	{"prefix", 0, 0, 'p'},
	{"class-prefix", 0, 0, OPT_CLASS_PREFIX},
	{"minaddr", 0, 0, OPT_MINADDR},
	{"maxaddr", 0, 0, OPT_MAXADDR},
	{"addresses", 0, 0, OPT_ADDRESSES},
	{"addrspace", 0, 0, OPT_ADDRSPACE},
	{"silent", 0, 0, 's'},
	{"no-decorate", 0, 0, OPT_NO_DECORATE},
	{"json", 0, 0, 'j'},
	{"format", 1, 0, OPT_FORMAT},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, OPT_HELP},
	{"usage", 0, 0, OPT_USAGE},
	{NULL, 0, 0, 0}
};

/* Options that select a field to print */
static const struct {
	int option;
	enum ipcalc_flag field_flag;
} field_selecting_options[] = {
	{'a', FLAG_SHOW_ADDRESS},
	{'o', FLAG_RESOLVE_IP},
	{'b', FLAG_SHOW_BROADCAST},
	{'h', FLAG_RESOLVE_HOST},
	{'m', FLAG_SHOW_NETMASK},
	{'n', FLAG_SHOW_NETWORK},
	{'p', FLAG_SHOW_PREFIX},
#ifdef USE_MAXMIND
	{'g', FLAG_SHOW_GEOIP},
#endif
	{OPT_REVERSE, FLAG_SHOW_REVERSE},
	{OPT_CIDR, FLAG_SHOW_CIDR},
	{OPT_WILDCARD, FLAG_SHOW_WILDCARD},
	{OPT_MINADDR, FLAG_SHOW_MINADDR},
	{OPT_MAXADDR, FLAG_SHOW_MAXADDR},
	{OPT_ADDRESSES, FLAG_SHOW_ADDRESSES},
	{OPT_ADDRSPACE, FLAG_SHOW_ADDRSPACE},
};

/* 0 for an option that selects no field */
static unsigned field_flag_of_option(int option)
{
	const unsigned n = sizeof(field_selecting_options) /
			   sizeof(field_selecting_options[0]);
	unsigned i;

	for (i = 0; i < n; i++) {
		if (field_selecting_options[i].option == option)
			return field_selecting_options[i].field_flag;
	}
	return 0;
}

static
void usage(unsigned verbose)
{
	if (verbose) {
		fprintf(stderr, "Usage: ipcalc [OPTION...]\n");
		fprintf(stderr, "  -c, --check                     Validate IP address\n");
		fprintf(stderr, "      --equals=NET                Succeed if the network equals NET\n");
		fprintf(stderr, "      --subnet-of=NET             Succeed if the network is within NET\n");
		fprintf(stderr, "      --overlaps=NET              Succeed if the network shares an address\n");
		fprintf(stderr, "                                  with NET\n");
		fprintf(stderr, "  -r, --random-private=PREFIX     Generate a random private IP network using\n");
		fprintf(stderr, "                                  the supplied prefix or mask.\n");
		fprintf(stderr, "  -S, --split=[COUNT:]PREFIX      Split the provided network using the\n");
		fprintf(stderr, "                                  provided prefix/netmask; repeat with\n");
		fprintf(stderr, "                                  COUNT for subnets of different sizes\n");
		fprintf(stderr, "      --split-hosts=N1,N2,...     Split the provided network into subnets\n");
		fprintf(stderr, "                                  that hold N1, N2, ... hosts\n");
		fprintf(stderr, "  -d, --deaggregate=IP1-IP2       Deaggregate the provided address range\n");
		fprintf(stderr, "  -i, --info                      Print information on the provided IP address\n");
		fprintf(stderr, "                                  (default)\n");
		fprintf(stderr, "      --all-info                  Print verbose information on the provided IP\n");
		fprintf(stderr, "                                  address\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Specific info options:\n");
		fprintf(stderr, "      --reverse-dns               Print network in a the reverse DNS format\n");
		fprintf(stderr, "  -a, --address                   Display IP address\n");
		fprintf(stderr, "  -b, --broadcast                 Display calculated broadcast address\n");
		fprintf(stderr, "  -m, --netmask                   Display netmask for IP\n");
		fprintf(stderr, "      --wildcard                  Display wildcard (inverse) mask for IP\n");
		fprintf(stderr, "  -n, --network                   Display network address\n");
		fprintf(stderr, "      --cidr                      Display network address and prefix in\n");
		fprintf(stderr, "                                  CIDR notation\n");
		fprintf(stderr, "  -p, --prefix                    Display network prefix\n");
		fprintf(stderr, "      --minaddr                   Display the minimum address in the network\n");
		fprintf(stderr, "      --maxaddr                   Display the maximum address in the network\n");
		fprintf(stderr, "      --addresses                 Display the maximum number of addresses in\n");
		fprintf(stderr, "                                  the network\n");
		fprintf(stderr, "      --addrspace                 Display the address space the network\n");
		fprintf(stderr, "                                  resides on\n");
		fprintf(stderr, "  -h, --hostname                  Show hostname determined via DNS\n");
		fprintf(stderr, "  -o, --lookup-host=STRING        Show IP as determined via DNS\n");
#ifdef USE_MAXMIND
		fprintf(stderr, "  -g, --geoinfo                   Show Geographic information about the\n");
		fprintf(stderr, "                                  provided IP\n");
#endif
		fprintf(stderr, "\n");
		fprintf(stderr, "Output format:\n");
		fprintf(stderr, "      --format=FORMAT             Print the output as human (default for the\n");
		fprintf(stderr, "                                  summary), shell (NAME=value, default for\n");
		fprintf(stderr, "                                  specific info options), json, or value\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Other options:\n");
		fprintf(stderr, "  -4, --ipv4                      Explicitly specify the IPv4 address family\n");
		fprintf(stderr, "  -6, --ipv6                      Explicitly specify the IPv6 address family\n");
		fprintf(stderr, "      --class-prefix              When specified the default prefix will be determined\n");
		fprintf(stderr, "                                  by the IPv4 address class\n");
		fprintf(stderr, "  -s, --silent                    Don't ever display error messages, nor\n");
		fprintf(stderr, "                                  the result of a comparison\n");
		fprintf(stderr, "  -v, --version                   Display program version\n");
		fprintf(stderr, "  -?, --help                      Show this help message\n");
		fprintf(stderr, "      --usage                     Display brief usage message\n");
	} else {
		fprintf(stderr, "Usage: ipcalc [-46sv?] [-c|--check] [--equals=NET] [--subnet-of=NET]\n");
		fprintf(stderr, "        [--overlaps=NET] [-r|--random-private=STRING] [-i|--info]\n");
		fprintf(stderr, "        [--all-info] [-4|--ipv4] [-6|--ipv6] [-a|--address] [-b|--broadcast]\n");
#ifdef USE_MAXMIND
		fprintf(stderr, "        [-h|--hostname] [-o|--lookup-host=STRING] [-g|--geoinfo]\n");
#else
		fprintf(stderr, "        [-h|--hostname] [-o|--lookup-host=STRING]\n");
#endif
		fprintf(stderr, "        [-m|--netmask] [--wildcard] [-n|--network] [--cidr] [-p|--prefix]\n");
		fprintf(stderr, "        [--minaddr] [--maxaddr] [--addresses] [--addrspace]\n");
		fprintf(stderr, "        [--format=FORMAT]\n");
		fprintf(stderr, "        [-s|--silent] [-v|--version] [--reverse-dns] [--class-prefix]\n");
		fprintf(stderr, "        [-?|--help] [--usage]\n");
	}
}

void output_start(unsigned * const jsonfirst)
{
	if (flags & FLAG_JSON) {
		printf("{\n");
	}

	*jsonfirst = JSON_FIRST;
}

void output_separate(unsigned * const jsonfirst)
{
	if (!(flags & FLAG_JSON)) {
		printf("\n");
	}
}

void output_stop(unsigned * const jsonfirst)
{
	if (flags & FLAG_JSON) {
		printf("\n}\n");
	}
}

void array_start(unsigned * const jsonfirst, const char *head, const char *json_head)
{
	if (flags & FLAG_JSON) {
		if (*jsonfirst == JSON_NEXT) {
			printf(",\n");
		}

		printf("  \"%s\":[\n  ", json_head);
	} else {
		if (!(flags & FLAG_NO_DECORATE))
			printf("[%s]\n", head);
	}

	*jsonfirst = JSON_ARRAY_FIRST;
}

void array_stop(unsigned * const jsonfirst)
{
	if (flags & FLAG_JSON) {
		printf("]");
		*jsonfirst = JSON_NEXT;
	}
}

void
__attribute__ ((format(printf, 3, 4)))
color_printf(const char *color, const char *title, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	va_color_printf(color, title, fmt, args);
	va_end(args);

	return;
}

void va_color_printf(const char *color, const char *title, const char *fmt, va_list varglist)
{
	int ret;
	char *str = NULL;

	ret = vasprintf(&str, fmt, varglist);

	if (ret < 0) {
		return;
	}

	fputs(title, stdout);
	if (colors) {
		fputs(color, stdout);
	}
	fputs(str, stdout);
	fputs("\n", stdout);
	if (colors) {
		fputs(KRESET, stdout);
	}
	free(str);
	return;
}

void
__attribute__ ((format(printf, 3, 4)))
json_printf(unsigned * const jsonfirst, const char *jsontitle, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	va_json_printf(jsonfirst, jsontitle, fmt, args);
	va_end(args);

	return;
}

/* Host names and geo values come from outside ipcalc and may contain
 * characters that are not allowed unescaped in a JSON string (RFC 8259).
 */
static void json_print_string(const char *str)
{
	const unsigned char *p;

	putchar('"');
	for (p = (const unsigned char *)str; *p; p++) {
		if (*p == '"' || *p == '\\')
			printf("\\%c", *p);
		else if (*p < 0x20)
			printf("\\u%04x", *p);
		else
			putchar(*p);
	}
	putchar('"');
}

void va_json_printf(unsigned * const jsonfirst, const char *jsontitle, const char *fmt, va_list varglist)
{
	int ret;
	char *str = NULL;

	ret = vasprintf(&str, fmt, varglist);
	if (ret < 0)
		return;

	if (*jsonfirst == JSON_ARRAY_NEXT) {
		fprintf(stdout, ",\n  ");
	} else if (*jsonfirst == JSON_NEXT) {
		fprintf(stdout, ",\n");
	}

	fprintf(stdout, "  ");
	if (jsontitle)
		fprintf(stdout, "\"%s\":", jsontitle);
	json_print_string(str);
	if (*jsonfirst == JSON_FIRST)
		*jsonfirst = JSON_NEXT;
	else if (*jsonfirst == JSON_ARRAY_FIRST)
		*jsonfirst = JSON_ARRAY_NEXT;

	free(str);
	return;
}

/* Always prints "title: value", will not color if --no-decorate is given
 */
static void
__attribute__ ((format(printf, 2, 3)))
pretty_printf(const char *title, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (flags & FLAG_NO_DECORATE) {
		fputs(title, stdout);
		vprintf(fmt, args);
		fputs("\n", stdout);
	} else {
		va_color_printf(KBLUE, title, fmt, args);
	}
	va_end(args);

	return;
}

/* Always prints "title: value", will not color if --no-decorate is given
 * To be used for distinct values (e.g., a summary).
 */
static void
__attribute__ ((format(printf, 2, 3)))
pretty_dist_printf(const char *title, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (flags & FLAG_NO_DECORATE) {
		fputs(title, stdout);
		vprintf(fmt, args);
		fputs("\n", stdout);
	} else {
		va_color_printf(KMAG, title, fmt, args);
	}
	va_end(args);

	return;
}

/* Prints "title: value", will only print value if --no-decorate is given
 */
void
__attribute__ ((format(printf, 4, 5)))
default_printf(unsigned * const jsonfirst, const char *title, const char *jsontitle, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (flags & FLAG_JSON) {
		va_json_printf(jsonfirst, jsontitle, fmt, args);
	} else if (flags & FLAG_NO_DECORATE) {
		vprintf(fmt, args);
		printf("\n");
	} else {
		va_color_printf(KBLUE, title, fmt, args);
	}
	va_end(args);

	return;
}

/* Prints "title: value", will only print value if --no-decorate is given. It
 * prints a distinct value (e.g., to be used for a summary).
 */
void
__attribute__ ((format(printf, 4, 5)))
dist_printf(unsigned * const jsonfirst, const char *title, const char *jsontitle, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (flags & FLAG_JSON) {
		va_json_printf(jsonfirst, jsontitle, fmt, args);
	}
	else if (flags & FLAG_NO_DECORATE) {
		vprintf(fmt, args);
		printf("\n");
	} else {
		va_color_printf(KMAG, title, fmt, args);
	}
	va_end(args);

	return;
}

/* Printed variables names */
#define HOSTNAME_NAME "HOSTNAME"
#define FULL_ADDRESS_NAME "FULLADDRESS"
#define ADDRESS_NAME "ADDRESS"
#define FULL_NETWORK_NAME "FULLNETWORK"
#define NETWORK_NAME "NETWORK"
#define NETMASK_NAME "NETMASK"
#define WILDCARD_NAME "WILDCARD"
#define PREFIX_NAME "PREFIX"
#define CIDR_NAME "CIDR"
#define BROADCAST_NAME "BROADCAST"
#define REVERSEDNS_NAME "REVERSEDNS"
#define ADDRSPACE_NAME "ADDRSPACE"
#define ADDRCLASS_NAME "ADDRCLASS"
#define INTERFACEID_NAME "INTERFACEID"
#define EUI64_NAME "EUI64"
#define MACADDR_NAME "MACADDR"
#define MACSCOPE_NAME "MACSCOPE"
#define MACTYPE_NAME "MACTYPE"
#define MINADDR_NAME "MINADDR"
#define MAXADDR_NAME "MAXADDR"
#define ADDRESSES_NAME "ADDRESSES"
#define COUNTRYCODE_NAME "COUNTRYCODE"
#define COUNTRY_NAME "COUNTRY"
#define CITY_NAME "CITY"
#define COORDINATES_NAME "COORDINATES"

enum output_format {
	FORMAT_HUMAN,	/* titled lines, colored on a terminal */
	FORMAT_SHELL,	/* NAME=value */
	FORMAT_JSON,
	FORMAT_VALUE	/* values only */
};

static const char * const format_names[] = {
	[FORMAT_HUMAN] = "human",
	[FORMAT_SHELL] = "shell",
	[FORMAT_JSON] = "json",
	[FORMAT_VALUE] = "value",
};

/* -1, after reporting it, for an unknown format name */
static int parse_format(const char *name)
{
	unsigned i;

	for (i = 0; i < sizeof(format_names) / sizeof(format_names[0]); i++) {
		if (strcmp(name, format_names[i]) == 0)
			return i;
	}
	if (!beSilent)
		fprintf(stderr, "ipcalc: unknown output format: %s; use human, shell, json or value\n",
			name);
	return -1;
}

/* How show_info_fields() prints a field; the option that selects a field is
 * a FLAG_SHOW_* (or FLAG_RESOLVE_*) of enum ipcalc_flag */
enum field_attr {
	/* Without an ATTR_SUMMARY_* attribute a field is in the summary only when its option is given */
	ATTR_SUMMARY_ALWAYS=1<<0,
	ATTR_SUMMARY_WITH_ALL_INFO=1<<1,
	/* the text summary already shows it as part of another field */
	ATTR_SUMMARY_JSON_ONLY=1<<2,
	/* the input address is meaningless for a network or a random network */
	ATTR_SUMMARY_HIDE_FOR_NETWORK_ADDRESS=1<<3,
	/* hidden in the text summary only, where it would repeat the single address */
	ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS=1<<4,
	/* printed in a different color than other values */
	ATTR_SUMMARY_HIGHLIGHT=1<<5,
	ATTR_VAR_QUOTE_ALWAYS=1<<6,
	/* values-only output quotes as released, since scripts read it verbatim */
	ATTR_VALUE_QUOTE_IF_SPACE=1<<7,
	ATTR_REQUIRES_SINGLE_ADDRESS=1<<8
};

enum summary_section { SECTION_NETWORK, SECTION_RANGE, SECTION_GEO };

/* A field without a value for the input's address family is left NULL by
 * get_ipv4_info() or get_ipv6_info(), so it is never printed */
struct output_field {
	const char *title;
	const char *name;
	enum ipcalc_flag option;
	const char *option_name;	/* named in diagnostics */
	enum field_attr attrs;
	enum summary_section section;
	const char *value;
	const char *text_value;		/* the value in the text summary, when it differs */
	char * const *values;		/* instead of value, for a field with several values */
	unsigned nvalues;
};

static unsigned field_has_value(const struct output_field *f)
{
	return f->value != NULL || f->nvalues > 0;
}

static unsigned field_in_summary(const struct output_field *f,
				 enum output_format format,
				 unsigned single_address,
				 unsigned input_is_network)
{
	if (!field_has_value(f))
		return 0;
	if ((f->attrs & ATTR_SUMMARY_HIDE_FOR_NETWORK_ADDRESS) && input_is_network)
		return 0;
	if (format == FORMAT_HUMAN &&
	    (((f->attrs & ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS) && single_address) || (f->attrs & ATTR_SUMMARY_JSON_ONLY)))
		return 0;
	if (flags & f->option)
		return 1;
	if (f->attrs & (ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_JSON_ONLY))
		return 1;
	return (f->attrs & ATTR_SUMMARY_WITH_ALL_INFO) && (flags & FLAG_SHOW_ALL_INFO);
}

static void show_json_field(unsigned * const jsonchain, const struct output_field *f)
{
	unsigned i;

	if (!f->values) {
		json_printf(jsonchain, f->name, "%s", f->value);
		return;
	}

	/* always an array, so that its type does not depend on the number of values */
	array_start(jsonchain, f->title, f->name);
	for (i = 0; i < f->nvalues; i++)
		json_printf(jsonchain, NULL, "%s", f->values[i]);
	array_stop(jsonchain);
}

static void show_text_field(const struct output_field *f)
{
	const char *value = f->text_value ? f->text_value : f->value;
	unsigned i;

	if (f->values) {
		for (i = 0; i < f->nvalues; i++)
			pretty_printf(f->title, "%s", f->values[i]);
	} else if (f->attrs & ATTR_SUMMARY_HIGHLIGHT) {
		pretty_dist_printf(f->title, "%s", value);
	} else {
		pretty_printf(f->title, "%s", value);
	}
}

/* Characters that the shell takes literally in an unquoted word; bytes of
 * UTF-8 sequences are among them */
static unsigned shell_safe(const char *value)
{
	const unsigned char *c;

	for (c = (const unsigned char *)value; *c; c++) {
		if (*c >= 0x80 || (*c >= 'a' && *c <= 'z') ||
		    (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9'))
			continue;
		if (strchr("._:/-,+@%", *c) == NULL)
			return 0;
	}
	return 1;
}

/* Inside double quotes the shell interprets only these characters */
static void print_shell_escaped(const char *value)
{
	for (; *value; value++) {
		if (strchr("\"\\$`", *value) != NULL)
			putchar('\\');
		putchar(*value);
	}
}

/* NAME=value output must be usable with eval, whatever the value holds */
static void print_shell_value(const char *value, unsigned quote)
{
	if (!quote && shell_safe(value)) {
		fputs(value, stdout);
		return;
	}
	putchar('"');
	print_shell_escaped(value);
	putchar('"');
}

static void show_var_field(const struct output_field *f, unsigned bare)
{
	unsigned i;

	if (f->values) {
		if (bare) {
			/* one value per line, as with --deaggregate */
			for (i = 0; i < f->nvalues; i++)
				printf("%s\n", f->values[i]);
		} else if (f->nvalues == 1) {
			printf("%s=", f->name);
			print_shell_value(f->values[0], 0);
			putchar('\n');
		} else {
			/* a quoted, space-separated list */
			printf("%s=\"", f->name);
			for (i = 0; i < f->nvalues; i++) {
				if (i > 0)
					putchar(' ');
				print_shell_escaped(f->values[i]);
			}
			printf("\"\n");
		}
		return;
	}

	if (bare) {
		if ((f->attrs & ATTR_VAR_QUOTE_ALWAYS) ||
		    ((f->attrs & ATTR_VALUE_QUOTE_IF_SPACE) &&
		     strchr(f->value, ' ') != NULL))
			printf("\"%s\"\n", f->value);
		else
			printf("%s\n", f->value);
		return;
	}

	printf("%s=", f->name);
	print_shell_value(f->value, f->attrs & ATTR_VAR_QUOTE_ALWAYS);
	putchar('\n');
}

static unsigned field_selected(const struct output_field *f,
			       enum output_format format,
			       unsigned single_address,
			       unsigned input_is_network)
{
	if (flags & FLAG_SHOW_MODERN_INFO)
		return field_in_summary(f, format, single_address,
					input_is_network);
	return field_has_value(f) && (flags & f->option);
}

static void show_field(unsigned * const jsonchain, const struct output_field *f,
		       enum output_format format)
{
	switch (format) {
	case FORMAT_HUMAN:
		show_text_field(f);
		break;
	case FORMAT_SHELL:
		show_var_field(f, 0);
		break;
	case FORMAT_JSON:
		show_json_field(jsonchain, f);
		break;
	case FORMAT_VALUE:
		show_var_field(f, 1);
		break;
	}
}

/* Prints the summary or, when specific fields are selected, those fields;
 * both in the order of fields[] and in the given format.
 */
static void show_info_fields(const ip_info_st *info, enum output_format format,
			     unsigned randomized)
{
	unsigned human_summary = (flags & FLAG_SHOW_MODERN_INFO) &&
				 format == FORMAT_HUMAN;
	unsigned single_address = info->prefix == ((flags & FLAG_IPV6) ? 128 : 32);
	unsigned input_is_network = !single_address &&
				    (randomized || strcmp(info->network, info->ip) == 0);
	char prefix[8];
	char *cidr = NULL, *full_network = NULL, *netmask = NULL, *hosts = NULL;
	unsigned jsonchain = JSON_FIRST;
	enum summary_section section = SECTION_NETWORK;
	unsigned i;

	snprintf(prefix, sizeof(prefix), "%u", info->prefix);
	safe_asprintf(&cidr, "%s/%u", info->network, info->prefix);
	safe_asprintf(&netmask, "%s = %u", info->netmask, info->prefix);
	if (info->expanded_network)
		safe_asprintf(&full_network, "%s/%u", info->expanded_network, info->prefix);
	if ((flags & FLAG_IPV6) && info->prefix < 112)
		safe_asprintf(&hosts, "2^(%u) = %s", 128 - info->prefix, info->hosts);

	const struct output_field fields[] = {
		{ .title = "Full Address:\t", .name = FULL_ADDRESS_NAME, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_NETWORK_ADDRESS, .value = info->expanded_ip },
		{ .title = "Address:\t", .name = ADDRESS_NAME, .option = FLAG_SHOW_ADDRESS|FLAG_RESOLVE_IP, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_NETWORK_ADDRESS, .value = info->ip },
		{ .title = "Hostname:\t", .name = HOSTNAME_NAME, .option = FLAG_RESOLVE_HOST, .option_name = "-h", .attrs = ATTR_SUMMARY_ALWAYS|ATTR_REQUIRES_SINGLE_ADDRESS, .value = info->hostname },
		{ .title = "Full Network:\t", .name = FULL_NETWORK_NAME, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .value = info->expanded_network, .text_value = full_network },
		{ .title = "Network:\t", .name = NETWORK_NAME, .option = FLAG_SHOW_NETWORK, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .value = info->network, .text_value = cidr },
		{ .title = "Netmask:\t", .name = NETMASK_NAME, .option = FLAG_SHOW_NETMASK, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .value = info->netmask, .text_value = netmask },
		{ .title = "Wildcard:\t", .name = WILDCARD_NAME, .option = FLAG_SHOW_WILDCARD, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .value = info->wildcard },
		{ .title = "Prefix:\t", .name = PREFIX_NAME, .option = FLAG_SHOW_PREFIX, .attrs = ATTR_SUMMARY_JSON_ONLY, .value = prefix },
		{ .title = "CIDR:\t", .name = CIDR_NAME, .option = FLAG_SHOW_CIDR, .attrs = ATTR_SUMMARY_JSON_ONLY, .value = cidr },
		{ .title = "Broadcast:\t", .name = BROADCAST_NAME, .option = FLAG_SHOW_BROADCAST, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .value = info->broadcast },
		{ .title = "Reverse DNS:\t", .name = REVERSEDNS_NAME, .option = FLAG_SHOW_REVERSE, .attrs = ATTR_SUMMARY_WITH_ALL_INFO, .values = info->reverse_dns, .nvalues = info->reverse_dns_count },
		{ .title = "Address space:\t", .name = ADDRSPACE_NAME, .option = FLAG_SHOW_ADDRSPACE, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIGHLIGHT|ATTR_VALUE_QUOTE_IF_SPACE, .section = SECTION_RANGE, .value = info->type },
		{ .title = "Address class:\t", .name = ADDRCLASS_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->class },
		{ .title = "Interface ID:\t", .name = INTERFACEID_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->iid },
		{ .title = "EUI-64:\t\t", .name = EUI64_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->eui64 },
		{ .title = "MAC address:\t", .name = MACADDR_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->mac },
		{ .title = "MAC scope:\t", .name = MACSCOPE_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->mac_scope },
		{ .title = "MAC type:\t", .name = MACTYPE_NAME, .attrs = ATTR_SUMMARY_WITH_ALL_INFO|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_RANGE, .value = info->mac_type },
		{ .title = "HostMin:\t", .name = MINADDR_NAME, .option = FLAG_SHOW_MINADDR, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .section = SECTION_RANGE, .value = info->hostmin },
		{ .title = "HostMax:\t", .name = MAXADDR_NAME, .option = FLAG_SHOW_MAXADDR, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS, .section = SECTION_RANGE, .value = info->hostmax },
		{ .title = "Hosts/Net:\t", .name = ADDRESSES_NAME, .option = FLAG_SHOW_ADDRESSES, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIDE_FOR_SINGLE_ADDRESS|ATTR_VALUE_QUOTE_IF_SPACE, .section = SECTION_RANGE, .value = info->hosts[0] ? info->hosts : NULL, .text_value = hosts },
		/* geo values exist only when looked up, with -g or --all-info */
		{ .title = "Country code:\t", .name = COUNTRYCODE_NAME, .option = FLAG_SHOW_GEOIP, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIGHLIGHT, .section = SECTION_GEO, .value = info->geoip_ccode },
		{ .title = "Country:\t", .name = COUNTRY_NAME, .option = FLAG_SHOW_GEOIP, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIGHLIGHT|ATTR_VALUE_QUOTE_IF_SPACE, .section = SECTION_GEO, .value = info->geoip_country },
		{ .title = "City:\t\t", .name = CITY_NAME, .option = FLAG_SHOW_GEOIP, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIGHLIGHT|ATTR_VALUE_QUOTE_IF_SPACE, .section = SECTION_GEO, .value = info->geoip_city },
		{ .title = "Coordinates:\t", .name = COORDINATES_NAME, .option = FLAG_SHOW_GEOIP, .attrs = ATTR_SUMMARY_ALWAYS|ATTR_SUMMARY_HIGHLIGHT|ATTR_VAR_QUOTE_ALWAYS, .section = SECTION_GEO, .value = info->geoip_coord },
	};
	const unsigned nfields = sizeof(fields) / sizeof(fields[0]);
	const struct output_field *f;

	for (i = 0; i < nfields; i++) {
		f = &fields[i];
		if ((f->attrs & ATTR_REQUIRES_SINGLE_ADDRESS) && (flags & f->option) && !single_address) {
			if (!beSilent)
				fprintf(stderr, "ipcalc: %s requires a single address, not a network: %s/%u\n",
					f->option_name, info->ip, info->prefix);
			exit(exit_failure);
		}
	}

	output_start(&jsonchain);
	for (i = 0; i < nfields; i++) {
		f = &fields[i];
		if (!field_selected(f, format, single_address,
				    input_is_network))
			continue;

		if (human_summary && f->section != section) {
			/* a single address has no network fields to separate
			 * from its range fields */
			if (f->section == SECTION_GEO || !single_address)
				output_separate(&jsonchain);
			section = f->section;
		}

		show_field(&jsonchain, f, format);
	}
	output_stop(&jsonchain);

	free(cidr);
	free(full_network);
	free(netmask);
	free(hosts);
}

/* The format implied by -j, --no-decorate and the selection, when no
 * --format is given */
static enum output_format legacy_format(void)
{
	if (flags & FLAG_JSON)
		return FORMAT_JSON;
	if (flags & FLAG_SHOW_MODERN_INFO)
		return FORMAT_HUMAN;	/* uncolored with --no-decorate */
	if (flags & FLAG_NO_DECORATE)
		return FORMAT_VALUE;
	return FORMAT_SHELL;
}

static int format_conflict(const char *option, enum output_format format)
{
	if (!beSilent)
		fprintf(stderr, "ipcalc: conflicting output formats: %s and --format=%s\n",
			option, format_names[format]);
	return -1;
}

/* Applies --format=name, given again as other_name when that differs, and
 * makes the flags that the other modes read agree with it */
static int select_format(const char *name, const char *other_name,
			 unsigned *selected_fields, enum output_format *format)
{
	int f = parse_format(name);

	if (f < 0)
		return -1;
	if (other_name) {
		if (parse_format(other_name) < 0)
			return -1;
		if (!beSilent)
			fprintf(stderr, "ipcalc: conflicting output formats: --format=%s and --format=%s\n",
				name, other_name);
		return -1;
	}
	if ((flags & FLAG_JSON) && f != FORMAT_JSON)
		return format_conflict("-j", f);
	if ((flags & FLAG_NO_DECORATE) && f != FORMAT_VALUE)
		return format_conflict("--no-decorate", f);

	if (flags & FLAG_SHOW_MODERN_INFO) {
		/* with -i or --all-info, -o only supplies the address */
		*selected_fields &= ~FLAG_RESOLVE_IP;
		if (*selected_fields) {
			if (!beSilent)
				fprintf(stderr, "ipcalc: -i and --all-info cannot be combined with options that select fields\n");
			return -1;
		}
	}

	if (f == FORMAT_JSON)
		flags |= FLAG_JSON;
	else if (f == FORMAT_VALUE)
		flags |= FLAG_NO_DECORATE;
	*format = f;
	return 0;
}

static int check_format_for_app(enum output_format format, enum app_t app)
{
	const char *unsupported = NULL;

	if ((app & (APP_SPLIT | APP_DEAGGREGATE)) && format == FORMAT_SHELL)
		unsupported = "--split, --split-hosts or --deaggregate; use --format=value";
	else if ((app & APP_COMPARE) && format != FORMAT_HUMAN)
		unsupported = "--equals, --subnet-of or --overlaps";

	if (unsupported) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: --format=%s is not supported with %s\n",
				format_names[format], unsupported);
		return -1;
	}

	/* info mode is the default, so app may have no bit set */
	if ((app & ~APP_SHOW_INFO) == 0 && format == FORMAT_VALUE &&
	    (flags & FLAG_SHOW_MODERN_INFO)) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: --format=value needs options that select fields; use --format=shell or --format=json for the summary\n");
		return -1;
	}
	return 0;
}

/* Only on a terminal, so that the output of scripts does not change */
static void suggest_format(enum app_t app, unsigned selected_fields)
{
	const char *option, *replacement;

	if (beSilent || !isatty(STDERR_FILENO))
		return;

	if (flags & FLAG_JSON) {
		option = "-j";
		if (app & APP_COMPARE)
			replacement = "-s to print no result";
		else if (selected_fields)
			replacement = "--format=json, which prints only the selected fields";
		else
			replacement = "--format=json";
	} else if (flags & FLAG_NO_DECORATE) {
		option = "--no-decorate";
		if (app & APP_COMPARE)
			replacement = "-s to print no result";
		else if (selected_fields || (app & (APP_SPLIT | APP_DEAGGREGATE)))
			replacement = "--format=value";
		else
			replacement = "--format=human";
	} else {
		return;
	}

	fprintf(stderr, "ipcalc: hint: %s is deprecated; use %s\n",
		option, replacement);
}

/*!
  \fn main(int argc, const char **argv)
  \brief wrapper program for ipcalc functions.

  This is a wrapper program for the functions that the ipcalc library provides.
  It can be used from shell scripts or directly from the command line.

  For more information, please see the ipcalc(1) man page.
*/
int main(int argc, char **argv)
{
	char *randomStr = NULL;
	char *hostname = NULL;
	char **splitStrs = NULL;
	unsigned nsplits = 0;
	char **splitHostsStrs = NULL;
	unsigned nsplitHosts = 0;
	char *ipStr = NULL, *prefixStr = NULL, *chptr = NULL;
	int prefix = -1;
	ip_info_st info;
	int r = 0;
	enum app_t app = 0;
	char *cmpStr = NULL;
	enum net_comparison cmpOp = CMP_EQUALS;
	unsigned ncmps = 0;
	unsigned userFlags;
	unsigned selected_fields = 0;
	enum output_format format;
	const char *formatStr = NULL, *otherFormatStr = NULL;
	int badOption = 0;

	while (1) {
		int c = getopt_long(argc, argv, "S:cr:i46abho:" GEO_SHORT_OPTION "mnpjsvd:", long_options, NULL);
		if (c == -1)
			break;

		switch(c) {
			case 'c':
				app |= APP_CHECK_ADDRESS;
				break;
			case OPT_EQUALS:
			case OPT_SUBNET_OF:
			case OPT_OVERLAPS:
				app |= APP_COMPARE;
				cmpOp = c == OPT_EQUALS ? CMP_EQUALS :
					c == OPT_SUBNET_OF ? CMP_SUBNET_OF : CMP_OVERLAPS;
				ncmps++;
				exit_failure = 2;
				cmpStr = safe_strdup(optarg);
				if (cmpStr == NULL) exit(exit_failure);
				break;
			case 'S':
				app |= APP_SPLIT;
				splitStrs = realloc(splitStrs, (nsplits + 1) * sizeof(splitStrs[0]));
				if (splitStrs == NULL) exit(exit_failure);
				splitStrs[nsplits] = safe_strdup(optarg);
				if (splitStrs[nsplits] == NULL) exit(exit_failure);
				nsplits++;
				break;
			case OPT_SPLIT_HOSTS:
				app |= APP_SPLIT;
				splitHostsStrs = realloc(splitHostsStrs, (nsplitHosts + 1) * sizeof(splitHostsStrs[0]));
				if (splitHostsStrs == NULL) exit(exit_failure);
				splitHostsStrs[nsplitHosts] = safe_strdup(optarg);
				if (splitHostsStrs[nsplitHosts] == NULL) exit(exit_failure);
				nsplitHosts++;
				break;
			case 'd':
				app |= APP_DEAGGREGATE;
				ipStr = safe_strdup(optarg);
				if (ipStr == NULL) exit(exit_failure);
				break;
			case 'r':
				app |= APP_SHOW_INFO;
				flags |= FLAG_RANDOM;
				randomStr = safe_strdup(optarg);
				if (randomStr == NULL) exit(exit_failure);
				break;
			case 'i':
				app |= APP_SHOW_INFO;
				flags |= FLAG_SHOW_MODERN_INFO;
				break;
			case OPT_ALLINFO:
				app |= APP_SHOW_INFO;
				flags |= FLAG_SHOW_ALL_INFO|FLAG_SHOW_MODERN_INFO;
				break;
			case OPT_CLASS_PREFIX:
				flags |= FLAG_ASSUME_CLASS_PREFIX;
				break;
			case '4':
				flags |= FLAG_IPV4;
				break;
			case '6':
				flags |= FLAG_IPV6;
				break;
			case 'o':
				selected_fields |= field_flag_of_option(c);
				hostname = safe_strdup(optarg);
				if (hostname == NULL) exit(exit_failure);
				break;
			case OPT_NO_DECORATE:
				flags |= FLAG_NO_DECORATE;
				break;
			case 'j':
				flags |= FLAG_JSON;
				break;
			case OPT_FORMAT:
				if (formatStr == NULL)
					formatStr = optarg;
				else if (strcmp(formatStr, optarg) != 0 &&
					 otherFormatStr == NULL)
					otherFormatStr = optarg;
				break;
			case 's':
				beSilent = 1;
				break;
			case 'v':
				app |= APP_VERSION;
				break;
			/* after an invalid option, help must not turn the
			 * error into a success */
			case OPT_USAGE:
				if (badOption)
					break;
				usage(0);
				exit(0);
			case OPT_HELP:
				if (badOption)
					break;
				usage(1);
				exit(0);
			case '?':
				/* -? is not in the short options, so getopt reports
				 * it like an unknown option */
				if (optopt == '?' && !badOption) {
					usage(1);
					exit(0);
				}
				if (IS_COMPARE_OPT(optopt))
					exit_failure = 2;
				/* keep parsing, since a later comparison option
				 * changes the exit status; report only this error */
				badOption = 1;
				opterr = 0;
				break;
			default:
				selected_fields |= field_flag_of_option(c);
				break;
		}
	}

	if (selected_fields)
		app |= APP_SHOW_INFO;

	if (badOption) {
		usage(1);
		exit(exit_failure);
	}

	if (optind < argc) {
		if (ipStr != NULL) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: superfluous option given\n");
			exit(exit_failure);
		}

		ipStr = argv[optind++];
		if (optind < argc)
			chptr = argv[optind++];
	}

	if (formatStr) {
		if (select_format(formatStr, otherFormatStr, &selected_fields,
				  &format) < 0)
			return exit_failure;
	} else if ((flags & FLAG_JSON) && (flags & FLAG_NO_DECORATE)) {
		flags &= ~FLAG_NO_DECORATE;
	}

	/* without --format, JSON output is always the summary, to which
	 * selected fields are added */
	if (!selected_fields || (!formatStr && (flags & FLAG_JSON)))
		flags |= FLAG_SHOW_MODERN_INFO;
	flags |= selected_fields;

	if (!formatStr)
		format = legacy_format();

	if (geo_setup() == 0 && (flags & FLAG_SHOW_ALL_INFO))
		flags |= FLAG_GET_GEOIP;

	if (bit_count(app) > 1 || ncmps > 1) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: you cannot mix these options\n");
		return exit_failure;
	}

	if (formatStr && check_format_for_app(format, app) < 0)
		return exit_failure;
	if (!formatStr)
		suggest_format(app, selected_fields);

	if ((flags & FLAG_IPV6) && (flags & FLAG_IPV4)) {
		if (!beSilent)
			fprintf(stderr,
				"ipcalc: you cannot specify both IPv4 and IPv6\n");
		return exit_failure;
	}

	userFlags = flags;

	/* if there is a : in the address, it is an IPv6 address.
	 * Note that we allow -4, and -6 to be given explicitly, so
	 * that the tool can be used to check for a valid IPv4 or IPv6
	 * address.
	 */
	if ((flags & FLAG_IPV4) == 0 && ipStr && strchr(ipStr, ':') != NULL) {
		flags |= FLAG_IPV6;
	}

	switch (app) {
	case APP_VERSION:
		printf("ipcalc %s\n", VERSION);
		return 0;
	case APP_DEAGGREGATE:
		deaggregate(ipStr, flags);
		return 0;
	case APP_SPLIT:
	case APP_CHECK_ADDRESS:
	case APP_COMPARE:
	case APP_SHOW_INFO:
		/* These are handled lower into the info app */
		break;
	}

	/* The main application which displays information about an address or
	 * a network. */
	if (flags & FLAG_RANDOM) {
		if (ipStr) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: provided superfluous parameter '%s'\n", ipStr);
			return exit_failure;
		}

		prefix = str_to_prefix(&flags, randomStr, 1);
		if (prefix < 0) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: bad %s prefix: %s\n", (flags&FLAG_IPV6)?"IPv6":"IPv4", randomStr);
			return exit_failure;
		}

		ipStr = generate_ip_network(prefix, flags);
		if (ipStr == NULL) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: cannot generate network with prefix: %u\n",
					prefix);
			return exit_failure;
		}
	}

	if (hostname == NULL && ipStr == NULL) {
		if (!beSilent) {
			fprintf(stderr,
				"ipcalc: ip address expected\n");
			usage(1);
		}
		return exit_failure;
	}

	/* resolve IP address if a hostname was given */
	if (hostname) {
		int family = AF_UNSPEC;
		if (flags & FLAG_IPV6)
			family = AF_INET6;
		else if (flags & FLAG_IPV4)
			family = AF_INET;

		ipStr = get_ip_address(family, hostname);
		if (ipStr == NULL) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: could not resolve %s\n", hostname);
			return exit_failure;
		}

		if ((flags & FLAG_IPV4) == 0 && strchr(ipStr, ':') != NULL) {
			flags |= FLAG_IPV6;
		}
	}

	if (chptr) {
		if ((flags & FLAG_IPV6) == 0) {
			prefixStr = chptr;
		} else {
			if (!beSilent) {
				fprintf(stderr, "ipcalc: unexpected argument: %s\n",
					chptr);
				usage(1);
			}
			return exit_failure;
		}
	}

	if (prefixStr == NULL && strchr(ipStr, '/') != NULL) {
		prefixStr = strchr(ipStr, '/');
		*prefixStr = '\0';	/* fix up ipStr */
		prefixStr++;
	}

	if (prefixStr != NULL) {
		prefix = str_to_prefix(&flags, prefixStr, 0);
		if (prefix < 0) {
			if (!beSilent)
				fprintf(stderr,
					"ipcalc: bad %s prefix: %s\n", (flags & FLAG_IPV6)?"IPv6":"IPv4", prefixStr);
			return exit_failure;
		}
	}

	if (flags & FLAG_IPV6)
		r = get_ipv6_info(ipStr, prefix, &info, flags);
	else
		r = get_ipv4_info(ipStr, prefix, &info, flags);

	if (r < 0) {
		return exit_failure;
	}

	switch (app) {
	case APP_SPLIT:
		{
			struct split_req *reqs = NULL;
			unsigned i, nfill = 0, nreqs = 0;

			if (nsplits && nsplitHosts) {
				if (!beSilent)
					fprintf(stderr,
						"ipcalc: --split-hosts cannot be combined with --split\n");
				return exit_failure;
			}

			for (i = 0; i < nsplitHosts; i++) {
				if (parse_split_hosts(flags, splitHostsStrs[i], &reqs, &nreqs) < 0)
					return exit_failure;
			}

			if (nsplits) {
				reqs = calloc(nsplits, sizeof(reqs[0]));
				if (reqs == NULL)
					exit(exit_failure);
				nreqs = nsplits;
			}

			for (i = 0; i < nsplits; i++) {
				if (parse_split_req(&flags, splitStrs[i], &reqs[i]) < 0)
					return exit_failure;

				if (reqs[i].count == 0 && ++nfill > 1) {
					if (!beSilent)
						fprintf(stderr,
							"ipcalc: only one split prefix may be given without a count\n");
					return exit_failure;
				}
			}

			show_split_networks(reqs, nreqs, &info, flags);
		}
		return 0;
	case APP_CHECK_ADDRESS:
		return 0;
	case APP_COMPARE:
		return compare_networks(cmpOp, &info, flags, cmpStr, userFlags);
	default:
		break;
	}

	if ((isatty(STDOUT_FILENO) != 0) && (getenv("NO_COLOR") == 0))
		colors = 1;

	show_info_fields(&info, format, randomStr != NULL);

	return 0;
}

