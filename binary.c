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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ipcalc.h"

#define PREFIX_NAME "PREFIX"

#define MAX_COLS 135
#define DEFAULT_COLS 80

#define LABEL_COLS 12
#define VALUE_COLS 17
#define DETAILS_INDENT (LABEL_COLS + VALUE_COLS)

/* Stand for the bracket characters, which take one column but several
 * bytes, so that a line holds one byte per column */
enum glyph {
	GLYPH_LEFT = 1,
	GLYPH_RIGHT,
	GLYPH_FILL
};

enum style {
	STYLE_PLAIN,
	STYLE_NETWORK,
	STYLE_HOST
};

struct layout {
	unsigned width;
	unsigned band_bits;
	unsigned group_bits;
	char separator;
	int hex;
};

static const struct layout ipv4_layout = { 32, 32, 8, '.', 0 };
static const struct layout ipv6_layout = { 128, 64, 16, ':', 1 };
static const struct layout ipv6_wide_layout = { 128, 128, 16, ':', 1 };

struct segment {
	unsigned first;
	unsigned end;
	enum style style;
	unsigned nlabels;
	char labels[3][24];
};

struct row {
	const char *name;
	const char *var;
	uint32_t value;
};

struct line {
	unsigned indent;
	unsigned len;
	char text[MAX_COLS];
	unsigned char style[MAX_COLS];
};

static unsigned get_bit(const unsigned char *addr, unsigned k)
{
	return (addr[k / 8] >> (7 - k % 8)) & 1;
}

static void line_init(struct line *l, unsigned indent)
{
	l->indent = indent;
	l->len = 0;
	memset(l->text, ' ', sizeof(l->text));
	memset(l->style, STYLE_PLAIN, sizeof(l->style));
}

static void put_char(struct line *l, unsigned col, char c, enum style style)
{
	if (col >= MAX_COLS)
		return;
	l->text[col] = c;
	l->style[col] = style;
	if (col >= l->len)
		l->len = col + 1;
}

static void put_text(struct line *l, unsigned col, const char *str, enum style style)
{
	for (; *str; str++, col++)
		put_char(l, col, *str, style);
}

static const char *style_color(enum style style)
{
	switch (style) {
	case STYLE_NETWORK:
		return KBLUE;
	case STYLE_HOST:
		return KDIM;
	default:
		return NULL;
	}
}

static const char *glyph_text(unsigned char c)
{
	switch (c) {
	case GLYPH_LEFT:
		return "└";
	case GLYPH_RIGHT:
		return "┘";
	case GLYPH_FILL:
		return "─";
	default:
		return NULL;
	}
}

static void print_line(struct line *l)
{
	unsigned col = 0;

	while (l->len > 0 && l->text[l->len - 1] == ' ')
		l->len--;
	if (l->len > 0) {
		char indent[DETAILS_INDENT + 1];

		snprintf(indent, sizeof(indent), "%*s", l->indent, "");
		color_fputs(NULL, indent);
	}

	while (col < l->len) {
		char run[MAX_COLS * 3 + 1];
		unsigned len = 0;
		unsigned char style = l->style[col];

		for (; col < l->len && l->style[col] == style; col++) {
			const char *glyph = glyph_text(l->text[col]);

			if (glyph != NULL) {
				memcpy(run + len, glyph, strlen(glyph));
				len += strlen(glyph);
			} else {
				run[len] = l->text[col];
				len++;
			}
		}
		run[len] = 0;
		color_fputs(style_color(style), run);
	}
	color_fputs(NULL, "\n");
}

static unsigned band_columns(const struct layout *lay)
{
	unsigned ngroups = lay->band_bits / lay->group_bits;

	return lay->band_bits + ngroups - 1;
}

static unsigned output_columns(void)
{
	const char *env = getenv("COLUMNS");
	struct winsize ws;
	int cols;

	if (env && safe_atoi(env, &cols) == 0 && cols > 0)
		return cols;
	if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
	    ws.ws_col > 0)
		return ws.ws_col;
	return DEFAULT_COLS;
}

/* Each group is followed by a separator column */
static unsigned group_column(const struct layout *lay, unsigned group)
{
	return group * (lay->group_bits + 1);
}

/* Column of bit k of the band that starts at bit start */
static unsigned bit_column(const struct layout *lay, unsigned start, unsigned k)
{
	unsigned pos = k - start;

	return group_column(lay, pos / lay->group_bits) + pos % lay->group_bits;
}

static unsigned group_value(const unsigned char *addr, unsigned first, unsigned bits)
{
	unsigned k, value = 0;

	for (k = first; k < first + bits; k++)
		value = (value << 1) | get_bit(addr, k);
	return value;
}

static unsigned group_host_bits(unsigned prefix, unsigned first, unsigned bits)
{
	unsigned end = first + bits;

	if (prefix >= end)
		return 0;
	if (prefix <= first)
		return bits;
	return end - prefix;
}

static void format_group(char *str, size_t size, const struct layout *lay,
			 unsigned value, unsigned host_bits)
{
	unsigned high = value | ((1u << host_bits) - 1);

	if (host_bits == 0) {
		if (lay->hex)
			snprintf(str, size, "%04x", value);
		else
			snprintf(str, size, "%u", value);
	} else {
		if (lay->hex)
			snprintf(str, size, "%04x-%04x", value, high);
		else
			snprintf(str, size, "%u-%u", value, high);
	}
}

static void print_band(const unsigned char *addr, unsigned prefix, unsigned start,
		       const struct layout *lay, unsigned indent)
{
	unsigned gbits = lay->group_bits, ngroups = lay->band_bits / gbits;
	char sep[2] = { lay->separator, 0 };
	struct line l;
	unsigned g, i;
	char str[16];

	line_init(&l, indent);
	for (g = 0; g < ngroups; g++) {
		unsigned first = start + g * gbits;
		unsigned col = group_column(lay, g);
		unsigned host_bits = group_host_bits(prefix, first, gbits);
		unsigned value = group_value(addr, first, gbits);
		enum style style = STYLE_NETWORK;

		if (host_bits > 0)
			style = STYLE_HOST;
		format_group(str, sizeof(str), lay, value, host_bits);
		put_text(&l, col + gbits - strlen(str), str, style);
		if (g + 1 < ngroups)
			put_text(&l, col + gbits, sep, STYLE_PLAIN);
	}
	print_line(&l);

	line_init(&l, indent);
	for (g = 0; g < ngroups; g++) {
		unsigned first = start + g * gbits;
		unsigned col = group_column(lay, g);

		for (i = 0; i < gbits; i++) {
			unsigned k = first + i;
			char bit = '0' + get_bit(addr, k);

			if (k < prefix)
				put_char(&l, col + i, bit, STYLE_NETWORK);
			else
				put_char(&l, col + i, 'x', STYLE_HOST);
		}
		if (g + 1 < ngroups)
			put_text(&l, col + gbits, sep, STYLE_PLAIN);
	}
	print_line(&l);
}

static void put_segment_ends(struct line *l, unsigned first, unsigned last,
			     int left, int right, enum style style)
{
	if (left)
		put_char(l, first, GLYPH_LEFT, style);
	if (right)
		put_char(l, last, GLYPH_RIGHT, style);
}

/* Draws a segment over columns first to last, with the first label that
 * fits, so that narrow segments (e.g., /1 or /31) still say something
 * (REQ-BINARY-OUTPUT-008) */
static void put_segment(struct line *l, unsigned first, unsigned last, int left, int right,
			const struct segment *seg)
{
	unsigned width = last - first + 1;
	unsigned ends = left + right;
	unsigned inner = 0;
	const char *shortest = seg->labels[seg->nlabels - 1];
	unsigned shortest_len = strlen(shortest);
	char text[32];
	unsigned i, col;

	if (width > ends)
		inner = width - ends;

	for (i = 0; i < seg->nlabels; i++) {
		if (strlen(seg->labels[i]) <= inner)
			break;
	}

	if (i == seg->nlabels) {
		if (shortest_len <= width)
			put_text(l, first + (width - shortest_len) / 2, shortest,
				 seg->style);
		else if (ends <= width)
			put_segment_ends(l, first, last, left, right,
					 seg->style);
		return;
	}

	if (strlen(seg->labels[i]) + 2 <= inner)
		snprintf(text, sizeof(text), " %s ", seg->labels[i]);
	else
		snprintf(text, sizeof(text), "%s", seg->labels[i]);

	for (col = first; col <= last; col++)
		put_char(l, col, GLYPH_FILL, seg->style);
	put_segment_ends(l, first, last, left, right, seg->style);
	put_text(l, first + left + (inner - strlen(text)) / 2, text,
		 seg->style);
}

/* A prefix segment is labeled by where it ends (e.g., "network /24"),
 * any other by its number of bits (e.g., "host 8") */
static void add_segment(struct segment *segs, unsigned *nsegs,
			unsigned first, unsigned end, enum style style,
			const char *name, int is_prefix)
{
	struct segment *seg = &segs[*nsegs];
	size_t size = sizeof(seg->labels[0]);

	seg->first = first;
	seg->end = end;
	seg->style = style;
	if (is_prefix) {
		snprintf(seg->labels[0], size, "%s /%u", name, end);
		snprintf(seg->labels[1], size, "/%u", end);
		snprintf(seg->labels[2], size, "%u", end);
		seg->nlabels = 3;
	} else {
		snprintf(seg->labels[0], size, "%s %u", name, end - first);
		snprintf(seg->labels[1], size, "%u", end - first);
		seg->nlabels = 2;
	}
	(*nsegs)++;
}

/* Global unicast (2000::/3) and unique local (fc00::/7) addresses have a
 * routing prefix, a subnet ID and a 64-bit interface ID (RFC 3587,
 * RFC 4193); other types, e.g., link-local or multicast, do not */
static int is_structured(const unsigned char *net, const struct layout *lay)
{
	if (lay->width != 128)
		return 0;
	return (net[0] & 0xe0) == 0x20 || (net[0] & 0xfe) == 0xfc;
}

static unsigned make_segments(struct segment *segs, unsigned prefix,
			      const struct layout *lay, int structured)
{
	unsigned n = 0;

	if (structured && prefix < 64) {
		add_segment(segs, &n, 0, prefix, STYLE_NETWORK,
			    "routing prefix", 1);
		add_segment(segs, &n, prefix, 64, STYLE_HOST, "subnet ID", 0);
		add_segment(segs, &n, 64, 128, STYLE_HOST, "interface ID", 0);
	} else if (structured && prefix == 64) {
		add_segment(segs, &n, 0, 64, STYLE_NETWORK, "subnet prefix", 1);
		add_segment(segs, &n, 64, 128, STYLE_HOST, "interface ID", 0);
	} else {
		if (prefix > 0)
			add_segment(segs, &n, 0, prefix, STYLE_NETWORK,
				    "network", 1);
		add_segment(segs, &n, prefix, lay->width, STYLE_HOST, "host", 0);
	}
	return n;
}

static void print_brackets(const struct segment *segs, unsigned nsegs,
			   unsigned start, const struct layout *lay,
			   unsigned indent)
{
	unsigned end = start + lay->band_bits;
	struct line l;
	unsigned i;

	line_init(&l, indent);
	for (i = 0; i < nsegs; i++) {
		unsigned first = segs[i].first, last = segs[i].end - 1;

		if (segs[i].end <= start || segs[i].first >= end)
			continue;
		if (first < start)
			first = start;
		if (last >= end)
			last = end - 1;
		put_segment(&l, bit_column(lay, start, first),
			    bit_column(lay, start, last), segs[i].first >= start,
			    segs[i].end <= end, &segs[i]);
	}
	print_line(&l);
}

static void print_diagram(const ip_info_st *info, const unsigned char *net,
			  const struct layout *lay)
{
	unsigned prefix = info->prefix, start, nsegs, indent = 0;
	int structured = is_structured(net, lay);
	char str[LABEL_COLS + INET6_ADDRSTRLEN + 8];
	struct segment segs[3];

	if (prefix == lay->width)
		snprintf(str, sizeof(str), "%-*s%s\n\n", LABEL_COLS, "Address:",
			 info->ip);
	else
		snprintf(str, sizeof(str), "%-*s%s/%u\n\n", LABEL_COLS,
			 "Network:", info->network, prefix);
	color_fputs(NULL, str);

	if (lay->width == 32 && prefix < 32)
		indent = DETAILS_INDENT;

	for (start = 0; start < lay->width; start += lay->band_bits) {
		unsigned end = start + lay->band_bits;
		int all_host_bits = start > 0 && prefix <= start;

		if (all_host_bits) {
			snprintf(str, sizeof(str), "bits %u-%u: %s, all x",
				 start, lay->width - 1,
				 structured ? "interface ID" : "host");
			color_fputs(KDIM, str);
			color_fputs(NULL, "\n");
			return;
		}
		print_band(net, prefix, start, lay, indent);
		/* the bands before the one where the prefix ends have
		 * prefix > end, and those after it returned above */
		if (prefix < lay->width && prefix <= end) {
			nsegs = make_segments(segs, prefix, lay, structured);
			print_brackets(segs, nsegs, start, lay, indent);
		}
	}
}

static void print_row(const struct row *row, unsigned prefix)
{
	struct in_addr in = { htonl(row->value) };
	char addr[INET_ADDRSTRLEN], str[32];
	struct line l;
	unsigned k;

	line_init(&l, 0);
	inet_ntop(AF_INET, &in, addr, sizeof(addr));
	snprintf(str, sizeof(str), "%s:", row->name);
	put_text(&l, 0, str, STYLE_PLAIN);
	put_text(&l, LABEL_COLS, addr, STYLE_PLAIN);
	for (k = 0; k < 32; k++) {
		unsigned col = DETAILS_INDENT + bit_column(&ipv4_layout, 0, k);
		char bit = '0' + ((row->value >> (31 - k)) & 1);

		put_char(&l, col, bit, k < prefix ? STYLE_NETWORK : STYLE_PLAIN);
		if (k % 8 == 7 && k < 31)
			put_char(&l, col + 1, '.', STYLE_PLAIN);
	}
	print_line(&l);
}

static unsigned make_rows(const ip_info_st *info, const unsigned char *net,
			  int always_address, struct row *rows)
{
	uint32_t network = (uint32_t)net[0] << 24 | net[1] << 16 |
			   net[2] << 8 | net[3];
	uint32_t mask = info->prefix ? UINT32_MAX << (32 - info->prefix) : 0;
	uint32_t broadcast = network | ~mask;
	unsigned nrows = 0;
	struct in_addr in;

	if (inet_pton(AF_INET, info->ip, &in) == 1 &&
	    (always_address || ntohl(in.s_addr) != network))
		rows[nrows++] = (struct row){ "Address", "ADDRESS",
					      ntohl(in.s_addr) };

	rows[nrows++] = (struct row){ "Netmask", "NETMASK", mask };
	if (info->prefix == 31) {
		rows[nrows++] = (struct row){ "HostMin", "HOSTMIN", network };
		rows[nrows++] = (struct row){ "HostMax", "HOSTMAX", broadcast };
	} else {
		rows[nrows++] = (struct row){ "Network", "NETWORK", network };
		rows[nrows++] = (struct row){ "HostMin", "HOSTMIN",
					      network + 1 };
		rows[nrows++] = (struct row){ "HostMax", "HOSTMAX",
					      broadcast - 1 };
		rows[nrows++] = (struct row){ "Broadcast", "BROADCAST",
					      broadcast };
	}
	return nrows;
}

static void print_details(const ip_info_st *info, const unsigned char *net)
{
	struct row rows[6];
	unsigned nrows, i;

	nrows = make_rows(info, net, 0, rows);
	color_fputs(NULL, "\n");
	for (i = 0; i < nrows; i++)
		print_row(&rows[i], info->prefix);
}

static void format_bits(char *bits, const unsigned char *addr, unsigned nbits)
{
	unsigned k;

	for (k = 0; k < nbits; k++)
		bits[k] = '0' + get_bit(addr, k);
	bits[k] = 0;
}

static void print_var(enum output_format format, unsigned *jsonchain,
		      const char *name, const char *value)
{
	/* bits and digits need no shell quoting */
	if (format == FORMAT_SHELL)
		printf("%s=%s\n", name, value);
	else
		json_printf(jsonchain, name, "%s", value);
}

static void print_bits_var(enum output_format format, unsigned *jsonchain,
			   const char *name, const unsigned char *addr,
			   unsigned nbits)
{
	char bits[129];

	format_bits(bits, addr, nbits);
	print_var(format, jsonchain, name, bits);
}

static void print_vars(const ip_info_st *info, const unsigned char *net,
		       const struct layout *lay, enum output_format format,
		       unsigned *jsonchain)
{
	struct row rows[6];
	unsigned nrows, i;
	char prefix[4];

	if (info->prefix == lay->width) {
		print_bits_var(format, jsonchain, "ADDRESS", net, lay->width);
		return;
	}
	snprintf(prefix, sizeof(prefix), "%u", info->prefix);
	print_var(format, jsonchain, PREFIX_NAME, prefix);

	if (lay->width == 128) {
		print_bits_var(format, jsonchain, "NETWORK", net, 128);
		return;
	}

	nrows = make_rows(info, net, 1, rows);
	for (i = 0; i < nrows; i++) {
		uint32_t value = htonl(rows[i].value);

		print_bits_var(format, jsonchain, rows[i].var,
			       (const unsigned char *)&value, 32);
	}
}

void show_binary(const ip_info_st *info, unsigned flags, enum output_format format)
{
	unsigned char net[16] = { 0 };
	const struct layout *lay = &ipv4_layout;
	int family = AF_INET;
	unsigned jsonchain = JSON_FIRST;
	char bits[129];

	if (flags & FLAG_IPV6) {
		lay = &ipv6_layout;
		family = AF_INET6;
	}

	if (inet_pton(family, info->network, net) != 1) {
		if (!beSilent)
			fprintf(stderr, "ipcalc: cannot convert the address to binary\n");
		exit(exit_failure);
	}
	switch (format) {
	case FORMAT_VALUE:
		format_bits(bits, net, lay->width);
		printf("%s\n", bits);
		return;
	case FORMAT_SHELL:
		print_vars(info, net, lay, format, &jsonchain);
		return;
	case FORMAT_JSON:
		output_start(&jsonchain);
		print_vars(info, net, lay, format, &jsonchain);
		output_stop(&jsonchain);
		return;
	case FORMAT_HUMAN:
		break;
	}

	if ((flags & FLAG_IPV6) && output_columns() >= band_columns(&ipv6_wide_layout))
		lay = &ipv6_wide_layout;
	print_diagram(info, net, lay);
	if (!(flags & FLAG_IPV6) && info->prefix < lay->width)
		print_details(info, net);
}
