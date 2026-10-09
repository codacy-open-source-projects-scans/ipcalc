---
title: Network splitting (-S/--split, --split-hosts)
id-prefix: REQ-SPLIT
sources:
  - netsplit.c (show_split_networks)
  - ipcalc.c (main, parse_split_req, parse_split_hosts, str_to_prefix)
  - ipcalc.1.md
  - tests/meson.build
---

# Network splitting (`-S/--split`, `--split-hosts`)

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
The base network is parsed as in `info.md` (REQ-INFO-INPUT-001 to
REQ-INFO-INPUT-005).

Terms used below: a **request** is one `-S` argument, `[COUNT:]PREFIX`,
asking for COUNT subnets of /PREFIX; a **counted request** has a COUNT;
the **fill request** has none and takes the space that remains; an
**equal split** is a split with a single fill request (e.g. `-S 26`);
a **VLSM split** is any other split, i.e. one with counted requests;
a **host-sized request** is one item of `--split-hosts`, asking for one
subnet that holds a number of hosts, and is allocated as a counted
request with a COUNT of 1; **unused space** is the part of the base
network that no request is allocated.

### REQ-SPLIT-INPUT-001
**Requirement:** `-S` MUST take an argument of the form
`[COUNT:]PREFIX` and MAY be repeated, each occurrence being one
request. PREFIX MUST be accepted as an integer, or for IPv4 as a
dotted-decimal netmask, and a PREFIX that is invalid for the family
(REQ-INFO-INPUT-003) MUST be rejected with
`ipcalc: bad IPv4 prefix: <arg>` / `ipcalc: bad IPv6 prefix: <arg>`,
where `<arg>` is the whole `-S` argument, and exit status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case 'S'` collects every argument, `case APP_SPLIT`), ipcalc.c:parse_split_req (`str_to_prefix(flags, prefixStr, 1)`); ipcalc.1.md#Options (`--split`)
**Acceptance:** `ipcalc -S 26 192.168.5.45/24` → tests/split-192.168.5.45-24-26; `ipcalc -S 255.255.255.192 192.168.5.0/24` → four /26 networks; `ipcalc -S 2:255.255.255.192 192.168.1.0/24` → two /26 networks; `ipcalc -S 1:abc 192.168.0.0/24` → `ipcalc: bad IPv4 prefix: 1:abc`, exit 1; `ipcalc -S 129 2001:db8::/64` → exit 1 — positive, negative ; test: SplitPrefix26, SplitVLSMNetmask, SplitVLSMBadPrefix (no test of the netmask form without a count or of an out-of-range prefix)
**Links:** REQ-SPLIT-INPUT-003

### REQ-SPLIT-INPUT-002
**Requirement:** ipcalc MUST reject a split prefix greater than 32 for
an IPv4 network with a message stating that the prefix is invalid for
IPv4, and exit with status 1.
**Strength:** MUST
**Status:** REVIEW
**Source:** ipcalc.c:str_to_prefix (`fix` sets `FLAG_IPV6` when the prefix exceeds 32), netsplit.c:show_split_networks (then parses the IPv4 network as IPv6)
**Acceptance:** `ipcalc -S 33 192.168.1.0/24` → `ipcalc: bad IPv4 prefix: 33`, exit 1 — negative ; test: none
**Links:** REQ-CLI-INPUT-005

> [REVIEW: defect] The input is rejected (exit 1), but via the IPv6 code
> path, with the misleading `ipcalc: bad IPv6 address: 192.168.1.0`.

### REQ-SPLIT-INPUT-003
**Requirement:** The COUNT of a `-S` request MUST be a decimal integer
from 1 to 2^64−1 made of digits only; ipcalc MUST reject a COUNT of 0,
one that is empty or has a non-digit character, or one that does not
fit in 64 bits, with `ipcalc: bad split count: <arg>` and exit status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:parse_split_req (`isdigit`, `strtoull`, `end != sep`, `errno`, `count == 0`)
**Acceptance:** `ipcalc -S 1:28 -S 2:27 192.168.0.0/24` → accepted; `ipcalc -S 0:26 192.168.0.0/24`, `ipcalc -S x:26 192.168.0.0/24`, `ipcalc -S 99999999999999999999:26 192.168.0.0/24` → `ipcalc: bad split count: <arg>`, exit 1 — positive, negative ; test: SplitVLSMUnused, SplitVLSMZeroCount, SplitVLSMBadCount, SplitVLSMCountOverflow
**Links:** REQ-SPLIT-INPUT-001

### REQ-SPLIT-INPUT-004
**Requirement:** `--split-hosts` MUST take a comma-separated list of
host counts and MAY be repeated, the lists being concatenated in the
order given; each count is one host-sized request for the smallest
subnet with at least that many usable hosts — for IPv4 2^n ≥ count + 2
(network and broadcast addresses included, so the subnet is at most a
/30), for IPv6 2^n ≥ count. Each count MUST be a decimal integer of
digits only from 1 to 2^64−1; ipcalc MUST reject a count of 0, an empty
item, a non-digit character, or a value that does not fit in 64 bits
with `ipcalc: bad host count: <arg>`, where `<arg>` is the whole
argument, and an IPv4 count above 4294967294 with
`ipcalc: too many hosts requested: <count>`; both with exit status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case OPT_SPLIT_HOSTS`), ipcalc.c:parse_split_hosts; ipcalc.1.md#Options (`--split-hosts`)
**Acceptance:** `ipcalc --split-hosts=10,20,30 192.168.0.0/24` → a /28 and two /27; `ipcalc --split-hosts=1,2,62,63 192.168.0.0/24` → /30, /30, /26, /25; `ipcalc --split-hosts=1,2 --split-hosts=62,63 192.168.0.0/24` → same; `ipcalc --split-hosts=1,2,3,256 2001:db8::/112` → /128, /127, /126, /120; `ipcalc -j --split-hosts=4294967294 0.0.0.0/0` → `0.0.0.0/0`; `ipcalc --split-hosts=0 …`, `--split-hosts=1,,2 …`, `--split-hosts=18446744073709551616 2001:db8::/64` → `ipcalc: bad host count: <arg>`, exit 1; `ipcalc --split-hosts=4294967295 0.0.0.0/0` → `ipcalc: too many hosts requested: 4294967295`, exit 1 — positive, negative ; test: SplitHosts, SplitHostsSmallest, SplitHostsRepeated, SplitHostsIPv6, JsonSplitHostsWholeIPv4, SplitHostsZero, SplitHostsEmptyItem, SplitHostsOverflow, SplitHostsTooManyHosts
**Links:** REQ-SPLIT-CALC-002, REQ-SPLIT-OUTPUT-004

> A host-sized IPv6 request is at most a /64, since counts are limited
> to 2^64−1.

### REQ-SPLIT-ERR-001
**Requirement:** ipcalc MUST reject a request whose prefix is shorter
than the base network's prefix with
`Cannot subnet to /<split> with this base network, use a prefix > /<base>`
and exit status 1, and MUST accept a prefix equal to the base prefix
(producing the base network itself).
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`reqs[i].prefix < info->prefix`)
**Acceptance:** `ipcalc -S 23 192.168.1.0/24`, `ipcalc -S 1:23 192.168.1.0/24` → exit 1; `ipcalc -S 24 192.168.1.0/24`, `ipcalc -S 1:24 192.168.1.0/24` → one network, exit 0 — negative, positive ; test: none

> [REVIEW] The message says "use a prefix > /N" but a prefix equal to N
> is accepted; it should say ">=".

### REQ-SPLIT-ERR-002
**Requirement:** ipcalc MUST reject more than one request without a
COUNT with `ipcalc: only one split prefix may be given without a count`
and exit status 1, so that the space left after the counted requests
has a single, unambiguous prefix.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case APP_SPLIT`, `nfill > 1`)
**Acceptance:** `ipcalc -S 26 -S 27 192.168.0.0/24` → exit 1 — negative ; test: SplitVLSMTwoFills

### REQ-SPLIT-ERR-003
**Requirement:** When the requests do not fit in the base network —
the counted or host-sized requests need more addresses than it has
(including a single host-sized request larger than the network), or a
fill request is given and no aligned subnet of its prefix remains
after the counted requests — ipcalc MUST print nothing to stdout,
print `ipcalc: <network>/<prefix> is too small for the requested subnets`
to stderr, and exit with status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`too_small()`, called before any output; for host-sized requests also when `reqs[i].prefix < info->prefix`)
**Acceptance:** `ipcalc -S 1:25 -S 1:25 -S 1:26 192.168.0.0/24`, `ipcalc -S 2:25 -S 26 192.168.0.0/24`, `ipcalc --split-hosts=200,100 192.168.0.0/24`, `ipcalc --split-hosts=300 192.168.0.0/24` → exit 1, only the message — negative ; test: SplitVLSMTooSmall, SplitVLSMNoSpaceForFill, SplitHostsTooSmall, SplitHostsLargerThanNetwork
**Links:** REQ-SPLIT-CALC-002, REQ-SPLIT-CALC-003

### REQ-SPLIT-ERR-004
**Requirement:** ipcalc MUST reject `--split-hosts` together with `-S`
with `ipcalc: --split-hosts cannot be combined with --split` and exit
status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case APP_SPLIT`, `nsplits && nsplitHosts`)
**Acceptance:** `ipcalc --split-hosts=10 -S 26 192.168.0.0/24` → exit 1 — negative ; test: SplitHostsWithSplit
**Links:** REQ-CLI-ERR-001

### REQ-SPLIT-CALC-001
**Requirement:** In an equal split, ipcalc MUST list, in ascending
address order, every subnet of the given prefix contained in the base
network — exactly 2^(split − base) subnets, starting at the base
network address — for both IPv4 and IPv6, including networks at either
end of the address space.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks
**Acceptance:** `ipcalc -S 18 10.10.10.10/16` → tests/split-10.10.10.0-16-18 (4 networks from 10.10.0.0); `ipcalc -S 120 fcfa:b4ca:f1d8:125b:dc00::/112` → 256 networks; `ipcalc -S 2 ::/0` → `::/2`, `4000::/2`, `8000::/2`, `c000::/2`; `ipcalc -S 124 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/120` → 16 networks from `ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00/124` — positive ; test: SplitPrefix18, SplitPrefix24, SplitPrefix26, SplitPrefix29, SplitPrefix31, SplitPrefix32, SplitPrefix64, SplitPrefix120, SplitPrefix128, SplitPrefix0IPv4, SplitPrefix3Of1, SplitPrefix2Of0IPv6, SplitPrefix124EndOfSpace, SplitLinesByNetworkIPv4, SplitLinesByNetworkIPv6
**Links:** REQ-SPLIT-OUTPUT-001

### REQ-SPLIT-CALC-002
**Requirement:** ipcalc MUST allocate the counted (or host-sized) requests back to back
from the base network address, largest subnets first and, among
requests of the same prefix, in the order given, so that every subnet
is aligned to its size and no space is lost between them. This is the
layout of the split of the Debian ipcalc.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`order[]` insertion sort by prefix, `cursor`)
**Acceptance:** `ipcalc -S 1:28 -S 2:27 192.168.0.0/24` → `192.168.0.64/28`, `192.168.0.0/27`, `192.168.0.32/27`; `ipcalc -S 1:26 -S 1:25 -S 27 192.168.0.0/24` → the /25 at `192.168.0.0`, the /26 at `192.168.0.128` — positive ; test: SplitVLSMUnused, SplitVLSMPrefix58And64, SplitVLSMWholeSpace, SplitHosts, SplitHostsExactFit
**Links:** REQ-SPLIT-OUTPUT-001, REQ-SPLIT-ERR-003

### REQ-SPLIT-CALC-003
**Requirement:** ipcalc MUST allocate the fill request as all subnets
of its prefix from the first address after the counted requests that
is aligned to that prefix up to the end of the base network; the space
skipped to reach that alignment is unused space.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (fill request after the counted ones, `gap`)
**Acceptance:** `ipcalc -S 1:58 -S 64 2001:db8:1c88:6000::/56` → one /58 and 192 /64 networks; `ipcalc -S 3:64 -S 60 2001:db8::/56` → three /64, then fifteen /60 from `2001:db8:0:10::/60`, unused `2001:db8:0:3::/64`, `2001:db8:0:4::/62`, `2001:db8:0:8::/61`; `ipcalc -S 2:26 -S 26 192.168.1.0/24` → four /26 networks — positive ; test: SplitVLSMPrefix58And64, SplitVLSMFillAligned, SplitVLSMSamePrefix, SplitVLSMWholeSpace
**Links:** REQ-SPLIT-OUTPUT-003, REQ-SPLIT-ERR-003

### REQ-SPLIT-OUTPUT-001
**Requirement:** Except for the decorated output of host-sized
requests (REQ-SPLIT-OUTPUT-004), the split output MUST list the subnets of each request
in the order the requests are given, each request's subnets in
ascending address order, as: decorated — a `[Split networks]` header,
one `Network:\t<net>/<prefix>` line per subnet, a blank line,
`Total:  \t<count>` with the number of subnets of all requests, and,
only when every request has the same prefix, `Hosts/Net:\t<hosts>`;
with `--no-decorate` — only one `<net>/<prefix>` line per subnet; with
`-j` — an object with `SPLITNETWORK` (array of `<net>/<prefix>`
strings), `NETS`, and, under the same condition as `Hosts/Net`,
`ADDRESSES`.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`array_start(…, "Split networks", "SPLITNETWORK")`, `default_printf`, `dist_printf(… "NETS" …)`, `same_prefix`, `dist_printf(… "ADDRESSES" …)`)
**Acceptance:** `ipcalc -S 26 192.168.5.45/24`, `ipcalc --no-decorate -S 26 192.168.5.45/24`, `ipcalc -j -S 26 192.168.5.45/24` → tests/split-…, tests/nsplit-…, tests/json-split-192.168.5.45-24-26; `ipcalc -S 1:28 -S 2:27 192.168.0.0/24` → `Total: 3` and no `Hosts/Net`; `ipcalc -S 2:26 -S 26 192.168.1.0/24` → `Hosts/Net: 62` — positive ; test: SplitPrefix26, NoDecorateSplitPrefix26, JsonSplitPrefix26, JsonNoDecorateSplitPrefix26, the other Split/NoDecorateSplit/JsonSplit tests, SplitVLSMUnused, JsonSplitVLSMUnused, SplitVLSMSamePrefix
**Links:** REQ-CLI-OUTPUT-006, REQ-CLI-OUTPUT-008

### REQ-SPLIT-OUTPUT-002
**Requirement:** The split `Hosts/Net`/`ADDRESSES` value MUST be the host
count of one subnet of the split prefix, as defined by
REQ-INFO-CALC-004, printed in full for every prefix.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`ipv4_prefix_to_hosts`, `ipv6_prefix_to_hosts`)
**Acceptance:** `ipcalc -S 26 192.168.5.45/24` → `Hosts/Net: 62`; `ipcalc -S 0 0.0.0.0/0` → `Hosts/Net: 4294967294`; `ipcalc -S 3 10.0.0.0/1` → `Hosts/Net: 536870910`; `ipcalc -S 2 ::/0` → `Hosts/Net: 85070591730234615865843651857942052864` — positive ; test: SplitPrefix26, SplitPrefix0IPv4, SplitPrefix3Of1, SplitPrefix2Of0IPv6
**Links:** REQ-INFO-CALC-004

### REQ-SPLIT-OUTPUT-003
**Requirement:** When part of the base network is unused space, ipcalc
MUST print it after the split networks as the minimal list of CIDR
networks that covers it, in ascending address order: decorated — a
blank line, an `[Unused networks]` header, and one
`Network:\t<net>/<prefix>` line per network; with `-j` — an `UNUSED`
array of `<net>/<prefix>` strings. ipcalc MUST NOT print unused space
with `--no-decorate` (without `-j`), nor print the section or key when
there is no unused space.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:show_split_networks (`has_gap`, `array_start(…, "Unused networks", "UNUSED")`), netsplit.c:print_range
**Acceptance:** `ipcalc -S 1:28 -S 2:27 192.168.0.0/24` → unused `192.168.0.80/28`, `192.168.0.96/27`, `192.168.0.128/25`; same with `-j` → `"UNUSED"` array; same with `--no-decorate` → only the three split networks; `ipcalc -S 26 192.168.0.0/24` → no unused section — positive, negative ; test: SplitVLSMUnused, JsonSplitVLSMUnused, NoDecorateSplitVLSMUnused, SplitVLSMFillAligned, SplitPrefix26
**Links:** REQ-SPLIT-CALC-003, REQ-CLI-OUTPUT-008

### REQ-SPLIT-OUTPUT-004
**Requirement:** For `--split-hosts`, the decorated output MUST replace
each subnet's `Network:` line with a block per request, in request
order and separated by blank lines: `<n>. Requested size: <count> hosts`,
`Network:\t<net>/<prefix>`, `Netmask:\t<mask> = <prefix>`, for IPv4
only `Broadcast:\t<last address>`, `HostMin:\t<min>`, `HostMax:\t<max>`,
and `Hosts/Net:\t<hosts>` — with the host range and count as for
info mode (REQ-INFO-CALC-003, REQ-INFO-CALC-004) for IPv4, and the
first address, last address and address count for IPv6. After `Total:`
(and `Hosts/Net:` per REQ-SPLIT-OUTPUT-001) it MUST print
`Needed size:\t<n>`, the number of addresses from the base network
address to the end of the last allocated subnet, and
`Used network:\t<net>/<prefix>`, the smallest network starting at the
base network address that contains all the subnets, followed by the
unused networks (REQ-SPLIT-OUTPUT-003). With `-j` the output MUST be
that of REQ-SPLIT-OUTPUT-001 with `NEEDED` and `USEDNETWORK` added
before `UNUSED`; with `--no-decorate` (without `-j`) only one
`<net>/<prefix>` line per subnet.
**Strength:** MUST
**Status:** DERIVED
**Source:** netsplit.c:print_host_sized_subnet, netsplit.c:show_split_networks (`host_sized`, `details`, `dist_printf(… "NEEDED" …)`, `dist_printf(… "USEDNETWORK" …)`)
**Acceptance:** `ipcalc --split-hosts=10,20,30 192.168.0.0/24` → tests/split-hosts-192.168.0.0-24-10,20,30 (`Needed size: 80`, `Used network: 192.168.0.0/25`); same with `-j` → `"NEEDED":"80"`, `"USEDNETWORK":"192.168.0.0/25"`; same with `--no-decorate` → three lines; `ipcalc --split-hosts=1,2,3,256 2001:db8::/112` → no `Broadcast:`, `Needed size: 263`, `Used network: 2001:db8::/119` — positive ; test: SplitHosts, JsonSplitHosts, NoDecorateSplitHosts, SplitHostsSmallest, SplitHostsIPv6, JsonSplitHostsWholeIPv4
**Links:** REQ-SPLIT-OUTPUT-001, REQ-SPLIT-OUTPUT-003, REQ-CLI-OUTPUT-008

### REQ-SPLIT-COMPAT-001
**Requirement:** For an IPv4 network that holds the requests and host
counts of 1 or more, `--split-hosts=N1,N2,…` SHOULD produce the same
values as `ipcalc -s N1 N2 …` of the Debian ipcalc (0.51) — the same
subnets in the same request order, with the same netmask, broadcast,
`HostMin`, `HostMax`, and `Hosts/Net`, and the same needed size, used
network, and unused networks — so that users of that tool can switch.
The layout MAY differ: ipcalc prints the fields in its own split
format (REQ-SPLIT-OUTPUT-004), without the Debian tool's leading block
for the base network and its class column. Where the Debian ipcalc
accepts input that ipcalc does not, ipcalc MUST follow its own
requirements: a host count of 0 (Debian: a /31) is rejected per
REQ-SPLIT-INPUT-004, and requests that do not fit (Debian: prints
`Network is too small` and subnets beyond the network) fail per
REQ-SPLIT-ERR-003.
**Strength:** SHOULD
**Status:** DERIVED
**Source:** netsplit.c (header comment, print_host_sized_subnet); ipcalc.1.md#Options (`--split-hosts`); Debian ipcalc 0.51-1 `split_network()` (https://sources.debian.org/src/ipcalc/0.51-1/ipcalc)
**Acceptance:** with the Debian script (tests/debian-ipcalc/ipcalc) run as `ipcalc -n -b <net> -s <counts>`, the values above match for `192.168.0.0/24` with `10,20,30`, `1,2,62,63`, `126,62,62`, `5,5,5`, `30,30,30,30,30,30,30,30`; `192.168.5.45/24` with `10,20`; `192.168.1.0/24` with `254`; `10.0.0.0/30` with `2`; `172.16.0.0/12` with `1`; `10.0.0.0/8` with `1000,5,5,70000,300,2`; `10.0.0.0/16` with `2000,1000,500,250,120,60,30,14,6,2,1`; `10.20.30.0/23` with `100,100,50,3,3,1` — positive ; test: DebianSplitHosts-<net>-<counts> (one per case, registered in the git tree, where the Debian script is present; they need perl and its bignum module and fail without them, per REQ-GEN-TEST-007); SplitHosts, SplitHostsSmallest, SplitHostsExactFit pin the matching values
**Links:** REQ-SPLIT-OUTPUT-004, REQ-SPLIT-INPUT-004, REQ-SPLIT-ERR-003

## Open items

`REVIEW`: REQ-SPLIT-INPUT-002 (IPv4 split prefix > 32 goes through the
IPv6 path), REQ-SPLIT-ERR-001 (message says ">").
Missing tests: netmask-form split prefix without a count, invalid split
prefix, split prefix shorter than or equal to the base.
