/*
 * Copyright (c) 2003-2016  Simon Ekstrand
 * Copyright (c) 2010-2016  Joachim Nilsson
 * Copyright (c) 2016 Nikos Mavrogiannopoulos
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <inttypes.h>

#include "ipcalc.h"

/* Splitting of a network into subnets.
 *
 * Terminology:
 *  - request: a [COUNT:]PREFIX item, i.e., COUNT subnets of size /PREFIX.
 *  - fill request: a request without a count (count is zero); it takes all
 *    the space that remains after the counted requests are allocated.
 *  - equal split: a split with a single fill request (e.g., -S 26).
 *  - VLSM split: any other split, i.e., one with counted requests.
 *  - unused: the space of the network that no request was allocated.
 *  - host-sized request: a request for a single subnet that holds a number
 *    of hosts (--split-hosts); its subnets are printed with their details,
 *    as in the Debian ipcalc's split.
 *
 * Counted requests are allocated largest first (equal sizes keep the request
 * order), back to back from the start of the network, which is the layout
 * the Debian ipcalc uses. Since every block allocated before a request is at
 * least as large as it, the allocation cursor is always aligned to the size
 * of the request and no space is lost. The fill request is aligned to its
 * own size and extends to the end of the network. An equal split's fill
 * request starts at the network address, so only a VLSM split can leave
 * unused space.
 *
 * Addresses of both families are handled as 128-bit numbers, with IPv4
 * using the low 32 bits.
 */

typedef struct {
	uint64_t hi, lo;
} u128_t;

/* last is inclusive: the last address of the last subnet */
struct split_run {
	u128_t start;
	u128_t last;
	unsigned prefix;
};

static u128_t u128_from64(uint64_t v)
{
	u128_t r;

	r.hi = 0;
	r.lo = v;
	return r;
}

static u128_t u128_add(u128_t a, u128_t b)
{
	u128_t r;

	r.lo = a.lo + b.lo;
	r.hi = a.hi + b.hi + (r.lo < a.lo);
	return r;
}

static u128_t u128_sub(u128_t a, u128_t b)
{
	u128_t r;

	r.lo = a.lo - b.lo;
	r.hi = a.hi - b.hi - (a.lo < b.lo);
	return r;
}

static int u128_cmp(u128_t a, u128_t b)
{
	if (a.hi != b.hi)
		return (a.hi < b.hi) ? -1 : 1;
	if (a.lo != b.lo)
		return (a.lo < b.lo) ? -1 : 1;
	return 0;
}

static int u128_is_zero(u128_t a)
{
	return (a.hi == 0 && a.lo == 0);
}

static u128_t u128_and(u128_t a, u128_t b)
{
	a.hi &= b.hi;
	a.lo &= b.lo;
	return a;
}

static u128_t u128_or(u128_t a, u128_t b)
{
	a.hi |= b.hi;
	a.lo |= b.lo;
	return a;
}

static u128_t u128_shl(u128_t a, unsigned n)
{
	u128_t r;

	if (n == 0) {
		r = a;
	} else if (n < 64) {
		r.hi = (a.hi << n) | (a.lo >> (64 - n));
		r.lo = a.lo << n;
	} else if (n < 128) {
		r.hi = a.lo << (n - 64);
		r.lo = 0;
	} else {
		r.hi = r.lo = 0;
	}
	return r;
}

static u128_t u128_shr(u128_t a, unsigned n)
{
	u128_t r;

	if (n == 0) {
		r = a;
	} else if (n < 64) {
		r.lo = (a.lo >> n) | (a.hi << (64 - n));
		r.hi = a.hi >> n;
	} else if (n < 128) {
		r.lo = a.hi >> (n - 64);
		r.hi = 0;
	} else {
		r.hi = r.lo = 0;
	}
	return r;
}

/* Returns a value with the n low bits set */
static u128_t u128_low_mask(unsigned n)
{
	u128_t r;

	if (n >= 128) {
		r.hi = r.lo = UINT64_MAX;
		return r;
	}
	return u128_sub(u128_shl(u128_from64(1), n), u128_from64(1));
}

/* Divides v by 10 in place and returns the remainder */
static unsigned u128_divmod10(u128_t *v)
{
	uint64_t r, mid, low, mid_q, low_q;

	r = v->hi % 10;
	v->hi /= 10;

	mid = (r << 32) | (v->lo >> 32);
	mid_q = mid / 10;
	r = mid % 10;

	low = (r << 32) | (v->lo & 0xffffffff);
	low_q = low / 10;
	r = low % 10;

	v->lo = (mid_q << 32) | low_q;
	return r;
}

/* Prints v + 1 in decimal; the addition is done on the digits so that
 * 2^128 can be printed. */
static const char *u128_plus1_to_dec(u128_t v, char *buf, size_t size)
{
	char digits[48];
	unsigned n = 0, i, carry = 1;

	do {
		digits[n++] = u128_divmod10(&v);
	} while (!u128_is_zero(v));

	for (i = 0; i < n && carry; i++) {
		digits[i] += carry;
		carry = (digits[i] == 10);
		if (carry)
			digits[i] = 0;
	}
	if (carry)
		digits[n++] = 1;

	if (size < n + 1) {
		buf[0] = 0;
		return buf;
	}
	for (i = 0; i < n; i++)
		buf[i] = '0' + digits[n - 1 - i];
	buf[n] = 0;

	return buf;
}

static int str_to_u128(const char *str, unsigned flags, u128_t *v)
{
	unsigned i;

	if (flags & FLAG_IPV6) {
		struct in6_addr a;

		if (inet_pton(AF_INET6, str, &a) <= 0)
			return -1;

		v->hi = v->lo = 0;
		for (i = 0; i < 8; i++)
			v->hi = (v->hi << 8) | a.s6_addr[i];
		for (i = 8; i < 16; i++)
			v->lo = (v->lo << 8) | a.s6_addr[i];
	} else {
		struct in_addr a;

		if (inet_pton(AF_INET, str, &a) <= 0)
			return -1;

		v->hi = 0;
		v->lo = ntohl(a.s_addr);
	}

	return 0;
}

static const char *u128_to_str(u128_t v, unsigned flags, char *buf, size_t size)
{
	int i;

	if (flags & FLAG_IPV6) {
		struct in6_addr a;

		for (i = 15; i >= 8; i--) {
			a.s6_addr[i] = v.lo & 0xff;
			v.lo >>= 8;
		}
		for (i = 7; i >= 0; i--) {
			a.s6_addr[i] = v.hi & 0xff;
			v.hi >>= 8;
		}
		return inet_ntop(AF_INET6, &a, buf, size);
	} else {
		struct in_addr a;

		a.s_addr = htonl((uint32_t)v.lo);
		return inet_ntop(AF_INET, &a, buf, size);
	}
}

static void print_network(unsigned *jsonchain, u128_t addr, unsigned prefix, unsigned flags)
{
	char str[INET6_ADDRSTRLEN];

	u128_to_str(addr, flags, str, sizeof(str));

	if (!(flags & FLAG_NO_DECORATE) || (flags & FLAG_JSON)) {
		default_printf(jsonchain, "Network:\t", NULL, "%s/%u", str, prefix);
	} else {
		printf("%s/%u\n", str, prefix);
	}
}

/* Returns the host bits of the largest network that starts at start and
 * ends at or before last */
static unsigned largest_aligned_block(u128_t start, u128_t last, unsigned width)
{
	const u128_t one = u128_from64(1);
	unsigned hb = 0;

	while (hb < width && u128_is_zero(u128_and(start, u128_shl(one, hb)))) {
		if (u128_cmp(u128_or(start, u128_low_mask(hb + 1)), last) > 0)
			break;
		hb++;
	}

	return hb;
}

/* last is inclusive */
static void print_range(unsigned *jsonchain, u128_t start, u128_t last,
			unsigned width, unsigned flags)
{
	const u128_t one = u128_from64(1);
	unsigned hb;

	while (1) {
		hb = largest_aligned_block(start, last, width);
		print_network(jsonchain, start, width - hb, flags);

		start = u128_or(start, u128_low_mask(hb));
		if (u128_cmp(start, last) >= 0)
			break;
		start = u128_add(start, one);
	}
}

static void print_host_sized_subnet(unsigned *jsonchain, unsigned idx, const struct split_req *req,
				    const struct split_run *run, unsigned width, unsigned flags)
{
	unsigned hb = width - run->prefix;
	const u128_t one = u128_from64(1);
	u128_t mask = u128_sub(u128_low_mask(width), u128_low_mask(hb));
	char str[INET6_ADDRSTRLEN], buf[64];
	char *title;

	if (idx)
		output_separate(jsonchain);
	safe_asprintf(&title, "%u. Requested size: ", idx + 1);
	default_printf(jsonchain, title, NULL, "%" PRIu64 " hosts", req->hosts);
	free(title);
	print_network(jsonchain, run->start, run->prefix, flags);
	default_printf(jsonchain, "Netmask:\t", NULL, "%s = %u",
		       u128_to_str(mask, flags, str, sizeof(str)), run->prefix);

	if (flags & FLAG_IPV6) {
		default_printf(jsonchain, "HostMin:\t", NULL, "%s",
			       u128_to_str(run->start, flags, str, sizeof(str)));
		default_printf(jsonchain, "HostMax:\t", NULL, "%s",
			       u128_to_str(run->last, flags, str, sizeof(str)));
		ipv6_prefix_to_hosts(buf, sizeof(buf), run->prefix);
	} else {
		/* host-sized IPv4 requests are at least /30, so start + 1
		 * and last - 1 are host addresses */
		default_printf(jsonchain, "Broadcast:\t", NULL, "%s",
			       u128_to_str(run->last, flags, str, sizeof(str)));
		default_printf(jsonchain, "HostMin:\t", NULL, "%s",
			       u128_to_str(u128_add(run->start, one), flags, str, sizeof(str)));
		default_printf(jsonchain, "HostMax:\t", NULL, "%s",
			       u128_to_str(u128_sub(run->last, one), flags, str, sizeof(str)));
		ipv4_prefix_to_hosts(buf, sizeof(buf), run->prefix);
	}
	default_printf(jsonchain, "Hosts/Net:\t", NULL, "%s", buf);
}

static void too_small(const struct ip_info_st *info)
{
	if (!beSilent)
		fprintf(stderr, "ipcalc: %s/%u is too small for the requested subnets\n",
			info->network, info->prefix);
	exit(1);
}

void show_split_networks(const struct split_req *reqs, unsigned nreqs,
			 const struct ip_info_st *info, unsigned flags)
{
	unsigned width = (flags & FLAG_IPV6) ? 128 : 32;
	unsigned i, j, hb, ncounted = 0, fill = nreqs;
	unsigned has_space = 1, has_gap = 0, same_prefix = 1;
	/* main() rejects combining --split-hosts with --split, so the first
	 * request decides */
	unsigned host_sized = (reqs[0].hosts != 0);
	unsigned details = host_sized && !(flags & (FLAG_JSON | FLAG_NO_DECORATE));
	unsigned jsonchain = JSON_FIRST;
	const u128_t one = u128_from64(1);
	u128_t net, last, cursor, gap_last, nets_m1, needed_m1;
	struct split_run *runs;
	unsigned *order;
	char buf[64];

	if (str_to_u128(info->network, flags, &net) < 0) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: bad %s address: %s\n",
				(flags & FLAG_IPV6) ? "IPv6" : "IPv4", info->network);
		exit(1);
	}

	last = u128_or(net, u128_low_mask(width - info->prefix));

	runs = calloc(nreqs, sizeof(runs[0]));
	order = calloc(nreqs, sizeof(order[0]));
	if (runs == NULL || order == NULL) {
		fprintf(stderr, "ipcalc: memory error\n");
		exit(1);
	}

	for (i = 0; i < nreqs; i++) {
		/* a host-sized request has no user-given prefix for the
		 * "Cannot subnet to /N" error to name */
		if (host_sized && reqs[i].prefix < info->prefix)
			too_small(info);

		if (reqs[i].prefix < info->prefix || reqs[i].prefix > width) {
			if (!beSilent)
				fprintf(stderr, "Cannot subnet to /%u with this base network, use a prefix > /%u\n",
					reqs[i].prefix, info->prefix);
			exit(1);
		}

		if (reqs[i].prefix != reqs[0].prefix)
			same_prefix = 0;

		if (reqs[i].count == 0) {
			fill = i;
			continue;
		}

		/* stable: equal prefixes keep the request order, as in the
		 * Debian ipcalc */
		for (j = ncounted; j > 0 && reqs[order[j - 1]].prefix > reqs[i].prefix; j--)
			order[j] = order[j - 1];
		order[j] = i;
		ncounted++;
	}

	cursor = net;
	for (j = 0; j < ncounted; j++) {
		const struct split_req *req = &reqs[order[j]];
		struct split_run *run = &runs[order[j]];

		hb = width - req->prefix;

		/* the cursor is aligned to the request, so (last - cursor) >> hb
		 * is the number of available subnets minus one */
		if (!has_space ||
		    u128_cmp(u128_from64(req->count - 1), u128_shr(u128_sub(last, cursor), hb)) > 0)
			too_small(info);

		run->start = cursor;
		run->last = u128_add(cursor, u128_sub(u128_shl(u128_from64(req->count), hb), one));
		run->prefix = req->prefix;

		/* the cursor is not advanced past the end, as last + 1 wraps
		 * when the network ends at the top of the IPv6 space */
		if (u128_cmp(run->last, last) == 0)
			has_space = 0;
		else
			cursor = u128_add(run->last, one);
	}

	if (fill < nreqs) {
		u128_t low_mask, rem, start = cursor;

		hb = width - reqs[fill].prefix;
		low_mask = u128_low_mask(hb);
		rem = u128_and(cursor, low_mask);

		/* the space skipped to align the fill request is unused */
		if (has_space && !u128_is_zero(rem)) {
			u128_t gap = u128_add(u128_sub(low_mask, rem), one);

			if (u128_cmp(u128_sub(last, cursor), gap) < 0) {
				has_space = 0;
			} else {
				start = u128_add(cursor, gap);
				gap_last = u128_sub(start, one);
				has_gap = 1;
			}
		}

		if (!has_space)
			too_small(info);

		runs[fill].start = start;
		runs[fill].last = last;
		runs[fill].prefix = reqs[fill].prefix;
	} else if (has_space) {
		gap_last = last;
		has_gap = 1;
	}

	output_start(&jsonchain);
	array_start(&jsonchain, "Split networks", "SPLITNETWORK");

	nets_m1 = u128_from64(0);
	for (i = 0; i < nreqs; i++) {
		u128_t addr, low_mask, step;

		hb = width - runs[i].prefix;
		low_mask = u128_low_mask(hb);
		step = u128_shl(one, hb);

		/* the total is kept minus one, as 2^128 subnets do not fit */
		if (i > 0)
			nets_m1 = u128_add(nets_m1, one);
		nets_m1 = u128_add(nets_m1, u128_shr(u128_sub(runs[i].last, runs[i].start), hb));

		if (details) {
			print_host_sized_subnet(&jsonchain, i, &reqs[i], &runs[i], width, flags);
			continue;
		}

		for (addr = runs[i].start;; addr = u128_add(addr, step)) {
			print_network(&jsonchain, addr, runs[i].prefix, flags);
			if (u128_cmp(u128_add(addr, low_mask), runs[i].last) == 0)
				break;
		}
	}

	array_stop(&jsonchain);

	if ((!(flags & FLAG_NO_DECORATE)) || (flags & FLAG_JSON)) {
		dist_printf(&jsonchain, "\nTotal:  \t", "NETS", "%s", u128_plus1_to_dec(nets_m1, buf, sizeof(buf)));
		/* Hosts/Net is per subnet, so it is printed only when all
		 * subnets have one size, as in every equal split */
		if (same_prefix) {
			if (flags & FLAG_IPV6)
				ipv6_prefix_to_hosts(buf, sizeof(buf), reqs[0].prefix);
			else
				ipv4_prefix_to_hosts(buf, sizeof(buf), reqs[0].prefix);
			dist_printf(&jsonchain, "Hosts/Net:\t", "ADDRESSES", "%s", buf);
		}

		if (host_sized) {
			char str[INET6_ADDRSTRLEN];

			if (has_space)
				needed_m1 = u128_sub(u128_sub(cursor, net), one);
			else
				needed_m1 = u128_sub(last, net);

			for (hb = 0; hb < width && u128_cmp(u128_low_mask(hb), needed_m1) < 0; hb++)
				;

			dist_printf(&jsonchain, "Needed size:\t", "NEEDED", "%s",
				    u128_plus1_to_dec(needed_m1, buf, sizeof(buf)));
			dist_printf(&jsonchain, "Used network:\t", "USEDNETWORK", "%s/%u",
				    u128_to_str(net, flags, str, sizeof(str)), width - hb);
		}

		if (has_gap) {
			output_separate(&jsonchain);
			array_start(&jsonchain, "Unused networks", "UNUSED");
			print_range(&jsonchain, cursor, gap_last, width, flags);
			array_stop(&jsonchain);
		}
	}

	output_stop(&jsonchain);

	free(order);
	free(runs);
}
