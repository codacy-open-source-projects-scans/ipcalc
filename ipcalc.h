/*
 * Copyright (c) 2016 Red Hat, Inc. All rights reserved.
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
 */

#ifndef _IPCALC_H
#define _IPCALC_H

#include <stdarg.h> /* for va_list */

#ifdef USE_MAXMIND
  void geo_ip_lookup(const char *ip, char **country, char **ccode, char **city, char  **coord);
  int geo_setup(void);
# ifndef USE_RUNTIME_LINKING
#   define geo_setup() 0
# endif
#else
# define geo_ipv4_lookup(x,y,z,w,a)
# define geo_ipv6_lookup(x,y,z,w,a)
# define geo_setup() -1
#endif

int __attribute__((__format__(printf, 2, 3))) safe_asprintf(char **strp, const char *fmt, ...);
char __attribute__((warn_unused_result)) *safe_strdup(const char *str);
int safe_atoi(const char *s, int *ret_i);

char *calc_reverse_dns4(struct in_addr ip, unsigned prefix, struct in_addr net, struct in_addr bcast);
/* An IPv6 prefix not on a nibble boundary needs up to 2^3 ip6.arpa. zones */
#define MAX_REVERSE_DNS 8
unsigned calc_reverse_dns6(struct in6_addr *ip, unsigned prefix, char *names[MAX_REVERSE_DNS]);

uint32_t prefix2mask(int prefix);
int ipv6_prefix_to_mask(unsigned prefix, struct in6_addr *mask);

struct in_addr calc_network(struct in_addr addr, int prefix);

char *ipv4_prefix_to_hosts(char *hosts, unsigned hosts_size, unsigned prefix);
char *ipv6_prefix_to_hosts(char *hosts, unsigned hosts_size, unsigned prefix);

typedef struct ip_info_st {
	char *ip;
	char *expanded_ip;
	char *expanded_network;
	char *reverse_dns[MAX_REVERSE_DNS];
	unsigned reverse_dns_count;

	char *network;
	char *broadcast;	/* ipv4 only */
	char *netmask;
	char *wildcard;	/* ipv4 only */
	char *hostname;
	char *geoip_country;
	char *geoip_ccode;
	char *geoip_city;
	char *geoip_coord;
	char hosts[64];		/* number of hosts in text */
	unsigned prefix;

	char *hostmin;
	char *hostmax;
	const char *type;
	const char *class;

	/* ipv6 only */
	char *iid;
	char *eui64;
	char *mac;
	const char *mac_scope;
	const char *mac_type;
} ip_info_st;

enum app_t {
	APP_VERSION=1,
	APP_CHECK_ADDRESS=1<<1,
	APP_SHOW_INFO=1<<2,
	APP_SPLIT=1<<3,
	APP_DEAGGREGATE=1<<4,
	APP_COMPARE=1<<5
};

/* Bits of the global flags: options to show a field (FLAG_SHOW_*, FLAG_RESOLVE_*)
 * and modifiers of the output or input */
enum ipcalc_flag {
	FLAG_IPV6=1<<1,
	FLAG_IPV4=1<<2,
	FLAG_SHOW_MODERN_INFO=1<<3,
	FLAG_RESOLVE_IP=1<<4,
	FLAG_RESOLVE_HOST=1<<5,
	FLAG_SHOW_BROADCAST=1<<6,
	FLAG_SHOW_NETMASK=1<<7,
	FLAG_SHOW_NETWORK=1<<8,
	FLAG_SHOW_PREFIX=1<<9,
	FLAG_SHOW_MINADDR=1<<10,
	FLAG_SHOW_MAXADDR=1<<11,
	FLAG_SHOW_ADDRESSES=1<<12,
	FLAG_SHOW_ADDRSPACE=1<<13,
	FLAG_GET_GEOIP=1<<14,
	FLAG_SHOW_GEOIP=(1<<15)|FLAG_GET_GEOIP,
	FLAG_SHOW_ALL_INFO=1<<16,
	FLAG_SHOW_REVERSE=1<<17,
	FLAG_ASSUME_CLASS_PREFIX=1<<18,
	FLAG_SHOW_CIDR=1<<19,
	FLAG_NO_DECORATE=1<<20,
	FLAG_SHOW_ADDRESS=1<<21,
	FLAG_JSON=1<<22,
	FLAG_RANDOM=1<<23,
	FLAG_SHOW_WILDCARD=1<<24,
};

/* A split request: count subnets of size /prefix. A count of zero makes it
 * the fill request, which takes all the remaining space. When hosts is
 * non-zero, the request is for one subnet of that many hosts; the requests
 * of a split are either all host-sized or none is. */
struct split_req {
	unsigned prefix;
	uint64_t count;
	uint64_t hosts;
};

void show_split_networks(const struct split_req *reqs, unsigned nreqs,
			 const struct ip_info_st *info, unsigned flags);
void output_separate(unsigned * const jsonfirst);

void deaggregate(char *str, unsigned flags);

int get_ipv4_info(const char *ipStr, int prefix, ip_info_st * info, unsigned flags);
int get_ipv6_info(const char *ipStr, int prefix, ip_info_st * info, unsigned flags);
int str_to_prefix(unsigned *flags, const char *prefixStr, unsigned fix);

enum net_comparison {
	CMP_EQUALS,
	CMP_SUBNET_OF,
	CMP_OVERLAPS
};

/* userFlags are the flags before the input address set its family, so that
 * otherStr gets its family on its own. Returns the exit status, as cmp(1). */
int compare_networks(enum net_comparison op, const ip_info_st *network, unsigned flags,
		     char *otherStr, unsigned userFlags);

#define KBLUE  "\x1B[34m"
#define KMAG   "\x1B[35m"
#define KRESET "\033[0m"

#define JSON_FIRST 0
#define JSON_NEXT  1
#define JSON_ARRAY_FIRST 2
#define JSON_ARRAY_NEXT  3

void
__attribute__ ((format(printf, 3, 4)))
color_printf(const char *color, const char *title, const char *fmt, ...);
void
__attribute__ ((format(printf, 3, 4)))
json_printf(unsigned * const jsonfirst, const char *jsontitle, const char *fmt, ...);
void va_color_printf(const char *color, const char *title, const char *fmt, va_list varglist);
void va_json_printf(unsigned  * const jsonfirst, const char *jsontitle, const char *fmt, va_list varglist);

void
__attribute__ ((format(printf, 4, 5)))
default_printf(unsigned * const jsonfirst, const char *title, const char *jsontitle, const char *fmt, ...);
void
__attribute__ ((format(printf, 4, 5)))
dist_printf(unsigned * const jsonfirst, const char *title, const char *jsontitle, const char *fmt, ...);

void array_start(unsigned * const jsonfirst, const char *head, const char *json_head);
void array_stop(unsigned * const jsonfirst);
void output_start(unsigned * const jsonfirst);
void output_stop(unsigned * const jsonfirst);

extern int beSilent;
/* 2 when comparing networks, as cmp(1) */
extern int exit_failure;

#endif
