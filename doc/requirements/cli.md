---
title: Command line, modes, output formats, and exit status
id-prefix: REQ-CLI
sources:
  - ipcalc.c (main, usage, long_options, output and printf helpers)
  - netcompare.c (compare_networks)
  - ipcalc.h (enum app_t, FLAG_*)
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
`--wildcard`, `--minaddr`, `--maxaddr`, `--addresses`, `--addrspace`), and MUST use the info mode
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
**Requirement:** In info mode without `--format` (REQ-CLI-OUTPUT-011),
ipcalc MUST print the summary output
(REQ-INFO-OUTPUT-001) when no field-selecting option is given — i.e.
when only `-i`, `--all-info`, `-r`, `-4`, `-6`, `--class-prefix`,
`--no-decorate`, `-j`, or `-s` are present — and `NAME=value` output
(REQ-CLI-OUTPUT-004) otherwise.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:field_selecting_options, ipcalc.c:main (`!selected_fields`, `FLAG_SHOW_MODERN_INFO`)
**Acceptance:** `ipcalc 192.168.2.0/24` → summary; `ipcalc -m 192.168.1.1/24` → `NETMASK=255.255.255.0` — positive ; test: StandardInfo, CalculateNetmaskPrefix24
**Links:** REQ-CLI-OUTPUT-003, REQ-CLI-OUTPUT-012

### REQ-CLI-OUTPUT-003
**Requirement:** When `-j`/`--json` is given in info mode without
`--format` (REQ-CLI-OUTPUT-011), ipcalc MUST
print the summary as a JSON object (REQ-INFO-OUTPUT-003) whichever
field-selecting options are given; those options only add their fields
to it (e.g. `--reverse-dns`, `--wildcard`). `-j` MUST also select JSON
output in split and deaggregate modes; in comparison mode it MUST
suppress the result line (REQ-CLI-OUTPUT-010). `-j` is deprecated in
favor of `--format=json` (REQ-CLI-OUTPUT-014), but MUST keep this
behavior so that existing commands keep their output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`!formatStr && (flags & FLAG_JSON)` selecting `FLAG_SHOW_MODERN_INFO`); ipcalc.1.md#Deprecated options (`--json`)
**Acceptance:** `ipcalc -j -n 2a03:2880:20:4f06:face:b00c:0:1/56` → same output as `ipcalc -j -i …/56`; `ipcalc -j -d 192.168.2.33-192.168.3.2` → JSON object — positive ; test: JsonTestHumanReadableSpecificInfo, DeaggregateJson
**Links:** REQ-CLI-OUTPUT-006, REQ-CLI-OUTPUT-011, REQ-CLI-OUTPUT-014

### REQ-CLI-OUTPUT-004
**Requirement:** In `NAME=value` output ipcalc MUST print one line per
selected field, in this fixed order regardless of option order, which
is the order of the summary output (REQ-INFO-OUTPUT-003):
`ADDRESS` (`-a` or `-o`, printed once when both are given), `HOSTNAME`
(`-h`), `NETWORK` (`-n`), `NETMASK` (`-m`), `WILDCARD` (`--wildcard`,
IPv4 only), `PREFIX` (`-p`), `CIDR` (`--cidr`), `BROADCAST` (`-b`, IPv4
only), `REVERSEDNS` (`--reverse-dns`), `ADDRSPACE` (`--addrspace`),
`MINADDR` (`--minaddr`), `MAXADDR` (`--maxaddr`), `ADDRESSES`
(`--addresses`), then `COUNTRYCODE`, `COUNTRY`, `CITY`, `COORDINATES`
(`-g`).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`fields[]` order, `field_selected()`); ipcalc.c:main (`FORMAT_SHELL`)
**Acceptance:** `ipcalc -abmnp --minaddr --maxaddr --addresses 192.168.2.6/24` → tests/192.168.2.6; `ipcalc -m -n 10.1.2.3/8` → `NETWORK=10.0.0.0`, `NETMASK=255.0.0.0`; `ipcalc -a -o localhost -4` → one `ADDRESS=127.0.0.1` line — positive ; test: SpecificInfoOutput, AllocateAddressSpacePrefix24, FieldOrderNetworkNetmask, AddressAndLookupHost
**Links:** REQ-CLI-OUTPUT-005, REQ-INFO-OUTPUT-003

### REQ-CLI-OUTPUT-005
**Requirement:** In `NAME=value` output ipcalc MUST double-quote a value
that contains a byte other than an ASCII letter or digit, one of
`. _ : / - , + @ %`, or a byte of 0x80 or above (UTF-8), MUST precede
each `"`, `\`, `$` and `` ` `` inside the quotes with a backslash, MUST
always double-quote `COORDINATES`, and MUST print a `REVERSEDNS` with
more than one domain as a double-quoted, space-separated list, so that
the shell's `eval` assigns exactly the value, including a host name
from DNS or a geo name that contains shell characters.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_var_field, ipcalc.c:print_shell_value (`shell_safe()`), `ATTR_VAR_QUOTE_ALWAYS`; ipcalc.1.md#Options (`--reverse-dns`)
**Acceptance:** `ipcalc --addrspace -abmnp 193.92.150.3/24` → `ADDRSPACE=Internet`; `ipcalc --addrspace 10.0.0.1` → `ADDRSPACE="Private Use"`; `ipcalc --reverse-dns 2001:db8::/31` → `REVERSEDNS="8.b.d.0.1.0.0.2.ip6.arpa. 9.b.d.0.1.0.0.2.ip6.arpa."`; `ipcalc -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → `eval` sets `HOSTNAME` to that name — positive ; test: AllocateAddressSpacePrefix24, AllocateAddressSpacePrefix64, LookupReverseDNSFromPrefix31IPv6, ShellEscapeHostname (Linux only; tests/fake-getnameinfo.c)

### REQ-CLI-OUTPUT-006
**Requirement:** With `--no-decorate` and without `-j` or `--format`
(REQ-CLI-OUTPUT-011), ipcalc MUST print
each `NAME=value` line without its `NAME=` prefix, print each reverse
DNS domain on its own line, and print the summary
output without color; `-j` MUST take precedence over `--no-decorate`.
With `--no-decorate` ipcalc MUST double-quote a value that contains a
space for `ADDRSPACE`, `ADDRESSES`, `COUNTRY`, and `CITY`, MUST always
double-quote `COORDINATES`, and MUST print every other value unchanged,
without the quoting and escaping of REQ-CLI-OUTPUT-005, so that a
reader that splits at the first space still gets the whole value and a
script that reads a single value gets it verbatim. `--no-decorate` is
deprecated in favor of `--format=value` and `--format=human`
(REQ-CLI-OUTPUT-014), but MUST keep this behavior.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`if ((flags & FLAG_JSON) && (flags & FLAG_NO_DECORATE)) flags &= ~FLAG_NO_DECORATE`); ipcalc.c:main (`FORMAT_VALUE`, `FORMAT_HUMAN` with `FLAG_NO_DECORATE`); ipcalc.c:show_var_field (`bare`, `ATTR_VALUE_QUOTE_IF_SPACE`, `ATTR_VAR_QUOTE_ALWAYS`); ipcalc.c:pretty_printf; ipcalc.1.md#Deprecated options (`--no-decorate`)
**Acceptance:** `ipcalc -abmnp --minaddr --maxaddr --addresses --no-decorate 192.168.2.7/24` → tests/192.168.2.7; `ipcalc --addrspace --no-decorate 10.0.0.1` → `"Private Use"`; `ipcalc --no-decorate -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → exactly that name; `ipcalc --reverse-dns --no-decorate 2001:db8:abcd::/37` → one domain per line; `ipcalc -j --no-decorate -S 24 10.10.10.0/16` → same as `-j` — positive ; test: SpecificInfoOutputNoDecorate, SpecificInfoOutputNoDecorateQuotedAddrspace, LookupReverseDNSNoDecoratePrefix37IPv6, NoDecorateInfo, JsonNoDecorateSplitPrefix24, NoDecorateHostnameVerbatim (Linux only; tests/fake-getnameinfo.c)

### REQ-CLI-OUTPUT-007
**Requirement:** ipcalc MUST emit ANSI color codes in the summary output
and in `--format=human` output (REQ-CLI-OUTPUT-012) only when stdout
is a terminal and the `NO_COLOR` environment variable is unset, and
MUST NOT emit them with `--no-decorate` or in split, deaggregate,
comparison, `NAME=value`, value, or JSON output.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`isatty(STDOUT_FILENO) && getenv("NO_COLOR") == 0`, set after the split/deaggregate/check returns); ipcalc.c:va_color_printf; ipcalc.1.md#Environment (`NO_COLOR`)
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
copied unchanged; the geo backend returns UTF-8.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:va_json_printf, ipcalc.c:json_print_string, array_start (separator when the array is not the first field), output_start/output_stop; ipcalc-maxmind.c (`MMDB_DATA_TYPE_UTF8_STRING`)
**Acceptance:** `ipcalc --all-info --json 192.168.2.7/24` → tests/json-all-info-192.168.2.7-24 (`"PREFIX":"24"`, `"REVERSEDNS":["…"]`); `ipcalc -j -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → valid JSON whose `HOSTNAME` decodes to that name — positive ; test: JsonAllInfoReverseDNSPrefix24IPv4, JsonSplitPrefix26, JsonSplitVLSMUnused, DeaggregateJson, JsonEscapeHostname (Linux only, needs `jq`; resolution faked with an `LD_PRELOAD` shim, tests/fake-getnameinfo.c)
**Links:** REQ-INFO-OUTPUT-005, REQ-GEO-OUTPUT-001

### REQ-CLI-OUTPUT-011
**Requirement:** `--format=FORMAT` MUST select the output format, where
FORMAT is `human` (titled lines, as in the summary of
REQ-INFO-OUTPUT-001), `shell` (`NAME=value` lines, REQ-CLI-OUTPUT-005),
`json` (a JSON object, REQ-CLI-OUTPUT-008), or `value` (values only, as
with `--no-decorate`, REQ-CLI-OUTPUT-006), so that the format can be
chosen explicitly instead of being inferred from the other options.
Without `--format` the output MUST stay as REQ-CLI-OUTPUT-002, -003
and -006 describe. `-j` with `--format=json` and `--no-decorate` with
`--format=value` MUST be accepted as that same format.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`OPT_FORMAT`, `legacy_format()`); ipcalc.c:format_names, ipcalc.c:parse_format, ipcalc.c:select_format; ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options (`--format`)
**Acceptance:** `ipcalc --format=human 192.168.2.0/24` → tests/standard-192.168.2.0-24; `ipcalc --format=json --all-info 192.168.2.7/24` → tests/json-all-info-192.168.2.7-24; `ipcalc --format=shell -m -n 10.1.2.3/8` → same as `ipcalc -m -n 10.1.2.3/8`; `ipcalc --format=value -m -n 10.1.2.3/8` → same as with `--no-decorate`; `ipcalc --format=value -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → exactly that name; `ipcalc -j --format=json -n 10.1.2.3/8`, `ipcalc --no-decorate --format=value -n 10.1.2.3/8` → exit 0 — positive ; test: FormatHumanSummary, FormatJsonAllInfo, FormatShellFields, FormatValueFields, FormatValueHostnameVerbatim, FormatJsonWithJ, FormatValueWithNoDecorate
**Links:** REQ-CLI-OUTPUT-002, REQ-CLI-OUTPUT-012, REQ-CLI-OUTPUT-013, REQ-CLI-ERR-009

### REQ-CLI-OUTPUT-012
**Requirement:** With `--format` in info mode, the field-selecting
options (REQ-CLI-OUTPUT-004) MUST select exactly the fields printed, in
the order of REQ-CLI-OUTPUT-004, and without them ipcalc MUST print the
summary (REQ-INFO-OUTPUT-001, or with `--all-info` its full form), so
that what is printed does not depend on the format. A selected field
MUST be printed as its summary line in `human` format (e.g.
`Network:\t10.1.2.0/8`), as `NAME=value` in `shell`, as a JSON object
holding only the selected keys in `json`, and as its value in `value`.
The summary in `shell` format MUST print the keys of the JSON summary
(REQ-INFO-OUTPUT-003), in that order. `-o` given with `-i` or
`--all-info` MUST only supply the address whose summary is printed.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:select_format (`FLAG_RESOLVE_IP` with `FLAG_SHOW_MODERN_INFO`); ipcalc.c:main (`!selected_fields`); ipcalc.c:show_info_fields, ipcalc.c:field_selected, ipcalc.c:field_in_summary (`FORMAT_HUMAN`)
**Acceptance:** `ipcalc --format=json -m -n 192.168.2.7/24` → `{"NETWORK":"192.168.2.0","NETMASK":"255.255.255.0"}`; `ipcalc --format=human -m -n 192.168.2.7/24` → `Network:\t192.168.2.0/24`, `Netmask:\t255.255.255.0 = 24`; `ipcalc --format=shell 192.168.2.7/24` → tests/shell-192.168.2.7-24; `ipcalc --format=json --reverse-dns 2001:db8::/31` → `"REVERSEDNS"` array of two domains; `ipcalc --format=json -h 127.0.0.1` → `{"HOSTNAME":"localhost"}`; `ipcalc --format=json -4 -o localhost` → `{"ADDRESS":"127.0.0.1"}`; `ipcalc --format=json -i -4 -o localhost` → tests/hostname-localhost-ipv4-json — positive ; test: FormatJsonFields, FormatHumanFields, FormatShellSummary, FormatJsonReverseDNS, FormatJsonHostname, FormatJsonLookupHost, FormatJsonInfoLookupHost
**Links:** REQ-CLI-OUTPUT-011, REQ-INFO-OUTPUT-003, REQ-GEO-OUTPUT-001

### REQ-CLI-OUTPUT-013
**Requirement:** With `--format`, split (`-S`, `--split-hosts`) and
deaggregate (`-d`) output MUST be printed as without any format option
for `human`, as with `-j` for `json`, and as with `--no-decorate` for
`value`. Comparison mode MUST print its result line
(REQ-CLI-OUTPUT-010) with `--format=human`. Check mode and `-v` MUST
ignore `--format`.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:select_format (`FORMAT_JSON` → `FLAG_JSON`, `FORMAT_VALUE` → `FLAG_NO_DECORATE`), read by netsplit.c, deaggregate.c, netcompare.c
**Acceptance:** `ipcalc --format=value -S 26 192.168.5.45/24` → tests/nsplit-192.168.5.45-24-26; `ipcalc --format=json -S 26 192.168.5.45/24` → tests/json-split-192.168.5.45-24-26; `ipcalc --format=human -S 26 192.168.5.45/24` → tests/split-192.168.5.45-24-26; `ipcalc --format=value -d 192.168.2.7-192.168.2.13` → tests/deaggregate-192.168.2.7-192.168.2.13; `ipcalc --format=human 192.168.7.15/24 --equals=192.168.7.5/24` → `The networks are equal`, exit 0; `ipcalc -c --format=shell 127.0.0.1` → exit 0 — positive ; test: FormatValueSplit, FormatJsonSplit, FormatHumanSplit, FormatValueDeaggregate, FormatHumanEquals, FormatCheck
**Links:** REQ-CLI-OUTPUT-011, REQ-CLI-ERR-009

### REQ-CLI-OUTPUT-014
**Requirement:** When `-j` or `--no-decorate` is given without
`--format`, stderr is a terminal, and `-s` is not given, ipcalc MUST
print one line to stderr naming the replacement of the deprecated
option, so that interactive users learn `--format` while scripts,
whose stderr is not a terminal, see no change. The output and exit
status MUST be unchanged. The first row that applies is used:

| Given | Hint |
|-------|------|
| `-j` in comparison mode | `ipcalc: hint: -j is deprecated; use -s to print no result` |
| `-j` with a field-selecting option | `ipcalc: hint: -j is deprecated; use --format=json, which prints only the selected fields` |
| `-j` | `ipcalc: hint: -j is deprecated; use --format=json` |
| `--no-decorate` in comparison mode | `ipcalc: hint: --no-decorate is deprecated; use -s to print no result` |
| `--no-decorate` with a field-selecting option, `-S`, `--split-hosts` or `-d` | `ipcalc: hint: --no-decorate is deprecated; use --format=value` |
| `--no-decorate` | `ipcalc: hint: --no-decorate is deprecated; use --format=human` |

**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:suggest_format (`isatty(STDERR_FILENO)`, `beSilent`)
**Acceptance:** with a terminal on stderr (tests/fake-isatty.c): `ipcalc -j 10.0.0.1/24`, `ipcalc -j -m 10.0.0.1/24`, `ipcalc -j 10.0.0.0/8 --equals=10.0.0.0/8`, `ipcalc --no-decorate -m 10.0.0.1/24`, `ipcalc --no-decorate -S 26 10.0.0.0/24`, `ipcalc --no-decorate 10.0.0.1/24` → the hint on stderr — positive; `ipcalc --format=json -m 10.0.0.1/24`, `ipcalc -m -n 10.0.0.1/24`, `ipcalc --format=value -S 26 10.0.0.0/24`, `ipcalc -s -j 10.0.0.1/24` → no hint; every other test (stderr not a terminal) → no hint — negative ; test: HintJson, HintJsonFields, HintJsonEquals, HintNoDecorateFields, HintNoDecorateSplit, HintNoDecorateSummary, NoHintFormat, NoHintFields, NoHintFormatValueSplit, NoHintSilent
**Links:** REQ-CLI-OUTPUT-003, REQ-CLI-OUTPUT-006, REQ-CLI-OUTPUT-011, REQ-CLI-ERR-006, REQ-CLI-COMPAT-001

### REQ-CLI-ERR-009
**Requirement:** ipcalc MUST reject these uses of `--format`, printing
the message to stderr (subject to `-s`, REQ-CLI-ERR-006) and exiting
with status 1, or 2 with a comparison option (REQ-CLI-OUTPUT-009), so
that every accepted combination has a single meaning and each message
names the alternative:

| Condition | Message |
|-----------|---------|
| FORMAT is not one of the four names | `ipcalc: unknown output format: FORMAT; use human, shell, json or value` |
| two different `--format` values | `ipcalc: conflicting output formats: --format=A and --format=B` |
| `-j` with `--format` other than `json` | `ipcalc: conflicting output formats: -j and --format=B` |
| `--no-decorate` with `--format` other than `value` | `ipcalc: conflicting output formats: --no-decorate and --format=B` |
| `-i` or `--all-info` with a field-selecting option other than `-o` | `ipcalc: -i and --all-info cannot be combined with options that select fields` |
| `value` without a field-selecting option in info mode | `ipcalc: --format=value needs options that select fields; use --format=shell or --format=json for the summary` |
| `shell` with `-S`, `--split-hosts` or `-d` | `ipcalc: --format=shell is not supported with --split, --split-hosts or --deaggregate; use --format=value` |
| `json`, `value` or `shell` with a comparison option | `ipcalc: --format=B is not supported with --equals, --subnet-of or --overlaps` |

**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:parse_format, ipcalc.c:select_format, ipcalc.c:format_conflict (after option parsing); ipcalc.c:check_format_for_app (after the mode check)
**Acceptance:** `ipcalc --format=xml 10.0.0.1`, `ipcalc --format=json --format=shell 10.0.0.1`, `ipcalc -j --format=shell 10.0.0.1`, `ipcalc --no-decorate --format=json -m 10.0.0.1`, `ipcalc --format=json -i -m 10.0.0.1`, `ipcalc --format=human --all-info -n 10.0.0.1/24`, `ipcalc --format=value 10.0.0.1/24`, `ipcalc --format=shell -S 26 10.0.0.0/24`, `ipcalc --format=shell -d 10.0.0.1-10.0.0.9` → the message, exit 1; `ipcalc --format=json 10.0.0.0/8 --equals=10.0.0.0/8`, `ipcalc --format=value 10.0.0.0/8 --subnet-of=10.0.0.0/7`, `ipcalc --format=shell 10.0.0.0/8 --overlaps=10.0.0.0/7` → the message, exit 2; `ipcalc -s --format=xml 10.0.0.1` → no output, exit 1 — negative ; test: FormatUnknown, FormatTwice, FormatConflictJson, FormatConflictNoDecorate, FormatInfoWithField, FormatAllInfoWithField, FormatValueSummary, FormatShellSplit, FormatShellDeaggregate, FormatJsonEquals, FormatValueSubnetOf, FormatShellOverlaps, FormatUnknownSilent
**Links:** REQ-CLI-OUTPUT-011, REQ-CLI-OUTPUT-012, REQ-CLI-OUTPUT-013, REQ-CLI-ERR-006

## Compatibility with earlier releases

Written with `contrib/ai/protocols/requirements-elicitation.md`. These
requirements apply to every mode and to both IPv4 and IPv6; "an
invocation" is a command line (options and arguments) given the same
input, the same geo database, and the same DNS answers.

### REQ-CLI-COMPAT-003
**Requirement:** For an invocation that printed JSON in a release
(`-j`/`--json`, in any mode), every later version MUST print a JSON
object that contains every key that release printed, with the same key
name, the same JSON type (string or array of strings), and the value
in the same format (e.g. `PREFIX` as a decimal string, `NETWORK`
without the prefix, `REVERSEDNS` as an array). A later version MAY add
keys, and the relative order of keys is not part of this contract,
since JSON objects are unordered (RFC 8259), so that programs that
parse ipcalc's JSON keep working across updates.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields, ipcalc.c:show_json_field, ipcalc.c:va_json_printf; netsplit.c; deaggregate.c; tests/json-*
**Acceptance:** `ipcalc --all-info --json 192.168.2.7/24` → contains every key and value of the 1.1.0 tests/json-all-info-192.168.2.7-24 (current output adds `WILDCARD` and `CIDR`); `ipcalc -j -h 127.0.0.1` → contains every key and value of the 1.1.0 tests/ip-localhost-ipv4-json — positive ; a change that renames `ADDRSPACE`, drops `BROADCAST`, or prints `PREFIX` as a JSON number fails JsonAllInfoReverseDNSPrefix24IPv4 — negative ; test: every JSON `--test-outfile` test (e.g. JsonAllInfoReverseDNSPrefix24IPv4, IPIPv4LocalhostJson, JsonSplitPrefix26, DeaggregateJson); review: a patch may change an existing JSON expected-output file only by adding keys or by a change REQ-CLI-COMPAT-005 permits
**Links:** REQ-CLI-OUTPUT-008, REQ-INFO-OUTPUT-003, REQ-CLI-COMPAT-005

### REQ-CLI-COMPAT-004
**Requirement:** For an invocation that printed `NAME=value` output in
a release, every later version MUST print every `NAME=value` line that
release printed, with the same name, the same value in the same format,
and the same quoting (REQ-CLI-OUTPUT-005), so that scripts that `eval`
the output or look up a variable by name keep working across updates.
A later version MAY change the quoting of a value only where the
release's line did not assign that value under `eval` (e.g.
`HOSTNAME=a"b`), which is a bug that REQ-CLI-COMPAT-005 lets a patch
fix.
A later version MAY print the lines in a different order and MAY add
lines.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`NAME=value` loop), ipcalc.c:show_var_field
**Acceptance:** `ipcalc -abmnp 10.10.10.5/24` → every line of the 1.1.0 tests/192.168.1.5-24 (the current version prints `NETWORK=` second where 1.1.0 printed it fifth); `ipcalc -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → a line that `eval` assigns to `HOSTNAME`, where 1.1.0 printed it unquoted — positive ; a change that renames `NETMASK=`, drops `BROADCAST=`, or stops quoting `ADDRSPACE="Private Use"` — negative ; test: TestPrefix24, SpecificInfoOutput, AllocateAddressSpacePrefix24, FieldOrderNetworkNetmask, ShellEscapeHostnameDefault; review: a patch may change an existing `NAME=value` expected-output file only by reordering or adding lines, or by a change REQ-CLI-COMPAT-005 permits
**Links:** REQ-CLI-OUTPUT-004, REQ-CLI-OUTPUT-005, REQ-CLI-COMPAT-005, REQ-CLI-COMPAT-007

### REQ-CLI-COMPAT-005
**Requirement:** Every information item that a release printed for an
invocation — in the decorated, `--no-decorate`, `NAME=value`, or JSON
output — MUST be printed by every later version for the same
invocation, in the same output format. The test suite is the
gatekeeper: a patch MUST NOT delete a test or remove an information
item from an expected-output file in `tests/` unless the item was
printed because of a bug, i.e. it is wrong per a requirement in
`doc/requirements/`, or it was printed although none of the given
options requests it (e.g. a hostname printed without `-h` or `-o`).
Such a removal MUST come with a requirement describing the correct
behavior, a test, and a `NEWS` entry, so that users never lose
information on an update except when it was a defect.
**Strength:** MUST
**Status:** DERIVED
**Source:** tests/meson.build; tests/*; AGENTS.md#Requirements-first workflow
**Acceptance:** `ninja -C build test` passes with the expected-output files of the latest release unchanged, or changed only by additions or by documented bug-fix removals — positive ; a patch that drops the `Hosts/Net` line from tests/info-192.168.2.7 or deletes the Info test without a bug-fix requirement — negative ; test: all `--test-outfile` tests; review
**Links:** REQ-CLI-COMPAT-003, REQ-CLI-COMPAT-004, REQ-CLI-COMPAT-006, REQ-CLI-COMPAT-007, REQ-GEN-TEST-002

### REQ-CLI-COMPAT-006
**Requirement:** In the decorated and `--no-decorate` summary output,
and in the decorated split and deaggregate output, a later version MAY
change the presentation — labels, order of lines, alignment,
separators, blank lines, and color — and MAY add information items,
but MUST NOT remove an information item a release printed for the same
invocation except as REQ-CLI-COMPAT-005 permits, so that the
human-readable output can be improved without losing content.
**Strength:** MUST NOT
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (summary loop), ipcalc.c:show_text_field; netsplit.c; deaggregate.c
**Acceptance:** `ipcalc --all-info 192.168.2.7/24` → every information item of the 1.1.0 tests/all-info-192.168.2.7 (current output adds `Wildcard`) — positive ; a change after which `ipcalc -i 192.168.2.7/24` no longer prints the address `192.168.2.7` fails Info — negative ; test: Info, AllInfo, StandardInfo, NoDecorateAllInfo, TestHumanReadableLoopback; review: an edit of a summary expected-output file that removes a value
**Links:** REQ-CLI-COMPAT-005, REQ-INFO-OUTPUT-001

### REQ-CLI-COMPAT-007
**Requirement:** For an invocation with `--no-decorate` and without `-j`
that printed a single information item in a release — one
field-selecting option (e.g. `-n`, `--addrspace`, `--reverse-dns`), or
the subnet list of split mode or the network list of deaggregate
mode — every later version MUST print byte-identical stdout for it and
MUST NOT add lines or other information items to it, so that
`$(ipcalc --no-decorate -n …)` and scripts that read the list line by
line keep working across updates. With two or more field-selecting
options, `--no-decorate` output is covered by REQ-CLI-COMPAT-005 only.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields, ipcalc.c:show_var_field (`bare`); netsplit.c; deaggregate.c
**Acceptance:** `ipcalc --addrspace --no-decorate 10.0.0.1` → exactly `"Private Use"`; `ipcalc --wildcard --no-decorate 172.16.5.4/12` → exactly `0.15.255.255`; `ipcalc --cidr --no-decorate 2001:db8::1/64` → exactly `2001:db8::/64`; `ipcalc --reverse-dns --no-decorate 2001:db8:abcd::/37` → tests/reverse-dns-no-decorate-2001:db8:abcd::-37; `ipcalc --no-decorate -S 26 192.168.5.45/24` → tests/nsplit-192.168.5.45-24-26; `ipcalc --no-decorate -h 127.0.0.1` with 127.0.0.1 resolving to `a"b\c<TAB>d` → exactly that name, unquoted — positive ; a change after which `ipcalc -n --no-decorate 10.1.2.3/8` prints `10.0.0.0/8`, or prints the prefix on a second line — negative ; test: SpecificInfoOutputNoDecorateQuotedAddrspace, WildcardNoDecorate, CidrNoDecorateIPv6, LookupReverseDNSNoDecoratePrefix37IPv6, NoDecorateSplitPrefix26, NoDecorateHostnameVerbatim; review: a patch MUST NOT change the expected output of such a test except as REQ-CLI-COMPAT-005 permits
**Links:** REQ-CLI-OUTPUT-006, REQ-CLI-COMPAT-005

### REQ-CLI-COMPAT-008
**Requirement:** For an invocation with `-c`, `--equals`,
`--subnet-of`, or `--overlaps`, every later version MUST return the
exit status a release returned for it — 0 or 1 for `-c`
(REQ-CLI-OUTPUT-001), and 0, 1, or 2 for the comparison options
(REQ-CLI-OUTPUT-009) — whether that status reported success or
failure, since in these modes the exit status is the result. For an
invocation in any other mode — info (including `-r` and field-selecting
options), split, deaggregate, `-v`, `--help`, and `--usage` — that
exited with status 0 in a release, every later version MUST exit with
status 0, unless the invocation succeeded because of a bug, i.e. a
requirement in `doc/requirements/` says it must fail; such a change
MUST come with that requirement, a test, and a `NEWS` entry (e.g.
`-h` with a network, REQ-INFO-ERR-001). A later version MAY make an
invocation of these modes that failed in a release succeed. This
applies to IPv4 and IPv6, so that scripts that test `$?` or run under
`set -e` keep working across updates.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`case APP_CHECK_ADDRESS`, `case APP_COMPARE`, `badOption`), ipcalc.c:exit_failure; netcompare.c:compare_networks; tests/ipcalc-testrunner.sh (`TestOutputFile` requires exit status 0)
**Acceptance:** `ipcalc -c 127.0.0.1` → exit 0; `ipcalc -c -6 gggg::`, `ipcalc -c ::1/999` → exit 1; `ipcalc 192.168.7.15/24 --equals=192.168.7.5/24` → exit 0; `ipcalc 192.168.1.15/24 --overlaps=192.168.7.1/24` → exit 1; `ipcalc 1.2.3 --equals=1.2.3.0/24` → exit 2; every `--test-outfile` command (info, split, deaggregate) → exit 0, which the test runner checks — positive ; a change after which `ipcalc -c ::1/999` exits 0, `ipcalc 192.168.1.15/24 --overlaps=192.168.7.1/24` exits 2, or `ipcalc -i 192.168.2.7/24` exits 1 fails NoValidateInvalidPrefixIPv6, OverlapsDisjoint, or Info — negative ; test: ValidateLoopbackIPv4, ValidateLoopbackIPv6WithPrefix, NoValidateInvalidHexIPv6, NoValidateInvalidPrefixIPv6, EqualsHostBits, OverlapsDisjoint, EqualsBadInputAddress, all `--test-outfile` tests; review: a patch MUST NOT change the expected status of an existing `--test-success`, `--test-failure`, or `--test-status` test for these modes
**Links:** REQ-CLI-OUTPUT-001, REQ-CLI-OUTPUT-009, REQ-CLI-COMPAT-005, REQ-INFO-ERR-001, REQ-GEN-TEST-001

## Documentation consistency

### REQ-CLI-COMPAT-001
**Requirement:** Every option ipcalc accepts MUST be described in
`ipcalc.1.md` and in the `--help` output, with the same argument
syntax, except the deprecated `-j`/`--json` and `--no-decorate`
(REQ-CLI-OUTPUT-014). Those MUST be described only in the
"Deprecated options" section of `ipcalc.1.md`, each with its
`--format` replacement, and MUST NOT appear in `--help` or `--usage`,
so that users learn `--format` while the man page still explains
commands that use them.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:long_options, ipcalc.c:usage; ipcalc.1.md#Options, ipcalc.1.md#Deprecated options
**Acceptance:** review: each `long_options` entry other than `json` and `no-decorate` appears in `usage(1)` and in ipcalc.1.md#Options; `json` and `no-decorate` appear only in ipcalc.1.md#Deprecated options — positive ; test: none
**Links:** REQ-CLI-OUTPUT-014

### REQ-CLI-COMPAT-002
**Requirement:** The `--usage` summary SHOULD list every option except
the deprecated ones (REQ-CLI-COMPAT-001).
**Strength:** SHOULD
**Status:** REVIEW
**Source:** ipcalc.c:usage (`verbose == 0` branch)
**Acceptance:** `ipcalc --usage` → lists `-S`, `--split-hosts`, `-d`, `--format` — positive ; test: none

> [REVIEW: drift] The brief usage omits `-S/--split`, `--split-hosts`,
> and `-d/--deaggregate`.

## Open items

`REVIEW`: REQ-CLI-ERR-006 (unguarded messages), REQ-CLI-COMPAT-002
(`--usage` drift).
`UNDOCUMENTED`: extra arguments ignored (REQ-CLI-INPUT-004).
Missing tests: REQ-CLI-INPUT-002, REQ-CLI-ERR-003, REQ-CLI-ERR-004,
REQ-CLI-ERR-006, `-v` mixed with another mode.
