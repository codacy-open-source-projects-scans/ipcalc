---
title: Random private network generation (-r/--random-private)
id-prefix: REQ-RANDOM
sources:
  - ipcalc.c (generate_ip_network, randomize, str_to_prefix, main)
  - ipcalc.1.md
  - tests/meson.build
---

# Random private network generation (`-r/--random-private`)

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
The generated network is then processed like an input network in info
mode (`info.md`); its address is random, so requirements fix the format
and the address space, not the value.

### REQ-RANDOM-INPUT-001
**Requirement:** `-r <prefix>` MUST accept a prefix as an integer or an
IPv4 dotted-decimal netmask; it MUST generate an IPv6 network when `-6`
is given or the prefix is greater than 32, and an IPv4 network
otherwise; and it MUST reject a prefix that is invalid for the family
with `ipcalc: bad IPv4 prefix: <arg>` / `ipcalc: bad IPv6 prefix: <arg>`
and exit status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`FLAG_RANDOM`, `str_to_prefix(&flags, randomStr, 1)`)
**Acceptance:** `ipcalc -r 112` → IPv6; `ipcalc -6 -r 24` → IPv6 /24; `ipcalc -r 255.255.0.0 -n` → an IPv4 /16; `ipcalc -r 129` → exit 1 — positive, negative ; test: RandomIPv6Implicit, RandomIPv6Explicit (no invalid-prefix test)

### REQ-RANDOM-ERR-001
**Requirement:** ipcalc MUST reject an address argument given together
with `-r` with `ipcalc: provided superfluous parameter '<arg>'` and exit
status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`if (flags & FLAG_RANDOM) { if (ipStr) …`)
**Acceptance:** `ipcalc -r 24 -i 127.0.0.1` → exit 1 — negative ; test: Random-Info

### REQ-RANDOM-CALC-001
**Requirement:** The generated network MUST lie entirely within private
address space — for IPv4 within 10.0.0.0/8, 172.16.0.0/12, or
192.168.0.0/16, choosing among those whose length does not exceed the
requested prefix; for IPv6 within the locally assigned ULA block
fd00::/8 — and MUST draw its random bits from `/dev/urandom`.
**Strength:** MUST
**Status:** REVIEW
**Source:** ipcalc.c:generate_ip_network, ipcalc.c:randomize; ipcalc.1.md#Options (`--random-private`)
**Acceptance:** `ipcalc -r 24 --addrspace` → `ADDRSPACE="Private Use"`; `ipcalc -6 -r 48 -a` → `ADDRESS=fd…`; `ipcalc -r 4 -n` → rejected or private — positive, negative ; test: RandomIPv6PrefixIsValidULA, Random, RandomAddress

> [REVIEW: defect] For prefixes shorter than the private block, the
> network is not private: `ipcalc -r 4 -n --addrspace` → `0.0.0.0`,
> `"This host on this network"`, and `ipcalc -r 6 -n` → `8.0.0.0`,
> which is public `Internet` space (any IPv4 prefix below 8);
> `ipcalc -6 -r 4 -n` → `f000::` (`Reserved`; any IPv6 prefix below 8,
> and /7 gives `fc00::/7`, which is not locally assigned). Either reject
> such prefixes or document the behavior.
> IPv4 prefixes 12–15 always use 10.0.0.0/8 (172.16.0.0/12 is chosen
> only for prefixes ≥ 16); the choice between blocks uses the low bits
> of the process CPU-time clock, which is incidental.

### REQ-RANDOM-OUTPUT-001
**Requirement:** With `-r` and no field-selecting option, ipcalc MUST
print the summary output of the generated network
(REQ-INFO-OUTPUT-001) without the `Address` line, except that for /32
and /128 it MUST print the `Address` line; with field-selecting options
(e.g. `-a`, `-n`) it MUST print `NAME=value` output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`input_is_network`, `ATTR_SUMMARY_HIDE_FOR_NETWORK_ADDRESS`); ipcalc.1.md#Options (`--random-private`)
**Acceptance:** `ipcalc -r 24` → contains `Network:`, no `Address:`; `ipcalc -r 32` → contains `Address:`; `ipcalc -r 24 -a` → `ADDRESS=…` — positive ; test: Random, RandomAddress, RandomIPv6Implicit, RandomIPv6Explicit

## Open items

`REVIEW`: REQ-RANDOM-CALC-001 (non-private networks for short
prefixes).
Missing tests: invalid `-r` prefix; netmask-form `-r`; short prefixes.
