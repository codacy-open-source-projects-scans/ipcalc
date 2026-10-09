---
title: Command line, modes, output formats, and exit status
id-prefix: REQ-CLI
sources:
  - ipcalc.c (main, usage, long_options, output and printf helpers)
  - netcompare.c (compare_networks)
  - ipcalc.h (enum app_t, FLAG_*, FLAGS_TO_IGNORE, ENV_INFO_FLAGS)
  - ipcalc.1.md
  - tests/meson.build
---

# Command line, modes, output formats, and exit status

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
Computed values (network, broadcast, address space, …) are in `info.md`;
this document covers how ipcalc is invoked and how results are printed.

## Modes and option combinations

### REQ-CLI-INPUT-001
**Requirement:** ipcalc MUST select exactly one mode per invocation from
the options given: version (`-v`), check (`-c`), comparison
(`--equals`, `--subnet-of`, `--overlaps`), split (`-S`, `--split-hosts`),
deaggregate (`-d`), or info (`-i`, `--all-info`, `-r`, `-a`, `-b`,
`-m`, `-n`, `-p`, `-h`, `-o`, `-g`, `--reverse-dns`, `--cidr`,
`--minaddr`, `--maxaddr`, `--addresses`, `--addrspace`), and MUST use the info mode
when none of these options is given, so that every invocation has one
well-defined output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (getopt loop setting `app`; `switch (app)`); ipcalc.h:enum app_t; ipcalc.1.md#Description
**Acceptance:** `ipcalc 192.168.2.7/24` → same output as `ipcalc -i 192.168.2.7/24` — positive ; test: InfoImplicit, Info
**Links:** REQ-CLI-ERR-001, REQ-CLI-INPUT-006

### REQ-CLI-ERR-001
**Requirement:** ipcalc MUST NOT run when options of more than one mode
(REQ-CLI-INPUT-001) are given; it MUST print
`ipcalc: you cannot mix these options` to stderr and exit with status 1
(status 2 when a comparison option is given, REQ-CLI-OUTPUT-009), so
that conflicting requests are not silently resolved.
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** ipcalc.c:main (`bit_count(app) > 1`)
**Acceptance:** `ipcalc -abmnp -S 26 10.100.1.0/24`, `ipcalc -c -abmnp 10.100.1.1`, `ipcalc -c -S 26 10.100.1.0/24`, `ipcalc -c -h 127.0.0.1`, `ipcalc -n --split-hosts=10 10.100.1.0/24`, `ipcalc -v -n 1.2.3.4` → exit 1; `ipcalc --equals=10.0.0.0/8 -n 10.0.0.0/8`, `ipcalc -c 10.0.0.0/8 --equals=10.0.0.0/8` → exit 2 — negative ; test: Split-Info, SplitHosts-Info, Check-Info, Check-Split, Check-Host, Equals-Info, Check-Equals (no test for `-v` with another mode)
**Links:** REQ-CLI-INPUT-001, REQ-CLI-OUTPUT-009

### REQ-CLI-INPUT-002
**Requirement:** In version mode ipcalc MUST print `ipcalc <version>`
to stdout, where `<version>` is the project version from `meson.build`,
and exit with status 0.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case APP_VERSION`); meson.build (`version`); Makefile (`VERSION=`)
**Acceptance:** `ipcalc -v` → `ipcalc 1.0.4`, exit 0 — positive ; test: none

### REQ-CLI-INPUT-003
**Requirement:** `-?`/`--help` MUST print the full option summary and
`--usage` MUST print the brief usage summary, both to stderr, and both
MUST exit with status 0 without processing other arguments.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case OPT_HELP`, `case '?'` with `optopt == '?'`, `case OPT_USAGE`); ipcalc.c:usage
**Acceptance:** `ipcalc --help`, `ipcalc -?` → exit 0 — positive ; test: TestHelpCommand, TestShortHelpCommand
**Links:** REQ-CLI-ERR-002, REQ-CLI-COMPAT-002

### REQ-CLI-ERR-002
**Requirement:** ipcalc MUST exit with status 1 (2 with a comparison
option, REQ-CLI-OUTPUT-009) without processing the address when given
an unrecognized option, or an option without its required argument,
after printing getopt's diagnostic and the full option summary to
stderr, so that scripts detect the mistake.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case '?'` sets `badOption`; `exit(err)` after the getopt loop)
**Acceptance:** `ipcalc --bogus 1.2.3.4`, `ipcalc -y 1.2.3.4`, `ipcalc -S` → exit 1 — negative ; test: UnknownLongOption, UnknownShortOption, MissingOptionArgument, UnknownOptionStatus
**Links:** REQ-CLI-INPUT-003

### REQ-CLI-INPUT-004
**Requirement:** Outside deaggregate mode, ipcalc MUST take the input
address as the first non-option argument and an optional IPv4 netmask
or prefix as the second; non-option arguments after the second MUST be
ignored.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`optind` handling, `chptr`)
**Acceptance:** `ipcalc -b 192.168.1.1 255.255.255.0` → `BROADCAST=192.168.1.255` — positive ; test: CalculateBroadcastAddressFromNetmaskIPv4
**Links:** REQ-INFO-INPUT-003, REQ-CLI-ERR-004

> [UNDOCUMENTED] Arguments after the second are silently ignored
> (`ipcalc -n 1.2.3.4 255.255.0.0 extra` → `NETWORK=1.2.0.0`, exit 0).

### REQ-CLI-ERR-003
**Requirement:** When no input address is available (no non-option
argument, no `-o`, no `-r`), ipcalc MUST print
`ipcalc: ip address expected` and the full option summary to stderr and
exit with status 1 (2 with a comparison option, REQ-CLI-OUTPUT-009).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`hostname == NULL && ipStr == NULL`)
**Acceptance:** `ipcalc -c` → exit 1 — negative ; test: none

### REQ-CLI-ERR-004
**Requirement:** ipcalc MUST reject a second non-option argument when
the address is IPv6 with `ipcalc: unexpected argument: <arg>`, the full
option summary, and exit status 1 (2 with a comparison option,
REQ-CLI-OUTPUT-009); and MUST reject a non-option argument in
deaggregate mode with `ipcalc: superfluous option given` and exit
status 1.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`chptr` with `FLAG_IPV6`; `ipStr != NULL` after `-d`)
**Acceptance:** `ipcalc -n ::1 64` → exit 1; `ipcalc -d 1.2.3.4-1.2.3.4 5.5.5.5` → exit 1 — negative ; test: none
**Links:** REQ-CLI-INPUT-004, REQ-DEAGG-INPUT-001

### REQ-CLI-INPUT-005
**Requirement:** ipcalc MUST treat the input as IPv6 when `-6` is given
or when, without `-4`, the input address (or the address resolved by
`-o`) contains `:`; otherwise it MUST treat it as IPv4.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`strchr(ipStr, ':')`, `FLAG_IPV6`/`FLAG_IPV4`)
**Acceptance:** `ipcalc -c ::1` → exit 0; `ipcalc -c -4 2a01:198:200:300::2` → exit 1; `ipcalc -6 -n 1.2.3.4` → `ipcalc: bad IPv6 address: 1.2.3.4`, exit 1 — positive, negative ; test: ValidateLoopbackIPv6, NoValidateIPv6InIPv4Mode
**Links:** REQ-CLI-ERR-005, REQ-SPLIT-INPUT-002

### REQ-CLI-ERR-005
**Requirement:** ipcalc MUST NOT accept `-4` together with `-6`; it MUST
print `ipcalc: you cannot specify both IPv4 and IPv6` and exit with
status 1 (2 with a comparison option, REQ-CLI-OUTPUT-009).
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** ipcalc.c:main
**Acceptance:** `ipcalc -4 -6 2a01:198:200:300::2`, `ipcalc -c -4 -6 127.0.0.1` → exit 1 — negative ; test: NoAllowIPv4AndIPv6, NoValidateIPv4AndIPv6WithIPv4Address

### REQ-CLI-ERR-006
**Requirement:** `-s`/`--silent` MUST suppress every error message ipcalc
itself prints to stderr, and the result line of comparison mode
(REQ-CLI-OUTPUT-010), without changing the exit status.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c, netsplit.c, deaggregate.c (`if (!beSilent)` guards); ipcalc.1.md#Options
**Acceptance:** `ipcalc -s -c bad` → no output, exit 1; `ipcalc -s 10.0.0.0/8 --overlaps=::/0` → no output, exit 1 — positive ; test: OverlapsSilent

> [REVIEW] A few messages are not guarded by `beSilent`: `inet_ntop
> failure at line N` (ipcalc.c, deaggregate.c — not reachable with valid
> input), the memory allocation failure messages, getopt's own
> unrecognized-option message, and the geo backend's runtime-linking
> error (guarded, but printed by the backend). None is reachable with
> well-formed input except the getopt message.

## Check mode

### REQ-CLI-OUTPUT-001
**Requirement:** In check mode (`-c`) ipcalc MUST parse the input exactly
as the info mode does — address, optional prefix or netmask, and the
address family rules of REQ-CLI-INPUT-005 — and MUST exit with status 0
without printing anything when it is valid, and with status 1 when it
is not.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case APP_CHECK_ADDRESS: return 0` after `get_ipv4_info`/`get_ipv6_info`); ipcalc.1.md#Options (`--check`)
**Acceptance:** `ipcalc -c 127.0.0.1`, `ipcalc -c -6 ::1/128`, `ipcalc -c -6 2a01:0198:0200:0300:0000:0000:0000:0002` → exit 0; `ipcalc -c -6 gggg::`, `ipcalc -c ::1/999`, `ipcalc -c 1.2.3` → exit 1 — positive, negative ; test: ValidateLoopbackIPv4, ValidateLoopbackIPv6WithPrefix, ValidateCompleteGlobalIPv6, NoValidateInvalidHexIPv6, NoValidateInvalidPrefixIPv6
**Links:** REQ-INFO-INPUT-001, REQ-INFO-INPUT-002

## Network comparison

### REQ-CLI-INPUT-006
**Requirement:** ipcalc MUST accept the comparison-mode options
`--equals=NET`, `--subnet-of=NET`, and `--overlaps=NET`, for both IPv4
and IPv6, which compare the input network with NET as REQ-INFO-CALC-014
to REQ-INFO-CALC-017 define. NET MUST be parsed as the input address is
parsed — address syntax (REQ-INFO-INPUT-001, REQ-INFO-INPUT-002), a
`/prefix` or, for IPv4, `/netmask` suffix (REQ-INFO-INPUT-003), the
default prefix and `--class-prefix` (REQ-INFO-INPUT-005), and the
family rules of REQ-CLI-INPUT-005 applied to NET on its own. The input
network MAY be given in any form check mode accepts, including an IPv4
netmask as the second argument, so that scripts can compare networks
without first normalizing them.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`OPT_EQUALS`, `OPT_SUBNET_OF`, `OPT_OVERLAPS`; `case APP_COMPARE`), netcompare.c:compare_networks, ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 192.168.7.15/24 --equals=192.168.7.5/24`, `ipcalc 192.168.7.15 255.255.255.0 --equals=192.168.7.5/255.255.255.0`, `ipcalc --class-prefix 10.0.0.0/8 --equals=10.9.9.9` → exit 0 — positive ; test: EqualsHostBits, EqualsNetmask, EqualsClassPrefix
**Links:** REQ-CLI-INPUT-001, REQ-CLI-ERR-007, REQ-CLI-ERR-008, REQ-CLI-OUTPUT-009

### REQ-CLI-ERR-007
**Requirement:** ipcalc MUST reject a NET argument of `--equals`,
`--subnet-of`, or `--overlaps` that the input address parser would
reject — malformed address, prefix out of range for its family, non-
contiguous netmask, or an address of the other family when `-4` or
`-6` is given — with the same diagnostic the parser prints for the
input address (e.g. `ipcalc: bad IPv4 address: <NET>`) and exit status
2 (REQ-CLI-OUTPUT-009).
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:parse_network (`str_to_prefix`, `get_ipv4_info`/`get_ipv6_info` on NET)
**Acceptance:** `ipcalc 10.0.0.0/8 --equals=1.2.3`, `ipcalc ::/0 --subnet-of=::/129`, `ipcalc 10.0.0.0/8 --overlaps=10.0.0.0/255.0.255.0`, `ipcalc -4 10.0.0.0/8 --overlaps=::/0` → diagnostic, exit 2 — negative ; test: EqualsBadAddress, SubnetOfBadPrefixIPv6, OverlapsBadNetmask, OverlapsForcedFamily
**Links:** REQ-CLI-INPUT-006, REQ-CLI-ERR-006

### REQ-CLI-ERR-008
**Requirement:** ipcalc MUST NOT accept more than one of `--equals`,
`--subnet-of`, and `--overlaps` in one invocation, including the same
option given twice; it MUST print `ipcalc: you cannot mix these options`
to stderr and exit with status 2 (REQ-CLI-OUTPUT-009), so that a
compound condition is never silently reduced to one comparison.
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** ipcalc.c:main (`ncmps > 1`)
**Acceptance:** `ipcalc 10.0.0.0/8 --equals=10.0.0.0/8 --overlaps=10.0.0.0/8`, `ipcalc 10.0.0.0/8 --equals=10.0.0.0/8 --equals=10.0.0.0/8` → exit 2 — negative ; test: Equals-Overlaps, Equals-Equals
**Links:** REQ-CLI-ERR-001

### REQ-CLI-OUTPUT-009
**Requirement:** With `--equals`, `--subnet-of`, or `--overlaps`, ipcalc
MUST exit with status 0 when the comparison is true, with status 1 when
it is false, and with status 2 when it cannot compare — any error that
exits with status 1 in the other modes, including an unrecognized
option or missing option argument (REQ-CLI-ERR-002), a missing input
address (REQ-CLI-ERR-003), conflicting options (REQ-CLI-ERR-001,
REQ-CLI-ERR-005, REQ-CLI-ERR-008), a malformed input address or NET
(REQ-INFO-INPUT-001 to REQ-INFO-INPUT-003, REQ-CLI-ERR-007), and a
memory allocation failure. A true or false result MUST be printed as
REQ-CLI-OUTPUT-010 specifies; an error MUST print its usual diagnostic
instead, subject to `-s` (REQ-CLI-ERR-006). This follows `cmp(1)`, so
that a caller can tell "different" from "could not compare" and decide
what to do with each.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:exit_failure, ipcalc.c:main (`badOption`, `case APP_COMPARE`), netcompare.c:compare_networks; ipcalc-utils.c:safe_strdup, ipcalc-utils.c:safe_asprintf
**Acceptance:** `ipcalc 192.168.7.15/24 --equals=192.168.7.5/24` → exit 0; `ipcalc 192.168.1.15/24 --overlaps=192.168.7.1/24` → exit 1; `ipcalc 1.2.3 --equals=1.2.3.0/24`, `ipcalc 10.0.0.0/8 --overlaps=10.0.0.0/8 --bogus`, `ipcalc --bogus 10.0.0.0/8 --overlaps=10.0.0.0/8`, `ipcalc 10.0.0.0/8 --equals`, `ipcalc --subnet-of=10.0.0.0/8`, `ipcalc -4 -6 10.0.0.0/8 --equals=10.0.0.0/8` → exit 2 — positive, negative ; test: EqualsHostBits, OverlapsDisjoint, EqualsBadInputAddress, Overlaps-UnknownOption, UnknownOption-Overlaps, EqualsMissingArgument, SubnetOfNoAddress, Equals-IPv4-IPv6 (no test for allocation failure)
**Links:** REQ-CLI-OUTPUT-010, REQ-CLI-ERR-001, REQ-CLI-ERR-002, REQ-CLI-ERR-003, REQ-INFO-CALC-014, REQ-INFO-CALC-015, REQ-INFO-CALC-016, REQ-INFO-CALC-017

### REQ-CLI-OUTPUT-010
**Requirement:** In comparison mode ipcalc MUST print the result to
stdout as one line, where NETWORK and NET stand for the input network
and NET written as `<network address>/<prefix>`:

| Option | True | False |
|--------|------|-------|
| `--equals` | `The networks are equal` | `The networks are not equal` |
| `--subnet-of` | `NETWORK is a subnet of NET` | `NETWORK is not a subnet of NET` |
| `--overlaps` | `The networks overlap` | `The networks do not overlap` |

With `--no-decorate`, `-j`, or `-s` it MUST print nothing. The line
MUST NOT contain color codes. This applies to both IPv4 and IPv6, and to networks of different
families (REQ-INFO-CALC-017), so that a user sees the answer without
inspecting the exit status.
**Strength:** MUST
**Status:** DERIVED
**Source:** netcompare.c:print_comparison, ipcalc.c:default_printf; ipcalc.1.md#Options (`--equals`)
**Acceptance:** `ipcalc 192.168.7.15/24 --equals=192.168.7.5/24` → `The networks are equal`; `ipcalc 192.168.0.1/24 --subnet-of=192.168.1.15/23` → `192.168.0.0/24 is a subnet of 192.168.0.0/23`; `ipcalc 2001:db8::/32 --subnet-of=2001:db8:1::/48` → `2001:db8::/32 is not a subnet of 2001:db8:1::/48`; `ipcalc 192.168.0.0/24 --overlaps=192.168.1.0/24` → `The networks do not overlap`; `ipcalc --no-decorate 192.168.7.15/24 --equals=192.168.7.5/25` → no output, exit 1; `ipcalc --no-decorate 192.168.0.1/24 --subnet-of=192.168.1.15/23`, `ipcalc -j 192.168.7.15/24 --equals=192.168.7.5/24` → no output, exit 0 — positive ; test: EqualsHostBits, SubnetOf, SubnetOfSupernetIPv6, OverlapsAdjacent, EqualsNoDecorate, SubnetOfNoDecorate, EqualsJson
**Links:** REQ-CLI-OUTPUT-009, REQ-CLI-OUTPUT-006, REQ-CLI-ERR-006

## Output formats

### REQ-CLI-OUTPUT-002
**Requirement:** In info mode, ipcalc MUST print the summary output
(REQ-INFO-OUTPUT-001) when no field-selecting option is given — i.e.
when only `-i`, `--all-info`, `-r`, `-4`, `-6`, `--class-prefix`,
`--no-decorate`, `-j`, or `-s` are present — and `NAME=value` output
(REQ-CLI-OUTPUT-004) otherwise.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`FLAGS_TO_IGNORE_MASK`, `FLAG_SHOW_MODERN_INFO`); ipcalc.h:FLAGS_TO_IGNORE
**Acceptance:** `ipcalc 192.168.2.0/24` → summary; `ipcalc -m 192.168.1.1/24` → `NETMASK=255.255.255.0` — positive ; test: StandardInfo, CalculateNetmaskPrefix24
**Links:** REQ-CLI-OUTPUT-003

### REQ-CLI-OUTPUT-003
**Requirement:** When `-j`/`--json` is given in info mode, ipcalc MUST
print the summary as a JSON object (REQ-INFO-OUTPUT-001) regardless of
which field-selecting options are given. `-j` MUST also select JSON
output in split and deaggregate modes; in comparison mode it MUST
suppress the result line (REQ-CLI-OUTPUT-010).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`(flags & ENV_INFO_MASK) && (flags & FLAG_JSON)` — always true when `FLAG_JSON` is set); ipcalc.1.md#Options (`--json`)
**Acceptance:** `ipcalc -j -n 2a03:2880:20:4f06:face:b00c:0:1/56` → same output as `ipcalc -j -i …/56`; `ipcalc -j -d 192.168.2.33-192.168.3.2` → JSON object — positive ; test: JsonTestHumanReadableSpecificInfo, DeaggregateJson
**Links:** REQ-CLI-OUTPUT-006

### REQ-CLI-OUTPUT-004
**Requirement:** In `NAME=value` output ipcalc MUST print one line per
selected field, in this fixed order regardless of option order:
`ADDRESS` (`-a`), `NETMASK` (`-m`), `PREFIX` (`-p`), `BROADCAST` (`-b`,
IPv4 only), `NETWORK` (`-n`), `CIDR` (`--cidr`), `REVERSEDNS`
(`--reverse-dns`), `MINADDR` (`--minaddr`), `MAXADDR` (`--maxaddr`),
`ADDRSPACE` (`--addrspace`), `ADDRESSES` (`--addresses`), `HOSTNAME` (`-h`), `ADDRESS` (`-o`), then
`COUNTRYCODE`, `COUNTRY`, `CITY`, `COORDINATES` (`-g`).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`else if (!(flags & FLAG_SHOW_MODERN_INFO))` block)
**Acceptance:** `ipcalc -abmnp --minaddr --maxaddr --addresses 192.168.2.6/24` → tests/192.168.2.6 — positive ; test: SpecificInfoOutput, AllocateAddressSpacePrefix24
**Links:** REQ-CLI-OUTPUT-005, REQ-INFO-OUTPUT-003

### REQ-CLI-OUTPUT-005
**Requirement:** In `NAME=value` output ipcalc MUST double-quote a value
that contains a space for `ADDRSPACE`, `ADDRESSES`, `COUNTRY`, and
`CITY`, MUST always double-quote `COORDINATES`, and MUST print a
`REVERSEDNS` with more than one domain as a double-quoted,
space-separated list, so that the output can be passed to the shell's
`eval`.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`strchr(…, ' ')` checks, `COORDINATES_NAME`, REVERSEDNS branch); ipcalc.1.md#Options (`--reverse-dns`)
**Acceptance:** `ipcalc --addrspace -abmnp 193.92.150.3/24` → `ADDRSPACE=Internet`; `ipcalc --reverse-dns 2001:db8::/31` → `REVERSEDNS="8.b.d.0.1.0.0.2.ip6.arpa. 9.b.d.0.1.0.0.2.ip6.arpa."` — positive ; test: AllocateAddressSpacePrefix24, AllocateAddressSpacePrefix64, LookupReverseDNSFromPrefix31IPv6

### REQ-CLI-OUTPUT-006
**Requirement:** With `--no-decorate` and without `-j`, ipcalc MUST print
each `NAME=value` line without its `NAME=` prefix, print each reverse
DNS domain on its own line, and print the summary
output without color; `-j` MUST take precedence over `--no-decorate`.
A value that is quoted in `NAME=value` output because it contains a
space (`ADDRSPACE`, `ADDRESSES`, `COUNTRY`, `CITY`) or that is always
quoted (`COORDINATES`) MUST keep its quotes with `--no-decorate`, so
that a reader that splits at the first space still gets the whole
value.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`FLAG_NO_DECORATE` checks; `if ((flags & FLAG_JSON) && (flags & FLAG_NO_DECORATE)) flags &= ~FLAG_NO_DECORATE`; `strchr(…, ' ')` quoting); ipcalc.c:pretty_printf; ipcalc.1.md#Options (`--no-decorate`)
**Acceptance:** `ipcalc -abmnp --minaddr --maxaddr --addresses --no-decorate 192.168.2.7/24` → tests/192.168.2.7; `ipcalc --addrspace --no-decorate 10.0.0.1` → `"Private Use"`; `ipcalc --reverse-dns --no-decorate 2001:db8:abcd::/37` → one domain per line; `ipcalc -j --no-decorate -S 24 10.10.10.0/16` → same as `-j` — positive ; test: SpecificInfoOutputNoDecorate, SpecificInfoOutputNoDecorateQuotedAddrspace, LookupReverseDNSNoDecoratePrefix37IPv6, NoDecorateInfo, JsonNoDecorateSplitPrefix24

### REQ-CLI-OUTPUT-007
**Requirement:** ipcalc MUST emit ANSI color codes in the summary output
only when stdout is a terminal and the `NO_COLOR` environment variable
is unset, and MUST NOT emit them in split, deaggregate, comparison,
`NAME=value`, or JSON output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`isatty(STDOUT_FILENO) && getenv("NO_COLOR") == 0`, set after the split/deaggregate/check returns); ipcalc.c:va_color_printf
**Acceptance:** any test in tests/meson.build (stdout not a TTY) → no escape codes — positive ; test: all `--test-outfile` tests (no TTY test exists)

### REQ-CLI-OUTPUT-008
**Requirement:** JSON output MUST be a single JSON object whose values
are all JSON strings or arrays of strings — including numeric values
such as `PREFIX`, `ADDRESSES`, and `NETS` — so that consumers can rely
on stable value types. Every string value MUST be escaped as RFC 8259
requires — `"` and `\` preceded by a backslash, and control characters
U+0000–U+001F written as `\u00XX` — so that the output is valid JSON
even when a value obtained from DNS (`HOSTNAME`) or from a geo database
(`COUNTRY`, `CITY`) contains such characters. Other bytes MUST be
copied unchanged; both geo backends return UTF-8.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:va_json_printf, ipcalc.c:json_print_string, array_start (separator when the array is not the first field), output_start/output_stop; ipcalc-geoip.c (`GEOIP_CHARSET_UTF8`); ipcalc-maxmind.c (`MMDB_DATA_TYPE_UTF8_STRING`)
**Acceptance:** `ipcalc --all-info --json 192.168.2.7/24` → tests/json-all-info-192.168.2.7-24 (`"PREFIX":"24"`, `"REVERSEDNS":["…"]`); `ipcalc -j -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → valid JSON whose `HOSTNAME` decodes to that name — positive ; test: JsonAllInfoReverseDNSPrefix24IPv4, JsonSplitPrefix26, JsonSplitVLSMUnused, DeaggregateJson, JsonEscapeHostname (Linux only, needs `jq`; resolution faked with an `LD_PRELOAD` shim, tests/fake-getnameinfo.c)
**Links:** REQ-INFO-OUTPUT-005, REQ-GEO-OUTPUT-001

## Documentation consistency

### REQ-CLI-COMPAT-001
**Requirement:** Every option ipcalc accepts MUST be described in
`ipcalc.1.md` and in the `--help` output, with the same argument
syntax.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options
**Acceptance:** review: each `long_options` entry appears in `usage(1)` and in ipcalc.1.md — positive ; test: none

### REQ-CLI-COMPAT-002
**Requirement:** The `--usage` summary SHOULD list every option.
**Strength:** SHOULD
**Status:** REVIEW
**Source:** ipcalc.c:usage (`verbose == 0` branch)
**Acceptance:** `ipcalc --usage` → lists `-S`, `--split-hosts`, `-d`, `--no-decorate` — positive ; test: none

> [REVIEW: drift] The brief usage omits `-S/--split`, `--split-hosts`,
> `-d/--deaggregate`, and `--no-decorate`.

## Open items

`REVIEW`: REQ-CLI-ERR-006 (unguarded messages), REQ-CLI-COMPAT-002
(`--usage` drift).
`UNDOCUMENTED`: extra arguments ignored (REQ-CLI-INPUT-004).
Missing tests: REQ-CLI-INPUT-002, REQ-CLI-ERR-003, REQ-CLI-ERR-004,
REQ-CLI-ERR-006, `-v` mixed with another mode.
