---
title: Range deaggregation (-d/--deaggregate)
id-prefix: REQ-DEAGG
sources:
  - deaggregate.c
  - ipv6.c
  - ipcalc.c (main)
  - ipcalc.1.md
  - tests/meson.build
---

# Range deaggregation (`-d/--deaggregate`)

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.

### REQ-DEAGG-INPUT-001
**Requirement:** `-d <first>-<last>` MUST take an address range as two
addresses of the same family separated by `-`, ignoring whitespace
around each address, and MUST treat the range as IPv6 when it contains
`:` or `-6` is given (REQ-CLI-INPUT-005).
**Strength:** MUST
**Status:** DERIVED
**Source:** deaggregate.c:deaggregate (`strtok(str, "-")`, `trim`); ipcalc.c:main; ipcalc.1.md#Options (`--deaggregate`)
**Acceptance:** `ipcalc -d " 192.168.2.0 - 192.168.3.255 "` → same as `ipcalc -d 192.168.2.0-192.168.3.255` — positive ; test: DeaggregateSpaces, Deaggregate

> [UNDOCUMENTED] Text after a second `-` is ignored:
> `ipcalc -d 1.2.3.4-1.2.3.4-1.2.3.9` deaggregates `1.2.3.4-1.2.3.4`.

### REQ-DEAGG-ERR-001
**Requirement:** ipcalc MUST reject, with exit status 1: a range without
a `-` separator (`ipcalc: bad deaggregation string: <arg>`), an address
that is not valid for the range's family (`ipcalc: bad IPv4 address: …`
/ `ipcalc: bad IPv6 address: …`), and a range whose first address is
greater than its last (`ipcalc: bad range` / `ipcalc: bad IPv6 range`).
**Strength:** MUST
**Status:** DERIVED
**Source:** deaggregate.c:deaggregate, deaggregate.c:deaggregate_v4, deaggregate.c:deaggregate_v6
**Acceptance:** `ipcalc -d 1.2.3.4`, `ipcalc -d 1.2.3.4-::5`, `ipcalc -d 1.2.3.5-1.2.3.4` → exit 1 — negative ; test: none

### REQ-DEAGG-CALC-001
**Requirement:** ipcalc MUST print the minimal list of CIDR networks
that exactly covers the range, in ascending address order, for every
range within the address space — including ranges that start at the
first address or end at the last address of the space.
**Strength:** MUST
**Status:** DERIVED
**Source:** deaggregate.c:deaggregate_v4, deaggregate.c:deaggregate_v6, ipv6.c:ipv6_orm
**Acceptance:** `ipcalc --no-decorate -d 192.168.2.7-192.168.2.13` → `192.168.2.7/32`, `192.168.2.8/30`, `192.168.2.12/31`; `ipcalc -d 0.0.0.0-255.255.255.255` → `0.0.0.0/0`; `ipcalc -d ::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff` → `::/0`; `ipcalc -d 8000::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff` → `8000::/1`; `ipcalc -d ::-8000::` → `::/1`, `8000::/128`; `ipcalc -d ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff` → `ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/128` — positive ; test: DeaggregateNoDecorate, Deaggregate, DeaggregateLargeNum, DeaggregateLargeNum2, DeaggregateLargeNum3, DeaggregateIPv6, DeaggregateIPv6NoDecorate, DeaggregateIPv6All, DeaggregateIPv6UpperHalf, DeaggregateIPv6LowerHalf, DeaggregateIPv6LastAddress

### REQ-DEAGG-OUTPUT-001
**Requirement:** The deaggregation output MUST be: decorated — a
`[Deaggregated networks]` header and one `Network:\t<net>/<prefix>` line
per network; with `--no-decorate` — only one `<net>/<prefix>` line per
network; with `-j` — an object whose `DEAGGREGATEDNETWORK` key is an
array of `<net>/<prefix>` strings.
**Strength:** MUST
**Status:** DERIVED
**Source:** deaggregate.c (`array_start(…, "Deaggregated networks", "DEAGGREGATEDNETWORK")`, `default_printf`)
**Acceptance:** `ipcalc -d 192.168.2.0-192.168.3.255`, `ipcalc --no-decorate -d 192.168.2.7-192.168.2.13`, `ipcalc -j -d 192.168.2.33-192.168.3.2` → the matching tests/deaggregate-… files — positive ; test: Deaggregate, DeaggregateNoDecorate, DeaggregateJson, DeaggregateIPv6Json
**Links:** REQ-CLI-OUTPUT-003, REQ-CLI-OUTPUT-008

## Open items

`UNDOCUMENTED`: trailing text after a second `-` ignored
(REQ-DEAGG-INPUT-001).
Missing tests: all of REQ-DEAGG-ERR-001.
