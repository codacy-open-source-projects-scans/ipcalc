---
title: Binary representation of an address or network (--binary)
id-prefix: REQ-BINARY
sources:
  - binary.c (show_binary)
  - ipcalc.c (main, usage, long_options, color_fputs)
  - ipcalc.1.md
  - tests/meson.build
---

# Binary representation of an address or network (`--binary`)

Generated with `contrib/ai/protocols/requirements-elicitation.md` from
issue #34. The input is parsed as in `info.md` (REQ-INFO-INPUT-001 to
REQ-INFO-INPUT-005); mode selection and conflicting options are in
`cli.md` (REQ-CLI-INPUT-001, REQ-CLI-ERR-001), and the output format
options in `cli.md` (REQ-CLI-OUTPUT-011, REQ-CLI-OUTPUT-013,
REQ-CLI-ERR-009).

Terms used below: **width** is 32 for IPv4 and 128 for IPv6; **p** is
the effective prefix (REQ-INFO-INPUT-005), so an address given without
a prefix has p equal to the width; **network bits** are bits 0 to p−1
and **host bits** bits p to width−1, numbered from the most significant
bit of the address, bit 0; an **octet** is 8 bits of an IPv4 address
and a **hextet** 16 bits of an IPv6 address; a **structured** network
is an IPv6 network whose network address is in `2000::/3` (global
unicast) or `fc00::/7` (unique local), the address types that RFC 3587
and RFC 4193 divide into a routing prefix, a subnet ID and the 64-bit
interface ID of RFC 4291 §2.5.1.

## Input

### REQ-BINARY-INPUT-001
**Requirement:** `--binary` MUST select the binary mode and accept, for
both IPv4 and IPv6, the input address in every form the info mode
accepts — address syntax (REQ-INFO-INPUT-001, REQ-INFO-INPUT-002), a
`/prefix` or, for IPv4, `/netmask` suffix or second argument
(REQ-INFO-INPUT-003), the default prefix and `--class-prefix`
(REQ-INFO-INPUT-005), and `-4`/`-6` (REQ-CLI-INPUT-005) — and MUST
reject the inputs they reject with the same diagnostic and exit status
1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`OPT_BINARY`, `APP_SHOW_BITS`); ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options (`--binary`)
**Acceptance:** `ipcalc --binary --format=value 192.168.92.6 255.255.252.0` → `11000000101010000101110000000000`; `ipcalc --binary --format=value --class-prefix 10.1.2.3` → `00001010` followed by 24 `0`s; `ipcalc --binary 1.2.3.400`, `ipcalc --binary 1.2.3.4/33`, `ipcalc --binary 2001:db8::/129`, `ipcalc --binary -6 1.2.3.4` → diagnostic, exit 1 — positive, negative ; test: BitsNetmaskValue, BitsClassPrefixValue, BitsBadAddress, BitsBadPrefixIPv4, BitsBadPrefixIPv6, BitsForcedFamily
**Links:** REQ-CLI-INPUT-001, REQ-CLI-ERR-001

### REQ-BINARY-INPUT-002
**Requirement:** `--binary` MUST reject the deprecated `--no-decorate`
(REQ-CLI-OUTPUT-014), with or without `-j` or `--format=value`, by
printing `ipcalc: --no-decorate is not supported with --binary; use
--format=value` to stderr (subject to `-s`, REQ-CLI-ERR-006) and
exiting with status 1, so that the new mode does not take on a
deprecated option. A `--format` other than `value` MUST be rejected as
a conflict with `--no-decorate` (REQ-CLI-ERR-009) instead.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`noDecorate`, `APP_SHOW_BITS`)
**Acceptance:** `ipcalc --binary --no-decorate 192.168.92.6/22`, `ipcalc --binary -j --no-decorate 192.168.92.0/22`, `ipcalc --binary --no-decorate --format=value 192.168.92.6/22` → the message, exit 1; `ipcalc --binary -s --no-decorate 192.168.92.6/22` → no output, exit 1 — negative ; test: BitsNoDecorate, BitsJsonNoDecorate, BitsNoDecorateFormatValue, BitsNoDecorateSilent
**Links:** REQ-CLI-OUTPUT-006, REQ-CLI-OUTPUT-014, REQ-CLI-ERR-009

## Computed value

### REQ-BINARY-CALC-001
**Requirement:** The bit string of the input MUST be written most
significant bit first, each bit as `0` or `1`, and host bits set in the
input address MUST NOT affect it. For both IPv4 and IPv6 it MUST be
all width bits of the network address (REQ-INFO-CALC-001), its host
bits being `0`, so that it is the value of `NETWORK`, or of `ADDRESS`
when p equals the width, in the shell output
(REQ-BINARY-OUTPUT-012).
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary
**Acceptance:** `ipcalc --binary --format=value 192.168.92.6` → `11000000101010000101110000000110`; `ipcalc --binary --format=value 192.168.92.6/22` → `11000000101010000101110000000000`; `ipcalc --binary --format=value 192.168.92.6/32` → same as without `/32`; `ipcalc --binary --format=value 0.0.0.0/0` → 32 `0`s; `ipcalc --binary --format=value 2001:db8::/32` → `00100000000000010000110110111000` followed by 96 `0`s; `ipcalc --binary --format=value 2001:db8::1` → 128 bits ending in `1`; `ipcalc --binary --format=value ::/0` → 128 `0`s — positive ; test: BitsAddressValue, BitsNetworkValue, BitsPrefix32Value, BitsPrefix0Value, BitsNetworkIPv6Value, BitsAddressIPv6Value, BitsPrefix0IPv6Value
**Links:** REQ-BINARY-OUTPUT-001, REQ-BINARY-OUTPUT-002

## Output

### REQ-BINARY-OUTPUT-001
**Requirement:** With `--format=value`, ipcalc MUST print only the bit
string (REQ-BINARY-CALC-001) followed by a newline, for both IPv4 and IPv6, and
exit with status 0, so that scripts can use it. With `--format=shell`
it MUST print, only when p is less than the width, `PREFIX=<p>`, then
the address variables (REQ-BINARY-OUTPUT-012), one `NAME=value` per
line, all unquoted, as their values need no quoting
(REQ-CLI-OUTPUT-005). It MUST NOT print the bit string, which
`NETWORK` or `ADDRESS` already holds.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary (`FORMAT_VALUE`), binary.c:print_vars
**Acceptance:** as REQ-BINARY-CALC-001; `ipcalc --binary --format=shell 192.168.92.6/22` → tests/bits-shell-192.168.92.6-22 (`PREFIX=22`, then the address variables, no `BITS`); `ipcalc --binary --format=shell 192.168.92.6` → only `ADDRESS=11000000101010000101110000000110`; `ipcalc --binary --format=shell ::/0` → `PREFIX=0`, `NETWORK=` 128 `0`s — positive, negative ; test: BitsAddressValue, BitsNetworkValue, BitsNetworkIPv6Value, BitsShell, BitsShellAddress, BitsShellPrefix0IPv6
**Links:** REQ-CLI-OUTPUT-004, REQ-CLI-OUTPUT-013, REQ-BINARY-OUTPUT-012

### REQ-BINARY-OUTPUT-002
**Requirement:** With `--format=json`, or without `--format` with `-j`,
ipcalc MUST print, for both IPv4 and IPv6, a JSON object
(REQ-CLI-OUTPUT-008) whose keys are, only when p is less than the
width, `PREFIX` holding p in decimal, then the address variables
(REQ-BINARY-OUTPUT-012), all as JSON strings, so that the JSON and
shell outputs hold the same values; as in the shell output, there MUST
be no `BITS` key.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary (`FORMAT_JSON`, `output_start`, `output_stop`), binary.c:print_vars (`json_printf`)
**Acceptance:** `ipcalc --binary --format=json 192.168.92.0/22` → tests/bits-json-192.168.92.0-22 (`PREFIX`, `ADDRESS`, `NETMASK`, `NETWORK`, `HOSTMIN`, `HOSTMAX`, `BROADCAST`, one key per line); `ipcalc --binary --format=json 2001:db8::1` → only `ADDRESS`; `ipcalc --binary --format=json 2001:db8:abcd:12::1/56` → tests/bits-json-2001:db8:abcd:12::1-56 (`PREFIX`, `NETWORK`); `ipcalc --binary -j 192.168.92.0/22` → same as `--format=json`; `ipcalc --binary -j --format=shell 10.0.0.0/8` → conflict, exit 1 (REQ-CLI-ERR-009) — positive, negative ; test: BitsJson, BitsJsonIPv6, BitsJsonNetworkIPv6, BitsJsonLegacy, BitsFormatConflict
**Links:** REQ-CLI-OUTPUT-003, REQ-CLI-OUTPUT-008, REQ-CLI-OUTPUT-013, REQ-BINARY-OUTPUT-012

### REQ-BINARY-OUTPUT-003
**Requirement:** With `--format=human`, or without `--format` and
`-j`, ipcalc MUST print a diagram (REQ-BINARY-OUTPUT-004,
REQ-BINARY-OUTPUT-005) whose first line is a title followed by an empty
line: `Address:` padded with spaces to 12 columns and `<address>` when
p equals the width, and `Network:` padded likewise and `<network>/<p>`
otherwise, where `<address>` and `<network>` are printed as in the info
summary (for IPv6, in compressed form), so that the title states how
the input was interpreted, in the `Label: value` form of the summary,
and its value lines up with those of the details
(REQ-BINARY-OUTPUT-010). No line MUST report host bits set in the
input, as the title already shows the network the input was reduced
to; the input address itself is shown only in the IPv4 details.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary, binary.c:print_diagram
**Acceptance:** `ipcalc --binary 192.168.92.6` → title `Address:    192.168.92.6`; `ipcalc --binary 192.168.92.6/22` → title `Network:    192.168.92.0/22`, no line before it; `ipcalc --binary 192.168.1.4/31` → title `Network:    192.168.1.4/31`; `ipcalc --binary 2001:db8::1` → title `Address:    2001:db8::1`; `ipcalc --binary 2001:db8:abcd:12::1/56` → title `Network:    2001:db8:abcd::/56`, no `Address` line; `ipcalc --binary --format=human 192.168.92.6/22` → same as without `--format` — positive ; test: BitsAddress, BitsHostBits, BitsPrefix31, BitsAddressIPv6, BitsHostBitsIPv6, BitsFormatHuman
**Links:** REQ-INFO-CALC-001

### REQ-BINARY-OUTPUT-004
**Requirement:** For IPv4, the diagram MUST print after the title
(REQ-BINARY-OUTPUT-003) the following lines, in which each octet takes 8
columns and consecutive octets are separated by a `.` column, so that
the lines read as dotted-quad addresses, line up, and are 35 columns
wide whatever the prefix; when the details follow (p less than 32),
each line MUST be indented by 29 spaces, so that its bits line up with
those of the details (REQ-BINARY-OUTPUT-010):
1. each octet's decimal value in the network address or, when the
   octet has host bits, the range `<low>-<high>` of values it takes in
   the network, right-aligned in its columns;
2. each octet's bits, network bits as `0`/`1` and host bits as `x`;
3. only when p is less than 32, the network/host brackets
   (REQ-BINARY-OUTPUT-008), followed by the details
   (REQ-BINARY-OUTPUT-010).
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary, binary.c:print_band
**Acceptance:** `ipcalc --binary 192.168.92.6/22` → tests/bits-192.168.92.6-22 (`   92-95.`, `010111xx.`, indented); `ipcalc --binary 192.168.92.6` → tests/bits-192.168.92.6 (no indentation, no bracket line, no details); `ipcalc --binary 10.0.0.0/8` → tests/bits-10.0.0.0-8; `ipcalc --binary 0.0.0.0/0` → all `0-255` and `x` — positive ; test: BitsHostBits, BitsAddress, BitsPrefix8, BitsPrefix0
**Links:** REQ-BINARY-OUTPUT-006, REQ-BINARY-OUTPUT-008, REQ-BINARY-OUTPUT-010

### REQ-BINARY-OUTPUT-005
**Requirement:** For IPv6, the diagram MUST print after the title
(REQ-BINARY-OUTPUT-003) the bits in bands, each made of the lines of
REQ-BINARY-OUTPUT-004 with hextets in place of octets: each hextet takes
16 columns and consecutive hextets are separated by a `:` column, its
value is four lowercase hex digits or a range of two such values, and
the brackets (REQ-BINARY-OUTPUT-008) are printed only in the band where
the prefix ends and only when p is less than 128. When the output width
(REQ-BINARY-OUTPUT-009) is at least 135 columns, the diagram MUST be a
single band of bits 0–127 (135 columns), so that the address reads on
one line. Otherwise it MUST be two bands of bits 0–63 and bits 64–127
(67 columns each), with the brackets in the first band when p is at
most 64 and in the second otherwise, and the second band MUST be
replaced by a single line when all its bits are host bits:
`bits 64-127: interface ID, all x` for a structured network
(REQ-BINARY-OUTPUT-011), and `bits 64-127: host, all x` otherwise.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:show_binary, binary.c:print_diagram, binary.c:print_band
**Acceptance:** `ipcalc --binary 2001:db8::1` → tests/bits-2001:db8::1 (two bands, no brackets); `ipcalc --binary 2001:db8:abcd:12::1/56` → tests/bits-2001:db8:abcd:12::1-56 (brackets in the first band, then `bits 64-127: interface ID, all x`); `ipcalc --binary fe80::/64` → `bits 64-127: host, all x`; `ipcalc --binary 2001:db8::/64` → network bracket over the whole first band; `ipcalc --binary 2001:db8:abcd:10::/62` → hextet `0010-0013`; `ipcalc --binary 2001:db8::/96` → brackets in the second band; `ipcalc --binary ::/0` → a first band of `0000-ffff` and `x`, then `bits 64-127: host, all x`; with `COLUMNS=135`: `ipcalc --binary 2001:db8::1` → tests/bits-wide-2001:db8::1 (one band, no brackets), `ipcalc --binary 2001:db8:abcd:12::1/56` → tests/bits-wide-2001:db8:abcd:12::1-56 (one band with brackets, no `bits 64-127` line), `ipcalc --binary 2001:db8::/96` → tests/bits-wide-2001:db8::-96, `ipcalc --binary ::/0` → tests/bits-wide-::-0 (host segment only); with `COLUMNS=134`: `ipcalc --binary 2001:db8:abcd:12::1/56` → two bands — positive, negative ; test: BitsAddressIPv6, BitsHostBitsIPv6, BitsLinkLocalIPv6, BitsPrefix64IPv6, BitsPrefix62IPv6, BitsPrefix96IPv6, BitsPrefix0IPv6, BitsWideAddressIPv6, BitsWideHostBitsIPv6, BitsWidePrefix96IPv6, BitsWidePrefix0IPv6, BitsNarrowIPv6
**Links:** REQ-BINARY-OUTPUT-004, REQ-BINARY-OUTPUT-006, REQ-BINARY-OUTPUT-008, REQ-BINARY-OUTPUT-009

### REQ-BINARY-OUTPUT-006
**Requirement:** The brackets MUST be drawn with the UTF-8 box drawing
characters `└ ─ ┘`, regardless of the locale. When colors are enabled
(REQ-CLI-OUTPUT-007), network bits, values made only of network bits,
and the network bracket MUST be printed in blue, and host bits,
ranges, the host, subnet ID and interface ID brackets and the
`bits 64-127` line dimmed; in the details (REQ-BINARY-OUTPUT-010),
network bits MUST be printed in blue and nothing else colored, so that
the host bits, which differ between the rows, stand out; the title
and the `.`/`:` separators MUST NOT be colored. The output MUST be
readable without color, so that it remains complete when piped or with
`NO_COLOR`.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c (`print_line`, `style_color`); ipcalc.c:color_fputs
**Acceptance:** any decorated test above → no escape codes (stdout not a TTY); `script -qc 'ipcalc --binary 192.168.92.0/22'` → blue network bits, dim host bits — positive ; test: BitsHostBits (no TTY test exists)
**Links:** REQ-CLI-OUTPUT-007

### REQ-BINARY-OUTPUT-007
**Requirement:** ipcalc MUST print the bits only in binary mode; the info
summary, including `--all-info`, MUST NOT contain them in any format,
so that the summary stays as it is and the bits are shown only when
requested.
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** ipcalc.c:main (`APP_SHOW_BITS` calling `show_binary()` instead of `show_info_fields()`)
**Acceptance:** `ipcalc --all-info 192.168.92.6/22`, `ipcalc --all-info --format=json 192.168.92.6/22`, `ipcalc --all-info --format=shell 2001:db8::1/56` → no `BITS` key or bits line — negative ; test: AllInfoNoBits, AllInfoNoBitsJson, AllInfoNoBitsShell
**Links:** REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003

### REQ-BINARY-OUTPUT-008
**Requirement:** The brackets line MUST mark under the bits line
(REQ-BINARY-OUTPUT-004 line 2) the columns of each segment of the
address: for a structured network with p at most 64, the segments of
REQ-BINARY-OUTPUT-011; otherwise a network segment of the network bits
and a host segment of the host bits. A separator column belongs to a
segment only when the bits on both of its sides do, so that two
segments are adjacent when their boundary is inside a group and a blank
column apart when it is between groups. A segment MUST be drawn as
`└`, a fill of `─` holding its label, and `┘`; its `└` MUST be omitted
when its bits begin in the previous band and its `┘` when they continue
in the next band, so that an open end shows that the segment goes on.
The label MUST be the first of the segment's labels that fits between
its ends — with a space on each side when there is room — centered,
with any odd fill on the right; the labels are `network /<p>`, `/<p>`,
`<p>` for the network segment and `host <n>`, `<n>` for the host
segment, n being the width minus p. When none fits, the segment MUST be
only its last label, centered, or, when even that does not fit, only
its ends, so that /1, /31 and similar prefixes still render. There is
no network segment for /0. The line MUST NOT end with spaces.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:print_brackets, binary.c:put_segment
**Acceptance:** `ipcalc --binary 192.168.92.6/22` → `└──── network /22 ─────┘└ host 10 ┘`; `ipcalc --binary 10.0.0.0/8` → `└─ /8 ─┘ └─────── host 24 ────────┘`; `ipcalc --binary 192.168.1.4/31` → host segment `1`; `ipcalc --binary 128.0.0.0/1` → network segment `1`; `ipcalc --binary 0.0.0.0/0` → only the host segment; `ipcalc --binary fe80::/10` → host segment `└────── host 118` without `┘`; `ipcalc --binary 2001:db8::/96` → network segment without `└`; `ipcalc --binary 2001:db8::/65` → network segment `┘`; `ipcalc --binary 2001:db8::/127` → host segment `1` — positive ; test: BitsHostBits, BitsPrefix8, BitsPrefix31, BitsPrefix1, BitsPrefix0, BitsLinkLocalPrefix10IPv6, BitsPrefix96IPv6, BitsPrefix65IPv6, BitsPrefix127IPv6
**Links:** REQ-BINARY-OUTPUT-004, REQ-BINARY-OUTPUT-005, REQ-BINARY-OUTPUT-006, REQ-BINARY-OUTPUT-011

### REQ-BINARY-OUTPUT-009
**Requirement:** The output width of the diagram MUST be the value of
the `COLUMNS` environment variable when it is a positive integer;
otherwise, when stdout is a terminal, the terminal's width in columns;
otherwise 80. A `COLUMNS` value that is not a positive integer MUST be
ignored. The width MUST affect only the IPv6 diagram
(REQ-BINARY-OUTPUT-005): the IPv4 diagram and the value, shell and JSON
formats MUST be the same whatever the width, so that piped output does
not depend on the terminal.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:output_columns, binary.c:show_binary; ipcalc.1.md#Environment (`COLUMNS`)
**Acceptance:** `COLUMNS=135 ipcalc --binary 2001:db8::1` → one band; `COLUMNS=abc ipcalc --binary 2001:db8:abcd:12::1/56` and `COLUMNS=0 …` → two bands, as with stdout not a terminal; `COLUMNS=200 ipcalc --binary 192.168.92.6/22` → tests/bits-192.168.92.6-22; the value and shell tests of REQ-BINARY-OUTPUT-001, run with `COLUMNS=200` → unchanged — positive, negative ; test: BitsWideAddressIPv6, BitsColumnsInvalid, BitsColumnsZero, BitsWideIPv4, BitsNetworkIPv6Value, BitsAddressIPv6Value, BitsShellPrefix0IPv6
**Links:** REQ-BINARY-OUTPUT-005

### REQ-BINARY-OUTPUT-010
**Requirement:** For IPv4 with p less than 32, the diagram MUST be
followed by an empty line and the details: one line per row below,
each made of the row's name and a `:` padded with spaces to 12
columns, the row's value in dotted-quad notation padded with spaces to
17 columns, and the row's 32 bits laid out as the bits line
(REQ-BINARY-OUTPUT-004 line 2) but with every bit as `0`/`1`, so that
each line reads as `Label: value` like the info summary and each
row's bits line up with the diagram and its brackets. The rows MUST be, in this order: `Address` (the
input address) only when it differs from the network address,
`Netmask`, `Network` (the network address), `HostMin`, `HostMax` and
`Broadcast` (all host bits 1), where HostMin and HostMax are the first
and last address of the network after the network and broadcast
addresses, so that the rows after Netmask rise in value; for /31 the
rows after Netmask MUST be only `HostMin` (the first address) and
`HostMax` (the second), as both of its addresses are hosts (RFC 3021).
For p equal to 32 there MUST be no
details. For IPv6 there MUST be no details, as IPv6 reserves no
network or broadcast address and uses no netmasks; its diagram marks
the structure of the address instead (REQ-BINARY-OUTPUT-011).
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:print_details, binary.c:make_rows
**Acceptance:** `ipcalc --binary 192.168.92.6/24` → tests/bits-192.168.92.6-24 (Address, Netmask, Network, HostMin, HostMax, Broadcast); `ipcalc --binary 10.0.0.0/8` → tests/bits-10.0.0.0-8 (no Address); `ipcalc --binary 192.168.92.1/24`, `ipcalc --binary 192.168.92.255/24` → Address first; `ipcalc --binary 192.168.1.5/31` → Address, Netmask, HostMin, HostMax; `ipcalc --binary 192.168.92.6/32` and `ipcalc --binary 192.168.92.6` → no details; `ipcalc --binary 2001:db8::/64` → no details — positive, negative ; test: BitsDetails, BitsPrefix8, BitsAddressHostMin, BitsAddressBroadcast, BitsAddressPrefix31, BitsPrefix32, BitsAddress, BitsPrefix64IPv6
**Links:** REQ-BINARY-OUTPUT-004, REQ-BINARY-OUTPUT-006, REQ-INFO-CALC-001

### REQ-BINARY-OUTPUT-011
**Requirement:** For a structured network with p at most 64, the
brackets (REQ-BINARY-OUTPUT-008) MUST name the parts of the address
instead of the network and host: with p less than 64, a routing prefix
segment of bits 0 to p−1 labeled `routing prefix /<p>`, `/<p>` or
`<p>`, a subnet ID segment of bits p to 63 labeled `subnet ID <s>` or
`<s>`, s being 64 minus p, and an interface ID segment of bits 64 to
127 labeled `interface ID 64` or `64`; with p equal to 64, a subnet
prefix segment of bits 0 to 63 labeled `subnet prefix /64`, `/64` or
`64`, and the interface ID segment. The routing prefix and subnet
prefix segments MUST be styled as network segments and the subnet ID
and interface ID segments as host segments (REQ-BINARY-OUTPUT-006).
Networks that are not structured — such as link-local, whose bits after
`fe80::/10` are zero, and multicast, which has no interface ID — and
networks with p greater than 64, where the host bits are fewer than the
interface ID, MUST keep the network and host segments.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:print_brackets, binary.c:is_structured
**Acceptance:** `ipcalc --binary 2001:db8:abcd:12::1/56` → tests/bits-2001:db8:abcd:12::1-56 (`routing prefix /56`, subnet ID `8`); `ipcalc --binary 2001:db8::/64` → `subnet prefix /64`; `ipcalc --binary fd10:da05:b5ac::/48` → `routing prefix /48`, `subnet ID 16`; `COLUMNS=135 ipcalc --binary 2001:db8:abcd:12::1/56` → also `interface ID 64`; `ipcalc --binary fe80::/64`, `ipcalc --binary ff02::/16`, `ipcalc --binary 2001:db8::/96` → network and host segments — positive, negative ; test: BitsHostBitsIPv6, BitsPrefix64IPv6, BitsUniqueLocalIPv6, BitsWideHostBitsIPv6, BitsLinkLocalIPv6, BitsMulticastIPv6, BitsPrefix96IPv6
**Links:** REQ-BINARY-OUTPUT-005, REQ-BINARY-OUTPUT-008

### REQ-BINARY-OUTPUT-012
**Requirement:** The address variables of the shell and JSON outputs
(REQ-BINARY-OUTPUT-001, REQ-BINARY-OUTPUT-002) MUST each hold the
width bits of an address, most significant bit first, as `0`/`1`
without separators, so that they show the addresses of the diagram
(REQ-BINARY-OUTPUT-003, REQ-BINARY-OUTPUT-010) as bits. They MUST be,
in this order:
1. when p equals the width, for both IPv4 and IPv6, only `ADDRESS`
   (the input address);
2. for IPv4 with p less than 32, the rows of the details
   (REQ-BINARY-OUTPUT-010) in their order, named `ADDRESS`, `NETMASK`,
   `NETWORK`, `HOSTMIN`, `HOSTMAX` and `BROADCAST`, except that
   `ADDRESS` MUST be printed even when the input address is the network
   address, so that a script finds it whatever the input; for /31 they
   are thus `ADDRESS`, `NETMASK`, `HOSTMIN` and `HOSTMAX`;
3. for IPv6 with p less than 128, only `NETWORK` (the network
   address, including its host bits, which are `0`): no `ADDRESS`, as
   the diagram does not show the input address, and no `NETMASK`, as
   IPv6 uses no netmasks and `PREFIX` already gives p.
**Strength:** MUST
**Status:** DERIVED
**Source:** binary.c:print_vars, binary.c:make_rows
**Acceptance:** `ipcalc --binary --format=shell 192.168.92.6/22` → tests/bits-shell-192.168.92.6-22; `ipcalc --binary --format=shell 10.0.0.0/8` → `ADDRESS` equal to `NETWORK`; `ipcalc --binary --format=shell 192.168.1.5/31` → tests/bits-shell-192.168.1.5-31 (no `NETWORK` or `BROADCAST`); `ipcalc --binary --format=shell 192.168.92.6` → only `ADDRESS`; `ipcalc --binary --format=shell 2001:db8:abcd:12::1/56` → tests/bits-shell-2001:db8:abcd:12::1-56 (`PREFIX`, `NETWORK` of 128 bits, no `ADDRESS` or `NETMASK`); `ipcalc --binary --format=shell 2001:db8::1` → only `ADDRESS`, of 128 bits — positive, negative ; test: BitsShell, BitsShellPrefix8, BitsShellPrefix31, BitsShellAddress, BitsShellIPv6, BitsShellAddressIPv6, BitsShellPrefix0IPv6
**Links:** REQ-BINARY-OUTPUT-001, REQ-BINARY-OUTPUT-002, REQ-BINARY-OUTPUT-010

## Open items

Missing tests: color output (REQ-BINARY-OUTPUT-006), as for all modes; the
terminal width when `COLUMNS` is unset (REQ-BINARY-OUTPUT-009), as tests
do not run on a terminal.
