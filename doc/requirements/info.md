---
title: Address parsing and computed network information
id-prefix: REQ-INFO
sources:
  - ipcalc.c (get_ipv4_info, get_ipv6_info, str_to_prefix, prefix2mask, calc_broadcast, calc_network, ipv4_net_to_type, ipv6_net_to_type, ipv4_net_to_class, get_ipv6_iid_info, default_ipv4_prefix, ipv4_prefix_to_hosts, ipv6_prefix_to_hosts, expand_ipv6, get_hostname, get_ip_address, main)
  - ipcalc-reverse.c
  - netcompare.c (compare_networks)
  - ipcalc.1.md
  - tests/meson.build
---

# Address parsing and computed network information

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
These requirements apply to info mode, check mode (parsing only),
comparison mode, split
mode (parsing of the base network), and random generation (computation
on the generated network). Output selection and formats are in `cli.md`.

## Input

### REQ-INFO-INPUT-001
**Requirement:** ipcalc MUST accept an IPv4 address only in the
dotted-decimal form accepted by `inet_pton(AF_INET)` — four decimal
octets 0–255, without leading zeros or surrounding whitespace — and MUST
reject any other IPv4 input with `ipcalc: bad IPv4 address: <input>` on
stderr and exit status 1 (2 with a comparison option, REQ-CLI-OUTPUT-009).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv4_info (`inet_pton`)
**Acceptance:** `ipcalc -c 192.168.1.1` → exit 0; `ipcalc -c 1.2.3`, `ipcalc -c 01.2.3.4`, `ipcalc -c -4 2a01:198:200:300::2` → exit 1 — positive, negative ; test: ValidatePrivateIPv4, NoValidateIPv6InIPv4Mode
**Links:** REQ-CLI-INPUT-005, REQ-INFO-INPUT-006

### REQ-INFO-INPUT-002
**Requirement:** ipcalc MUST accept an IPv6 address in any textual form
accepted by `inet_pton(AF_INET6)` (compressed, fully expanded, or with
an embedded IPv4 suffix) and MUST reject any other IPv6 input with
`ipcalc: bad IPv6 address: <input>` and exit status 1 (2 with a
comparison option, REQ-CLI-OUTPUT-009).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv6_info (`inet_pton`)
**Acceptance:** `ipcalc -c -6 2a01:198:200:300::2`, `ipcalc -c -6 2a01:0198:0200:0300:0000:0000:0000:0002` → exit 0; `ipcalc -c -6 gggg::` → exit 1 — positive, negative ; test: ValidateAbbreviatedGlobalIPv6, ValidateCompleteGlobalIPv6, NoValidateInvalidHexIPv6

### REQ-INFO-INPUT-003
**Requirement:** ipcalc MUST accept the network prefix as a `/<prefix>`
suffix of the address or, for IPv4 only, as a second argument; the
prefix MUST be either an integer in 0–32 (IPv4) or 0–128 (IPv6), or,
for IPv4 only, a dotted-decimal netmask whose one-bits are contiguous.
Any other prefix MUST be rejected with
`ipcalc: bad IPv4 prefix: <prefix>` or `ipcalc: bad IPv6 prefix: <prefix>`
and exit status 1 (2 with a comparison option, REQ-CLI-OUTPUT-009).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (prefix extraction), ipcalc.c:str_to_prefix, ipcalc.c:mask2prefix, ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info
**Acceptance:** `ipcalc -p 192.168.1.1 255.255.255.0` → `PREFIX=24`; `ipcalc -p 192.168.1.1 0.0.0.0` → `PREFIX=0`; `ipcalc -n 1.2.3.4/255.255.0.0` → `NETWORK=1.2.0.0`; `ipcalc -m 192.168.1.1/-1`, `ipcalc -m 192.168.1.1/64`, `ipcalc -m 192.168.1.1/99999`, `ipcalc -c ::1/999`, `ipcalc -n 1.2.3.4/255.0.255.0`, `ipcalc -p 1.2.3.4/` → exit 1 — positive, negative ; test: CalculatePrefix24FromNetmask, CalculatePrefix0FromNetmask, NoValidateNegativePrefix, NoValidateIPv6PrefixInIPv4Mode, NoValidateInvalidPrefixIPv4, NoValidateInvalidPrefixIPv6
**Links:** REQ-CLI-INPUT-004

> [AMBIGUOUS] The integer prefix is parsed with `strtol(…, 0)`, so
> `0x18` is accepted as 24 and `010` as **8** (octal). Interpretation A:
> incidental — only decimal is documented and a reimplementation may
> reject hex/octal. Interpretation B: essential — scripts may pass
> zero-padded prefixes, which today silently change meaning (`/010` is
> /8, not /10). B argues for parsing as decimal.

### REQ-INFO-INPUT-004
**Requirement:** ipcalc SHOULD reject an IPv4 input that gives both a
`/prefix` suffix and a netmask argument with a message naming the
conflict.
**Strength:** SHOULD
**Status:** REVIEW
**Source:** ipcalc.c:main (`prefixStr = chptr` before the `/` split; no check for both a prefix and a netmask)
**Acceptance:** `ipcalc -n 1.2.3.4/24 255.255.0.0` → exit 1 with a message about both netmask and prefix — negative ; test: none

> [REVIEW: may be a defect] The input is rejected (exit 1) but with the
> misleading `ipcalc: bad IPv4 address: 1.2.3.4/24`, because the second
> argument takes precedence and the `/24` is left in the address.

### REQ-INFO-INPUT-005
**Requirement:** When no prefix is given, ipcalc MUST use /128 for IPv6
and /32 for IPv4; with `--class-prefix` it MUST instead use the IPv4
classful prefix of the address — /8 when the first octet is below 128,
/16 when below 192, and /24 otherwise. `--class-prefix` MUST have no
effect when a prefix is given or the address is IPv6.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv4_info, ipcalc.c:default_ipv4_prefix, ipcalc.c:get_ipv6_info; ipcalc.1.md#Options (`--netmask`, `--class-prefix`)
**Acceptance:** `ipcalc -m 192.168.1.1` → `NETMASK=255.255.255.255`; `ipcalc --class-prefix -m 10.1.2.3` → `NETMASK=255.0.0.0`; `ipcalc --class-prefix -m 129.22.4.3` → `NETMASK=255.255.0.0`; `ipcalc --class-prefix -m 192.168.1.1` → `NETMASK=255.255.255.0` — positive ; test: CalculateNetmaskNoPrefix, CalculateClassNetmask8, CalculateClassNetmask16, CalculateClassNetmask24, AssignClassPrefix12, AssignClassPrefix129, AssignClassPrefix193

> [REVIEW: contradicts ipcalc.1.md] The `--netmask` entry of the man
> page says that without a prefix IPv4 "assumes that the IP address is in
> a complete class A, B, or C network"; that is only true with
> `--class-prefix` (the default is /32 — test CalculateNetmaskNoPrefix).

### REQ-INFO-INPUT-006
**Requirement:** ipcalc MAY accept an IPv4 address with trailing octets
omitted when a prefix is given (e.g. `172/8` meaning `172.0.0.0/8`).
**Strength:** MAY
**Status:** UNDOCUMENTED
**Source:** ipcalc.c:get_ipv4_info ("Handle CIDR entries such as 172/8" — the padding runs after `inet_pton` has already rejected the short form, so it only affects the string passed to the geo lookup)
**Acceptance:** `ipcalc -n 172/8` → today `ipcalc: bad IPv4 address: 172`, exit 1 — negative ; test: none

> [UNDOCUMENTED] Dead code that suggests an intended feature. Decide
> whether to implement it (then make this a MUST with tests and
> document it) or remove the padding code.

## Computed values

### REQ-INFO-CALC-001
**Requirement:** ipcalc MUST compute the network address as the input
address with all bits beyond the prefix cleared, and the netmask as the
prefix's one-bits, printed in dotted-decimal (IPv4) or compressed IPv6
form (e.g. `ffff:ffff:ffff:ffff::`).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:calc_network, ipcalc.c:prefix2mask, ipcalc.c:get_ipv6_info, ipcalc.c:ipv6_prefix_to_mask
**Acceptance:** `ipcalc -n 192.168.1.1/32` → `NETWORK=192.168.1.1`; `ipcalc -n 192.168.1.1/0` → `NETWORK=0.0.0.0`; `ipcalc -m 172.16.59.222/22` → `NETMASK=255.255.252.0`; `ipcalc -m 10.0.0.0/1` → `NETMASK=128.0.0.0`; `ipcalc --addrspace -abmnp fd95:6be5:0ae0:84a5::/64` → `NETMASK=ffff:ffff:ffff:ffff::` — positive ; test: CalculateNetworkFromPrefix32, CalculateNetworkFromPrefix0, CalculateNetmaskPrefix22, CalculateNetmaskPrefix1, AllocateAddressSpacePrefix64

### REQ-INFO-CALC-002
**Requirement:** For IPv4, ipcalc MUST compute the broadcast address as
the network address with all host bits set, except that for a /31
network it MUST use the limited broadcast address `255.255.255.255`
(RFC 3021). ipcalc MUST NOT compute a broadcast address for IPv6.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:calc_broadcast ("Follow RFC3021"); ipcalc.c:main (`BROADCAST` only `!(flags & FLAG_IPV6)`); ipcalc.h (`broadcast; /* ipv4 only */`)
**Acceptance:** `ipcalc -b 192.168.1.1/24` → `BROADCAST=192.168.1.255`; `ipcalc -abmnp 192.168.1.5/31` → tests/192.168.1.5-31 (`BROADCAST=255.255.255.255`); `ipcalc -b -n 2001:db8::1/64` → no `BROADCAST` line — positive, negative ; test: CalculateBroadcastAddressFromPrefixIPv4, TestPrefix31 (no IPv6 test)

### REQ-INFO-CALC-003
**Requirement:** ipcalc MUST compute the host range (`MINADDR`/`HostMin`
to `MAXADDR`/`HostMax`) as: for IPv4 prefixes 0–30, the network address
plus one to the broadcast address minus one; for IPv4 /31, both
addresses of the network; for IPv6 prefixes 0–127, the network address
to the last address of the network; and for /32 and /128, the input
address for both ends.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info (`hostmin`, `hostmax`)
**Acceptance:** `ipcalc -abmnp --minaddr --maxaddr --addresses 192.168.2.6/24` → `MINADDR=192.168.2.1`, `MAXADDR=192.168.2.254`; `ipcalc --minaddr --maxaddr 192.168.1.4/31` → `.4`, `.5`; `ipcalc -i 2001:db8::/64` → `HostMin: 2001:db8::`, `HostMax: 2001:db8::ffff:ffff:ffff:ffff` — positive ; test: SpecificInfoOutput, TestPrefix31 (no MINADDR/MAXADDR assertion), TestHumanReadableGenericInfoPrefix48

### REQ-INFO-CALC-004
**Requirement:** ipcalc MUST report the host count (`ADDRESSES`,
`Hosts/Net`) as a decimal integer equal to: for IPv4, 2^(32−prefix)−2
for prefixes 0–30, 2 for /31, and 1 for /32; for IPv6, 2^(128−prefix)
for every prefix 0–128.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:ipv4_prefix_to_hosts, ipcalc.c:ipv6_prefix_to_hosts, ipcalc.c:p2_table
**Acceptance:** `ipcalc --addresses 192.168.2.6/24` → `ADDRESSES=254`; `ipcalc --addresses 192.168.1.4/31` → `2`; `ipcalc --addresses 0.0.0.0/0` → `ADDRESSES=4294967294`; `ipcalc --addresses 10.0.0.0/1` → `ADDRESSES=2147483646`; `ipcalc --addresses ::/0` → `ADDRESSES=340282366920938463463374607431768211456` — positive ; test: SpecificInfoOutput, AddressesPrefix0IPv4, AddressesPrefix1IPv4, AddressesPrefix0IPv6 (no /31, /32 test)
**Links:** REQ-SPLIT-OUTPUT-002

### REQ-INFO-CALC-005
**Requirement:** ipcalc MUST print IPv6 addresses in the compressed form
produced by `inet_ntop` (`ADDRESS`, `NETWORK`, `MINADDR`, `MAXADDR`), and
MUST additionally provide the fully expanded form — eight groups of four
lowercase hex digits — as `FULLADDRESS`/`Full Address` and
`FULLNETWORK`/`Full Network` in the summary output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:expand_ipv6, ipcalc.c:get_ipv6_info
**Acceptance:** `ipcalc -i 2a03:2880:20:4f06:face:b00c:0:1` → `Full Address: 2a03:2880:0020:4f06:face:b00c:0000:0001`; `ipcalc -a 2001:0db8::0001` → `ADDRESS=2001:db8::1` — positive ; test: TestHumanReadableGenericInfoNoPrefix, JsonTestHumanReadableGenericInfoPrefix56

### REQ-INFO-CALC-006
**Requirement:** ipcalc MUST classify an IPv4 network's address space
(`ADDRSPACE`) by the most specific (longest-prefix) block in this table
that contains the whole network — that is, the network's prefix is at
least the block's length and its network address lies in the block —
independently of the order of the rows, and as `Internet`
when no row matches. The table follows the IANA IPv4 Special-Purpose
Address Registry (updated 2025-10-09), plus 224.0.0.0/4 from the IANA
IPv4 Address Space Registry:

| Block | ADDRSPACE |
|---|---|
| 0.0.0.0/32 | This host on this network |
| 0.0.0.0/8 | This network |
| 10.0.0.0/8 | Private Use |
| 100.64.0.0/10 | Shared Address Space |
| 127.0.0.0/8 | Loopback |
| 169.254.0.0/16 | Link Local |
| 172.16.0.0/12 | Private Use |
| 192.0.0.0/29 | IPv4 Service Continuity Prefix |
| 192.0.0.8/32 | IPv4 dummy address |
| 192.0.0.9/32 | Port Control Protocol Anycast |
| 192.0.0.10/32 | Traversal Using Relays around NAT Anycast |
| 192.0.0.170/32, 192.0.0.171/32 | NAT64/DNS64 Discovery |
| 192.0.0.0/24 | IETF Protocol Assignments |
| 192.0.2.0/24 | Documentation (TEST-NET-1) |
| 192.31.196.0/24 | AS112-v4 |
| 192.52.193.0/24 | AMT |
| 192.88.99.2/32 | 6a44-relay anycast address |
| 192.88.99.0/24 | 6 to 4 Relay Anycast (Deprecated) |
| 192.168.0.0/16 | Private Use |
| 192.175.48.0/24 | Direct Delegation AS112 Service |
| 198.18.0.0/15 | Benchmarking |
| 198.51.100.0/24 | Documentation (TEST-NET-2) |
| 203.0.113.0/24 | Documentation (TEST-NET-3) |
| 224.0.0.0/4 | Multicast |
| 255.255.255.255/32 | Limited Broadcast |
| 240.0.0.0/4 | Reserved |

**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:ipv4_net_to_type; https://www.iana.org/assignments/iana-ipv4-special-registry
**Acceptance:** `ipcalc -i <addr>` for an address in each row → the listed `Address space`; `ipcalc -i 0.0.0.0` → `This host on this network`; `ipcalc -i 0.0.0.1` → `This network`; `ipcalc -i 192.88.99.2` → `6a44-relay anycast address`; `ipcalc --addrspace 192.168.0.0/8` → `ADDRSPACE=Internet`; `ipcalc --addrspace 10.0.0.0/7` → `ADDRSPACE=Internet`; `ipcalc --addrspace 0.0.0.0/7` → `ADDRSPACE=Internet` — positive, negative ; test: TestHumanReadable* (IPv4 rows), TestHumanReadableThisHostOnThisNetwork, TestHumanReadable6a44Relay, AddrspaceIPv4LargerThanBlock, AddrspaceIPv4LargerThanPrivateBlock, AddrspaceIPv4LargerThanThisNetwork
**Links:** REQ-INFO-CALC-007, REQ-INFO-COMPAT-001

### REQ-INFO-CALC-007
**Requirement:** ipcalc MUST classify an IPv6 network's address space
(`ADDRSPACE`) by the most specific (longest-prefix) block in this table
that contains the whole network, independently of the order of the rows,
and as `Reserved` when no row matches. The table follows
the IANA IPv6 Special-Purpose Address Registry (updated 2025-10-09),
followed by the unicast and multicast blocks of the IANA IPv6 Address
Space Registry:

| Block | ADDRSPACE |
|---|---|
| ::1/128 | Loopback Address |
| ::/128 | Unspecified Address |
| ::ffff:0:0/96 | IPv4-mapped Address |
| 64:ff9b::/96 | IPv4-IPv6 Translat. |
| 64:ff9b:1::/48 | IPv4-IPv6 Translat. |
| 100::/64 | Discard-Only Address Block |
| 100:0:0:1::/64 | Dummy IPv6 Prefix |
| 2001::/32 | TEREDO |
| 2001:1::1/128 | Port Control Protocol Anycast |
| 2001:1::2/128 | Traversal Using Relays around NAT Anycast |
| 2001:1::3/128 | DNS-SD Service Registration Protocol Anycast |
| 2001:2::/48 | Benchmarking |
| 2001:3::/32 | AMT |
| 2001:4:112::/48 | AS112-v6 |
| 2001:10::/28 | Deprecated (previously ORCHID) |
| 2001:20::/28 | ORCHIDv2 |
| 2001:30::/28 | Drone Remote ID Protocol Entity Tags (DETs) Prefix |
| 2001::/23 | IETF Protocol Assignments |
| 2001:db8::/32 | Documentation |
| 2002::/16 | 6to4 |
| 2620:4f:8000::/48 | Direct Delegation AS112 Service |
| 3fff::/20 | Documentation |
| 5f00::/16 | Segment Routing (SRv6) SIDs |
| 2000::/3 | Global Unicast |
| fc00::/7 | Unique Local Unicast |
| fe80::/10 | Link-Scoped Unicast |
| ff00::/8 | Multicast |

**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:ipv6_net_to_type; https://www.iana.org/assignments/iana-ipv6-special-registry; https://www.iana.org/assignments/ipv6-address-space
**Acceptance:** `ipcalc -i <addr>` for an address in each row → the listed `Address space`; `ipcalc -i 100::1` → `Discard-Only Address Block`; `ipcalc -i 1000::1` → `Reserved`; `ipcalc -i 100:0:0:1::5`, `ipcalc -i 2001:1::3`, `ipcalc -i 2001:30::1`, `ipcalc -i 3fff::1`, `ipcalc -i 5f00::1` → their new names; `ipcalc --addrspace 2001:db8::/16` → `"Global Unicast"`; `ipcalc --addrspace 2002::/15` → `"Global Unicast"` — positive, negative ; test: TestHumanReadable* (IPv6 rows), TestHumanReadableDiscardOnly, AddrspaceNotDiscardOnly, TestHumanReadableDummyIPv6Prefix, TestHumanReadableDNSSDSRPAnycast, TestHumanReadableDRIPDET, TestHumanReadableDocumentation3fff, TestHumanReadableSRv6SIDs, AddrspaceIPv6LargerThanBlock, AllocateAddressSpacePrefix48, AllocateAddressSpacePrefix64
**Links:** REQ-INFO-CALC-006, REQ-INFO-COMPAT-001

### REQ-INFO-COMPAT-001
**Requirement:** ipcalc MUST NOT change the `ADDRSPACE` string it prints
for a block when the IANA registry renames that block; a block newly
added to a registry MUST be reported with its registry name, so that
scripts matching existing `ADDRSPACE` values keep working.
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** maintainer decision (2026-10-03) when refreshing REQ-INFO-CALC-006/007 — the registry now names `Private-Use`, `Deprecated (6to4 Relay Anycast)`, `Unique-Local`, and `Link-Local Unicast`, which ipcalc keeps as `Private Use`, `6 to 4 Relay Anycast (Deprecated)`, `Unique Local Unicast`, and `Link-Scoped Unicast`
**Acceptance:** `ipcalc --addrspace 10.0.0.1` → `ADDRSPACE="Private Use"`; `ipcalc -i fc00:0:a001::bcdf:4` → `Address space: Unique Local Unicast` — positive ; test: TestHumanReadablePrivateUse1, TestHumanReadableUniqueLocalUnicast, TestHumanReadableLinkScopedUnicast, TestHumanReadableDeprecated6to4RelayAnycast

### REQ-INFO-CALC-008
**Requirement:** For IPv4, ipcalc MUST report the address class
(`ADDRCLASS`) of the network address as `Class A` (first octet 0–127),
`Class B` (128–191), `Class C` (192–223), `Class D` (224–239), or
`Class E` (240–255), and MUST NOT report a class for IPv6.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:ipv4_net_to_class
**Acceptance:** `ipcalc --all-info 192.168.2.7/24` → `Address class: Class C`; `ipcalc --all-info 224.0.0.1` and `ipcalc --all-info 239.1.1.1` → `Address class: Class D`; `ipcalc --all-info 240.0.0.1` → `Address class: Class E` — positive ; test: AllInfo, AllInfoClassDLowest, AllInfoClassDHighestOctet, AllInfoClassE

### REQ-INFO-CALC-009
**Requirement:** For IPv4, ipcalc MUST compute one reverse DNS domain
(`REVERSEDNS`) for the network: `d.c.b.a.in-addr.arpa.` for /32;
`c.b.a.in-addr.arpa.`, `b.a.in-addr.arpa.`, `a.in-addr.arpa.` for /24,
/16, /8; `in-addr.arpa.` for /0; and for any other prefix,
`<first>-<last>.` followed by the labels of the enclosing octet
boundary, where `<first>`/`<last>` are the values of the first partial
octet in the network's first and last address.
**Strength:** MUST
**Status:** REVIEW
**Source:** ipcalc-reverse.c:calc_reverse_dns4 (`USE_RFC2317_STYLE` undefined; "draft-ietf-dnsop-rfc2317bis-00")
**Acceptance:** `ipcalc --reverse-dns 193.92.150.3/32` → `REVERSEDNS=3.150.92.193.in-addr.arpa.`; `/26` → `0-63.150.92.193.in-addr.arpa.`; `/16` → `92.193.in-addr.arpa.`; `/12` → `80-95.193.in-addr.arpa.`; `/4` → `192-207.in-addr.arpa.`; `/0` → `in-addr.arpa.`; `ipcalc --reverse-dns 192.168.1.4/31` → `4-5.1.168.192.in-addr.arpa.` — positive ; test: LookupReverseDNSFromPrefix32IPv4, LookupReverseDNSFromPrefix26IPv4, LookupReverseDNSFromPrefix16IPv4, LookupReverseDNSFromPrefix12IPv4, LookupReverseDNSFromPrefix4IPv4, LookupReverseDNSFromPrefix0IPv4 (no /31 test)
**Links:** REQ-INFO-CALC-002

> [REVIEW: defect] For /31 the last value is taken from the broadcast
> address, which is `255.255.255.255` for /31 (REQ-INFO-CALC-002), so
> `192.168.1.4/31` gives `4-255.1.168.192.in-addr.arpa.`.

### REQ-INFO-CALC-010
**Requirement:** For IPv6, ipcalc MUST compute the reverse DNS domains
(`REVERSEDNS`) in the RFC 3596 nibble format: one domain when the prefix
is a multiple of 4 (`ip6.arpa.` for /0), and otherwise the
2^(4 − prefix mod 4) domains at the next nibble boundary that cover the
network, in ascending order.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-reverse.c:calc_reverse_dns6; ipcalc.h:MAX_REVERSE_DNS; ipcalc.1.md#Options (`--reverse-dns`)
**Acceptance:** `ipcalc --reverse-dns 1234::4321/124` → one domain; `ipcalc --reverse-dns 2001:db8::/31` → 2 domains; `…/33` → 8; `…/34` → 4; `ipcalc --reverse-dns ::/0` → `REVERSEDNS=ip6.arpa.`; `ipcalc --reverse-dns 2001:db8:3fff::1/35` → domains of the network, not the address — positive ; test: LookupReverseDNSFromPrefix124IPv6, LookupReverseDNSFromPrefix31IPv6, LookupReverseDNSFromPrefix33IPv6, LookupReverseDNSFromPrefix34IPv6, LookupReverseDNSFromPrefix127IPv6, LookupReverseDNSFromPrefix0IPv6, LookupReverseDNSUnmaskedPrefix35IPv6, LookupReverseDNS4321IPv6

### REQ-INFO-CALC-011
**Requirement:** With `-h`/`--hostname`, ipcalc MUST look up the name of
the input address with `getnameinfo()` and print it as `HOSTNAME`; a
network is rejected without a lookup (REQ-INFO-ERR-001).
**Strength:** MUST
**Status:** REVIEW
**Source:** ipcalc.c:get_hostname, ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info; ipcalc.1.md#Options (`--hostname`)
**Acceptance:** `ipcalc -h 127.0.0.1` → `HOSTNAME=localhost`; `ipcalc -h ::1` → tests/ip-localhost-ipv6 — positive ; test: IPIPv4Localhost, IPIPv6Localhost, IPIPv4LocalhostJson
**Links:** REQ-INFO-ERR-001

> [REVIEW] `getnameinfo()` is called without `NI_NAMEREQD`, so an
> address with no name succeeds and prints the numeric address
> (`ipcalc -h 192.0.2.1` → `HOSTNAME=192.0.2.1`, exit 0). The error path
> `ipcalc: cannot find hostname for <addr>` (exit 1) is only reached when
> the lookup itself fails. Decide whether "no name" is an error.

### REQ-INFO-ERR-001
**Requirement:** ipcalc MUST reject `-h`/`--hostname` when the input is
a network rather than a single address — an IPv4 prefix shorter than 32
or an IPv6 prefix shorter than 128, whether given, implied by
`--class-prefix`, or generated by `-r` — by printing
`ipcalc: -h requires a single address, not a network: <address>/<prefix>`
to stderr (subject to `-s`, REQ-CLI-ERR-006) and exiting with status 1
without other output, in every output format, so that a name is never
reported for a whole network. The name MUST NOT be looked up first, so
that a network is rejected without a DNS query and with this message
even when the lookup would fail.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`ATTR_REQUIRES_SINGLE_ADDRESS` check); ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info (prefix check before `get_hostname()`); ipcalc.1.md#Options (`--hostname`)
**Acceptance:** `ipcalc -h 127.0.0.1/32` → `HOSTNAME=localhost` — positive; `ipcalc -h 127.0.0.1/8` → `ipcalc: -h requires a single address, not a network: 127.0.0.1/8`, exit 1; `ipcalc -j -h 127.0.0.1/8`, `ipcalc -h ::1/64` → exit 1, no stdout; `ipcalc -s -h 127.0.0.1/8` → no output, exit 1; `ipcalc --format=json -h 127.0.0.1/8` → the message, exit 1; `ipcalc -h 127.0.0.1/8` with every `getnameinfo()` call failing → the message above, exit 1 — negative ; test: HostnameHostPrefix, HostnameNetwork, HostnameNetworkJson, HostnameNetworkIPv6, HostnameNetworkSilent, HostnameNetworkNoLookup, HostnameNetworkFormatJson
**Links:** REQ-INFO-CALC-011, REQ-INFO-OUTPUT-005, REQ-CLI-ERR-006

### REQ-INFO-CALC-012
**Requirement:** With `-o`/`--lookup-host <name>`, ipcalc MUST resolve
the name with `getaddrinfo()` — restricted to IPv4 with `-4`, to IPv6
with `-6`, otherwise any family — and use the first returned address as
the input address; if resolution fails it MUST print
`ipcalc: could not resolve <name>` and exit with status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ip_address, ipcalc.c:main (`if (hostname)`); ipcalc.1.md#Options (`--lookup-host`)
**Acceptance:** `ipcalc -4 -o localhost` → `ADDRESS=127.0.0.1`; `ipcalc -6 -o localhost` → tests/hostname-localhost-ipv6; `ipcalc -o no-such-host.invalid` → exit 1 — positive, negative ; test: HostnameIPv4Localhost, HostnameIPv6Localhost, HostnameIPv4LocalhostJson (no negative test)

### REQ-INFO-CALC-013
**Requirement:** For an IPv6 input address whose first three bits are
not `000`, whose first octet is not `ff`, and whose low 64 bits are not
all zero, ipcalc MUST take those low 64 bits as the interface
identifier. When octets 3 and 4 of the interface identifier are `ff`
and `fe`, ipcalc MUST treat it as a Modified EUI-64 (RFC 4291
Appendix A) and derive the EUI-64 by inverting bit `0x02` of its first
octet, and the MAC address from octets 0–2 and 5–7 of that EUI-64. It
MUST then report the MAC as universally administered when bit `0x02` of
its first octet is clear and locally administered otherwise, and as
individual when bit `0x01` of its first octet is clear and group
otherwise. Interface identifiers without the `ff:fe` octets MUST NOT
have an EUI-64, MAC address, or flags derived (RFC 7136). IPv4 inputs
have no interface identifier.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv6_iid_info, ipcalc.c:get_ipv6_info; GitLab issue #29
**Acceptance:** `ipcalc --all-info fe80::21b:21ff:fe3a:5c7d/64` → tests/all-info-fe80::21b:21ff:fe3a:5c7d-64 (MAC `00:1b:21:3a:5c:7d`, universal, individual); `ipcalc --all-info fe80::ff:fe00:1` → MAC `02:00:00:00:00:01`, local; `ipcalc --all-info 2001:db8::300:ff:fe00:1` → MAC `01:00:00:00:00:01`, group; `ipcalc --all-info 2001:db8::f00d:1234:5678:9abc/64` → interface identifier only; `ipcalc --all-info fe80::/64`, `ipcalc --all-info ::ffff:192.0.2.1`, `ipcalc --all-info ff02::21b:21ff:fe3a:5c7d`, `ipcalc --all-info 192.168.2.7/24` → no interface identifier — positive, negative ; test: AllInfoIIDLinkLocalUniversal, AllInfoIIDLinkLocalLocal, AllInfoIIDGroup, AllInfoIIDNotEUI64, AllInfoIIDNetwork, AllInfoIIDMapped, AllInfoIIDMulticast, AllInfo, JsonAllInfoIIDLinkLocalUniversal, NoDecorateAllInfoIIDLinkLocalUniversal
**Links:** REQ-INFO-OUTPUT-006

## Network comparison

These requirements define the comparisons of REQ-CLI-INPUT-006. Both
networks are compared by network address and prefix; host bits of the
input address and of NET are ignored.

### REQ-INFO-CALC-014
**Requirement:** `--equals=NET` MUST be true exactly when the input
network and NET are of the same family, have the same prefix, and have
the same network address, for both IPv4 and IPv6.
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:comparison_holds, netcompare.c:network_contains, netcompare.c:same_leading_bits, netcompare.c:to_binary_network; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 192.168.1.15 --equals=192.168.1.15/32`, `ipcalc 192.168.7.15/24 --equals=192.168.7.5/24`, `ipcalc 2001:db8::1/64 --equals=2001:db8::ffff/64`, `ipcalc 1.2.3.4/0 --equals=0.0.0.0/0` → exit 0; `ipcalc 192.168.0.0/24 --equals=192.168.0.0/23`, `ipcalc 192.168.1.15/24 --equals=192.168.7.1/24`, `ipcalc 2001:db8::/64 --equals=2001:db8::/65` → exit 1 — positive, negative ; test: EqualsHost32, EqualsHostBits, EqualsHostBitsIPv6, EqualsPrefix0, EqualsDifferentPrefix, EqualsDisjoint, EqualsDifferentPrefixIPv6
**Links:** REQ-CLI-INPUT-006, REQ-CLI-OUTPUT-009

### REQ-INFO-CALC-015
**Requirement:** `--subnet-of=NET` MUST be true exactly when the input
network and NET are of the same family and every address of the input
network is an address of NET — that is, the prefix of NET is less than
or equal to the input prefix and the input network address with its
last width−(NET prefix) bits cleared equals the network address of NET
— for both IPv4 and IPv6. Equal networks MUST compare true; a /0 NET
MUST contain every network of its family.
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:comparison_holds, netcompare.c:network_contains, netcompare.c:same_leading_bits, netcompare.c:to_binary_network; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 192.168.0.1/24 --subnet-of=192.168.1.15/23`, `ipcalc 10.0.0.0/8 --subnet-of=10.0.0.0/8`, `ipcalc 10.1.2.3 --subnet-of=0.0.0.0/0`, `ipcalc 10.64.0.0/10 --subnet-of=10.0.0.0/9`, `ipcalc 2001:db8:1::/48 --subnet-of=2001:db8::/32`, `ipcalc ::1 --subnet-of=::/0` → exit 0; `ipcalc 192.168.1.15/23 --subnet-of=192.168.0.1/24`, `ipcalc 10.128.0.0/10 --subnet-of=10.0.0.0/9`, `ipcalc 0.0.0.0/0 --subnet-of=0.0.0.0/1`, `ipcalc 2001:db8::/32 --subnet-of=2001:db8:1::/48` → exit 1 — positive, negative ; test: SubnetOf, SubnetOfEqual, SubnetOfPrefix0, SubnetOfPartialByte, SubnetOfIPv6, SubnetOfPrefix0IPv6, SubnetOfSupernet, SubnetOfSiblingPartialByte, SubnetOfPrefix0Supernet, SubnetOfSupernetIPv6
**Links:** REQ-INFO-CALC-016, REQ-CLI-OUTPUT-009

### REQ-INFO-CALC-016
**Requirement:** `--overlaps=NET` MUST be true exactly when the input
network and NET are of the same family and share at least one address,
for both IPv4 and IPv6. For two networks given by address and prefix
this holds exactly when one is a subnet of the other
(REQ-INFO-CALC-015), so the result MUST equal
`--subnet-of=NET` or, with the operands swapped, `--subnet-of`.
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:comparison_holds, netcompare.c:network_contains, netcompare.c:same_leading_bits, netcompare.c:to_binary_network; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 192.168.1.15/23 --overlaps=192.168.0.1/24`, `ipcalc 192.168.0.1/24 --overlaps=192.168.1.15/23`, `ipcalc 10.0.0.1 --overlaps=10.0.0.1`, `ipcalc ::/0 --overlaps=2001:db8::1` → exit 0; `ipcalc 192.168.1.15/24 --overlaps=192.168.7.1/24`, `ipcalc 192.168.0.0/24 --overlaps=192.168.1.0/24`, `ipcalc 2001:db8::/33 --overlaps=2001:db8:8000::/33` → exit 1 — positive, negative ; test: OverlapsSupernet, OverlapsSubnet, OverlapsHost, OverlapsPrefix0IPv6, OverlapsDisjoint, OverlapsAdjacent, OverlapsAdjacentIPv6
**Links:** REQ-INFO-CALC-015, REQ-CLI-OUTPUT-009

### REQ-INFO-CALC-017
**Requirement:** When the input network and NET are of different
families, `--equals`, `--subnet-of`, and `--overlaps` MUST be false
(exit status 1, REQ-CLI-OUTPUT-009) rather than an error, including
for an IPv4-mapped IPv6 address compared with the IPv4 address it maps,
so that lists that mix IPv4 and IPv6 networks can be processed without
errors.
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:comparison_holds, netcompare.c:network_contains, netcompare.c:same_leading_bits, netcompare.c:to_binary_network; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 10.0.0.0/8 --overlaps=::/0`, `ipcalc ::ffff:10.0.0.1 --equals=10.0.0.1` → `The networks do not overlap`, `The networks are not equal`, exit 1 — negative ; test: OverlapsMixedFamily, EqualsMappedIPv4
**Links:** REQ-CLI-INPUT-006, REQ-CLI-ERR-007

## Summary output

### REQ-INFO-OUTPUT-001
**Requirement:** The decorated and `--no-decorate` summary output MUST
print these lines, in this order, each only when its condition holds:
`Full Address` (IPv6) and `Address` — when the network is a single host
or the input address differs from the network address, and not for
`-r` unless single host; `Hostname` — `-h` (REQ-INFO-ERR-001); `Full Network` (IPv6), `Network: <net>/<prefix>`,
`Netmask: <mask> = <prefix>`, `Wildcard` (IPv4, `--all-info` or
`--wildcard`, REQ-INFO-OUTPUT-008), `Broadcast` (IPv4) — not for a single host; `Reverse DNS` — `--all-info` or `--reverse-dns`, one line per
domain; a blank
line (not for a single host); `Address space`; `Address class` —
`--all-info` and IPv4 only; `Interface ID`, `EUI-64`, `MAC address`,
`MAC scope`, `MAC type` — `--all-info` only (REQ-INFO-OUTPUT-006); `HostMin`, `HostMax`, `Hosts/Net` — not for
a single host; then, when geo information was found, a blank line and
`Country code`, `Country`, `City`, `Coordinates`.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`fields[]`, `field_selected()`, `field_in_summary()`)
**Acceptance:** `ipcalc 192.168.2.0/24` → tests/standard-192.168.2.0-24 (no `Address`); `ipcalc -i --reverse-dns 192.168.2.7/24` → tests/info-reverse-dns-192.168.2.7-24; `ipcalc -i 192.168.2.7/24` → tests/info-192.168.2.7; `ipcalc --all-info 192.168.2.7/24` → tests/all-info-192.168.2.7; `ipcalc -i 127.0.0.1` → tests/i-127.0.0.1 (single host) — positive ; test: StandardInfo, Info, AllInfo, NoDecorateAllInfo, TestHumanReadableLoopback, AllInfoReverseDNSPrefix31IPv6, InfoReverseDNS
**Links:** REQ-CLI-OUTPUT-002, REQ-INFO-OUTPUT-002, REQ-INFO-OUTPUT-006, REQ-GEO-OUTPUT-001

### REQ-INFO-OUTPUT-002
**Requirement:** In the decorated summary, the IPv6 `Hosts/Net` line
MUST be printed as `2^(<128 − prefix>) = <count>` when the prefix is
below 112, and as `<count>` otherwise; IPv4 MUST always print `<count>`.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`info->prefix < 112`)
**Acceptance:** `ipcalc -i 2001:db8::/64` → `Hosts/Net: 2^(64) = 18446744073709551616`; `ipcalc -i fd0b:a336:4e7d::/48` → tests/i-fd0b:a336:4e7d::-48 — positive ; test: TestHumanReadableGenericInfoPrefix48
**Links:** REQ-INFO-CALC-004

### REQ-INFO-OUTPUT-003
**Requirement:** The JSON summary MUST contain these keys, in this
order, each only when its condition holds: `FULLADDRESS` (IPv6) and
`ADDRESS` (same condition as REQ-INFO-OUTPUT-001), `HOSTNAME` (`-h`,
REQ-INFO-ERR-001), `FULLNETWORK` (IPv6), `NETWORK`, `NETMASK`,
`WILDCARD` (IPv4, `--all-info` or `--wildcard`, REQ-INFO-OUTPUT-008), `PREFIX`, `CIDR` (REQ-INFO-OUTPUT-007), `BROADCAST` (IPv4), `REVERSEDNS` (`--all-info` or `--reverse-dns`), `ADDRSPACE`,
`ADDRCLASS` (`--all-info`, IPv4), `INTERFACEID`, `EUI64`, `MACADDR`,
`MACSCOPE`, `MACTYPE` (`--all-info`, REQ-INFO-OUTPUT-006), `MINADDR`, `MAXADDR`, `ADDRESSES`,
then `COUNTRYCODE`, `COUNTRY`, `CITY`, `COORDINATES` when found.
`NETWORK` and `FULLNETWORK` MUST NOT include the prefix, and the
network keys MUST be present for single hosts too.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`fields[]`, `field_selected()`, `field_in_summary()`)
**Acceptance:** `ipcalc -j -i 2a03:2880:20:4f06:face:b00c:0:1/56` → tests/json-i-2a03:2880:20:4f06:face:b00c:0:1-56; `ipcalc -j --reverse-dns 192.168.2.7/24` → tests/json-reverse-dns-192.168.2.7-24 (`REVERSEDNS` array); `ipcalc -j 192.168.2.7/24` → no `REVERSEDNS`; `ipcalc -j -h 127.0.0.1` → tests/ip-localhost-ipv4-json — positive ; test: JsonTestHumanReadableGenericInfoPrefix56, JsonAllInfoReverseDNSPrefix24IPv4, IPIPv4LocalhostJson, HostnameIPv4LocalhostJson, JsonReverseDNS
**Links:** REQ-CLI-OUTPUT-008, REQ-CLI-OUTPUT-012, REQ-INFO-OUTPUT-004, REQ-INFO-OUTPUT-006, REQ-INFO-OUTPUT-008

### REQ-INFO-OUTPUT-004
**Requirement:** In JSON output, `REVERSEDNS` MUST always be an array of
strings, even when there is a single domain, so that its type does not
depend on the prefix.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_json_field ("always an array"); ipcalc.1.md#Options (`--reverse-dns`); NEWS
**Acceptance:** `ipcalc --all-info --json 2001:db8::/32` → `"REVERSEDNS":["8.b.d.0.1.0.0.2.ip6.arpa."]`; `…/31` → two-element array — positive ; test: JsonAllInfoReverseDNSPrefix32IPv6, JsonAllInfoReverseDNSPrefix31IPv6, JsonAllInfoReverseDNSPrefix24IPv4

### REQ-INFO-OUTPUT-005
**Requirement:** When `-h` is combined with `-i` or `-j`, ipcalc MUST
include the resolved name as `Hostname` in the summary and as
`HOSTNAME` in the JSON object; since `-h` accepts only a single address
(REQ-INFO-ERR-001), the name is never dropped for a network.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`HOSTNAME_NAME` row)
**Acceptance:** `ipcalc -j -h 127.0.0.1` → tests/ip-localhost-ipv4-json; `ipcalc -j -h 127.0.0.1/8` → exit 1 (REQ-INFO-ERR-001) — positive, negative ; test: IPIPv4LocalhostJson, HostnameNetworkJson
**Links:** REQ-INFO-ERR-001

### REQ-INFO-OUTPUT-006
**Requirement:** With `--all-info`, when the input address has an
interface identifier (REQ-INFO-CALC-013), the summary MUST print
`Interface ID` (`INTERFACEID`) as four colon-separated groups of four
lowercase hex digits; when it is a Modified EUI-64 it MUST also print
`EUI-64` (`EUI64`) and `MAC address` (`MACADDR`) as colon-separated
lowercase hex octets, `MAC scope` (`MACSCOPE`) as `universal` or
`local`, and `MAC type` (`MACTYPE`) as `individual` or `group`. The
fields MUST be printed for single hosts as well as networks, MUST be
JSON strings, and MUST NOT be printed without `--all-info` or in
`NAME=value` output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`INTERFACEID_NAME` … `MACTYPE_NAME` rows, `ATTR_SUMMARY_WITH_ALL_INFO`); ipcalc.1.md#Options (`--all-info`); GitLab issue #29
**Acceptance:** `ipcalc --all-info fe80::21b:21ff:fe3a:5c7d/64` → tests/all-info-fe80::21b:21ff:fe3a:5c7d-64; `ipcalc --all-info --json fe80::21b:21ff:fe3a:5c7d/64` → tests/json-all-info-fe80::21b:21ff:fe3a:5c7d-64; `ipcalc --all-info --no-decorate fe80::21b:21ff:fe3a:5c7d/64` → tests/no-decorate-all-info-fe80::21b:21ff:fe3a:5c7d-64; `ipcalc -i fe80::21b:21ff:fe3a:5c7d/64` → no interface identifier lines — positive, negative ; test: AllInfoIIDLinkLocalUniversal, JsonAllInfoIIDLinkLocalUniversal, NoDecorateAllInfoIIDLinkLocalUniversal, InfoIIDLinkLocal
**Links:** REQ-INFO-CALC-013, REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003

### REQ-INFO-OUTPUT-007
**Requirement:** `--cidr` MUST select the `CIDR` field, which is the
network address (REQ-INFO-CALC-001) and the prefix in CIDR notation,
`<network>/<prefix>`, for both IPv4 and IPv6, with the network address
written as `NETWORK` is (REQ-INFO-OUTPUT-003). The prefix MUST be
included for single hosts too (/32, /128, or no prefix given), so that
scripts get one form for every input. In `NAME=value` output it MUST be
printed as `CIDR=<network>/<prefix>`, with `--no-decorate` as
`<network>/<prefix>`, and in the JSON summary as the string key `CIDR`;
the decorated summary MUST NOT change, as its `Network` line already
shows the same value. It applies to generated networks (`-r`) as to
given ones.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`OPT_CIDR`, `FLAG_SHOW_CIDR`); ipcalc.c:show_info_fields (`CIDR_NAME` row); ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options (`--cidr`)
**Acceptance:** `ipcalc --cidr 192.168.123.7/24` → `CIDR=192.168.123.0/24`; `ipcalc --cidr --no-decorate 2001:db8::1/64` → `2001:db8::/64`; `ipcalc --cidr 192.168.1.1` → `CIDR=192.168.1.1/32`; `ipcalc --cidr ::1` → `CIDR=::1/128`; `ipcalc -n --cidr -p 10.1.2.3/8` → `NETWORK=10.0.0.0`, `PREFIX=8`, `CIDR=10.0.0.0/8`; `ipcalc -j 192.168.2.7/24` → `"CIDR":"192.168.2.0/24"`; `ipcalc -r 24 --cidr --no-decorate` → exit 0 — positive ; test: CidrIPv4, CidrNoDecorateIPv6, CidrSingleHost, CidrSingleHostIPv6, CidrFieldOrder, JsonAllInfoReverseDNSPrefix24IPv4, RandomCidr
**Links:** REQ-INFO-OUTPUT-003, REQ-CLI-OUTPUT-004, REQ-CLI-OUTPUT-006

### REQ-INFO-OUTPUT-008
**Requirement:** `--wildcard` MUST select the `WILDCARD` field, the
wildcard (inverse) mask used by Cisco ACLs and OSPF: the bitwise
complement of the IPv4 netmask, in dotted-quad notation (e.g.
`0.0.0.255` for /24, `0.0.0.0` for /32 and `255.255.255.255` for /0).
For IPv6, where wildcard masks are not used, ipcalc MUST NOT print the
field in any output format, and `--wildcard` MUST NOT make it fail, as
with `BROADCAST` (REQ-INFO-CALC-002). In `NAME=value`
output it MUST be printed as `WILDCARD=<mask>`, with `--no-decorate` as
`<mask>`. The decorated and `--no-decorate` summary MUST print it as a
`Wildcard:` line, and the JSON summary as the string key `WILDCARD`,
only with `--all-info` or `--wildcard` and not with `-i` alone or by
default, so that the default and `-i` output of every format is
unchanged. It applies to generated networks (`-r`) as to given ones.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`OPT_WILDCARD`, `FLAG_SHOW_WILDCARD`); ipcalc.c:show_info_fields (`WILDCARD_NAME` row); ipcalc.c:get_ipv4_info (`info->wildcard`); ipcalc.h (`wildcard; /* ipv4 only */`); ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options (`--wildcard`)
**Acceptance:** `ipcalc --wildcard 192.168.123.7/24` → `WILDCARD=0.0.0.255`; `ipcalc --wildcard 192.168.1.1` → `WILDCARD=0.0.0.0`; `ipcalc --wildcard 0.0.0.0/0` → `WILDCARD=255.255.255.255`; `ipcalc --wildcard 10.0.0.1/1` → `WILDCARD=127.255.255.255`; `ipcalc --wildcard 10.0.0.1/8` → `WILDCARD=0.255.255.255`; `ipcalc --wildcard 172.16.5.4/12` → `WILDCARD=0.15.255.255`; `ipcalc --wildcard 172.16.5.4/16` → `WILDCARD=0.0.255.255`; `ipcalc --wildcard 192.168.1.130/25` → `WILDCARD=0.0.0.127`; `ipcalc --wildcard 192.168.1.130/27` → `WILDCARD=0.0.0.31`; `ipcalc --wildcard 192.168.1.130/31` → `WILDCARD=0.0.0.1`; `ipcalc --wildcard 10.1.2.3/255.255.255.252` → `WILDCARD=0.0.0.3`; `ipcalc --wildcard --no-decorate 172.16.5.4/12` → `0.15.255.255`; `ipcalc --wildcard -m -n 10.1.2.3/255.255.240.0` → `NETWORK=10.1.0.0`, `NETMASK=255.255.240.0`, `WILDCARD=0.0.15.255`; `ipcalc -i --wildcard 192.168.2.7/24` → tests/info-wildcard-192.168.2.7-24; `ipcalc -j --wildcard 192.168.2.7/24` → tests/json-wildcard-192.168.2.7-24; `ipcalc --all-info 192.168.2.7/24` → tests/all-info-192.168.2.7; `ipcalc --all-info --json 192.168.2.7/24` → tests/json-all-info-192.168.2.7-24; `ipcalc -r 24 --wildcard --no-decorate` → exit 0 — positive; `ipcalc -j 192.168.2.7/24` → no `WILDCARD` key; `ipcalc -i 192.168.2.7/24` → no `Wildcard` line; `ipcalc --wildcard 2001:db8::1/64`, `ipcalc --wildcard ::/0` → no output, exit 0; `ipcalc -i --wildcard 2001:db8::/32` → no `Wildcard` line; `ipcalc --all-info --json 2001:db8::/32` → no `WILDCARD` key — negative ; test: WildcardIPv4, WildcardSingleHost, WildcardPrefix0, WildcardPrefix1, WildcardPrefix8, WildcardPrefix12, WildcardPrefix16, WildcardPrefix25, WildcardPrefix27, WildcardPrefix31, WildcardNetmask, WildcardNoDecorate, WildcardFieldOrder, InfoWildcard, JsonWildcard, RandomWildcard, AllInfo, JsonAllInfoReverseDNSPrefix24IPv4, JsonTestHumanReadableGenericInfoPrefix56, Info, WildcardIPv6, WildcardPrefix0IPv6, InfoWildcardIPv6, JsonAllInfoReverseDNSPrefix32IPv6
**Links:** REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003, REQ-CLI-OUTPUT-004, REQ-CLI-OUTPUT-006

### REQ-INFO-OUTPUT-009
**Requirement:** The summary output of `-i`, or of info mode with no
mode option (REQ-CLI-INPUT-001), MUST show a small, basic set of
information about the input: without field-selecting options it MUST
print at most 15 lines, each at most 80 columns with tabs expanded to
8-column stops, for any IPv4 or IPv6 input, decorated or with
`--no-decorate`. Each field-selecting option given with `-i` MUST add
its field to the summary (e.g. `--reverse-dns`, `--wildcard`, `-h`), and
those fields do not count toward the limit. An information item that
would take the summary past the limit MUST NOT be added to it; it
belongs to `--all-info` (REQ-INFO-OUTPUT-010) or to its own option, so
that the default output stays readable at a glance.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`ATTR_SUMMARY_ALWAYS` rows, `field_in_summary()`); ipcalc.1.md#Options (`--info`)
**Acceptance:** `ipcalc -i ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1` → 10 lines, at most 65 columns; `ipcalc -i 255.255.255.255/1` → 9 lines; `ipcalc -i --reverse-dns 192.168.2.7/24` → tests/info-reverse-dns-192.168.2.7-24; `ipcalc -i --wildcard 192.168.2.7/24` → tests/info-wildcard-192.168.2.7-24 — positive ; `ipcalc -i 192.168.2.7/24` → no `Wildcard`, `Reverse DNS`, or `Address class` line; `ipcalc -i fe80::21b:21ff:fe3a:5c7d/64` → no interface identifier lines — negative ; test: InfoSummarySizeIPv6, InfoSummarySizeIPv4, InfoReverseDNS, InfoWildcard, Info, InfoIIDLinkLocal
**Links:** REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-010, REQ-CLI-OUTPUT-002, REQ-CLI-COMPAT-006

### REQ-INFO-OUTPUT-010
**Requirement:** With `--all-info` ipcalc MUST show every information
item it can determine for the input without a network query, in the
summary and JSON outputs: the fields of every field-selecting option
except `-h` and `-o`, for both IPv4 and IPv6 — including `-g` when a
geo backend and database are available (REQ-GEO-CALC-001) — and every
field shown only with `--all-info` (`Address class`, the interface
identifier fields of REQ-INFO-OUTPUT-006). The output length is not
limited. An information item that ipcalc learns to compute MUST be
added to `--all-info`. `--all-info` MUST NOT resolve a hostname or
perform any other DNS query, which only `-h` and `-o` request, so that
it shows everything available without blocking on the network.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`ATTR_SUMMARY_WITH_ALL_INFO` rows, `field_in_summary()`); ipcalc.c:main (`geo_setup() == 0 && FLAG_SHOW_ALL_INFO`); ipcalc.c:field_selecting_options; ipcalc.1.md#Options (`--all-info`)
**Acceptance:** `ipcalc --all-info -j 192.168.2.7/24` and `ipcalc --all-info -j fe80::21b:21ff:fe3a:5c7d/64` → contain every key that `-j` with `-a -b -m -n -p --cidr --wildcard --reverse-dns --addrspace --minaddr --maxaddr --addresses` prints for the same input; `ipcalc --all-info 192.168.2.7/24` → tests/all-info-192.168.2.7 — positive ; `ipcalc --all-info -j 127.0.0.1` with a reverse lookup that would return a name → exit 0 and no `HOSTNAME` key — negative ; test: AllInfoJsonKeysIPv4, AllInfoJsonKeysIPv6, AllInfo, AllInfoIIDLinkLocalUniversal, AllInfoNoLookup (Linux only, lookups faked with an `LD_PRELOAD` shim, tests/fake-getnameinfo.c)
**Links:** REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003, REQ-INFO-OUTPUT-006, REQ-INFO-OUTPUT-009, REQ-GEO-CALC-001, REQ-CLI-COMPAT-006

## Open items

`REVIEW`: REQ-INFO-INPUT-004 (prefix plus netmask message),
REQ-INFO-INPUT-005 (man page on default IPv4 prefix),
REQ-INFO-CALC-009 (/31 reverse DNS),
REQ-INFO-CALC-011 (`-h` never fails for unnamed addresses).
`AMBIGUOUS`: hex/octal prefixes (REQ-INFO-INPUT-003).
`UNDOCUMENTED`: REQ-INFO-INPUT-006 (`172/8` padding).
Missing tests: IPv4 /31, /32 host counts; IPv6 broadcast
omission; /31 reverse DNS; `-o` resolution failure.
