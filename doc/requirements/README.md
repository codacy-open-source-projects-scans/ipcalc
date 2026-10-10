# ipcalc Requirements

This directory is the normative description of what ipcalc must do, in
atomic, testable statements with RFC 2119 keywords, source citations,
and acceptance criteria. It complements `ipcalc.1.md`, which describes
the same behavior for users.

When the code, `ipcalc.1.md`, and a requirement disagree, treat it as a
`REVIEW` item, not as an automatic override: resolve by reading the
cited source and, if still unclear, ask a maintainer.

## Generation protocols

Requirements are written with the protocols in `contrib/ai/protocols/`:

| Protocol | Use it when |
|----------|-------------|
| `requirements-from-implementation.md` | Documenting behavior ipcalc already has (status `DERIVED`, `REVIEW`, `AMBIGUOUS`, or `UNDOCUMENTED`). |
| `requirements-elicitation.md` | Specifying new or changed behavior from a feature request or bug report, before the code exists. |

Whichever protocol is used, a new entry MUST match the ID scheme,
category tags, and per-requirement format below and the style of the
rest of its document.

## Document map

A document is created the first time a requirement in its area is
written.

| Document | ID prefix | Covers | Main sources |
|----------|-----------|--------|--------------|
| `general.md` | `REQ-GEN-` | Project policy: build configurations, portability, test conventions | `AGENTS.md`, `meson.build`, `Makefile`, `.gitlab-ci.yml`, `tests/meson.build` |
| `cli.md` | `REQ-CLI-` | Options, modes (`enum app_t`), rejected option combinations, output formats (default, `NAME=value`, `--no-decorate`, `--json`, color), exit status | `ipcalc.c` (`main()`, `usage()`, printf helpers), `ipcalc.h`, `ipcalc.1.md` |
| `info.md` | `REQ-INFO-` | Address/network parsing and the computed fields of info and check modes: network, broadcast, host range, counts, class, address space, reverse DNS, hostname lookups | `ipcalc.c` (`get_ipv4_info()`, `get_ipv6_info()`), `ipv6.c`, `ipcalc-reverse.c` |
| `split.md` | `REQ-SPLIT-` | `-S/--split`, `--split-hosts` | `netsplit.c` (`show_split_networks()`), `ipcalc.c` (`main()`, `parse_split_req()`, `parse_split_hosts()`, `str_to_prefix()`) |
| `deaggregate.md` | `REQ-DEAGG-` | `-d/--deaggregate` | `deaggregate.c` |
| `random.md` | `REQ-RANDOM-` | `-r/--random-private` network generation | `ipcalc.c` (`generate_ip_network()`) |
| `geo.md` | `REQ-GEO-` | Geo-IP output and behavior per backend, including no backend | `ipcalc-maxmind.c`, `ipcalc.h` |

## ID scheme

```
REQ-<PREFIX>-<CATEGORY>-<NNN>
```

- `<PREFIX>` identifies the document (table above).
- `<CATEGORY>` is one of:

  | Tag | Domain |
  |-----|--------|
  | `INPUT` | Accepted and rejected arguments and address syntax |
  | `CALC` | Computed values and their boundary behavior |
  | `OUTPUT` | Printed fields, names, JSON keys and types, formatting |
  | `ERR` | Error handling, diagnostics, exit status |
  | `COMPAT` | Compatibility with earlier ipcalc releases or other ipcalc implementations |
  | `BUILD` | Build configurations and portability |
  | `TEST` | Test authorship conventions |

- `<NNN>` is a 3-digit sequence number, unique within
  `<PREFIX>-<CATEGORY>` and never reused: a removed requirement is
  marked `WITHDRAWN`, not renumbered.

Examples: `REQ-SPLIT-CALC-003`, `REQ-CLI-ERR-001`, `REQ-INFO-OUTPUT-012`.

## Status legend

| Status | Meaning |
|--------|---------|
| `DERIVED` | Directly supported by current code, tests, or `ipcalc.1.md`; no open questions. |
| `SPECIFIED` | Written ahead of the code (requirements-elicitation); not yet implemented. Becomes `DERIVED` when the implementing commit lands. |
| `REVIEW` | Behavior observed but contradicts documentation, another requirement, or looks like a defect — needs a maintainer decision. |
| `AMBIGUOUS` | Cannot be classified as essential or incidental without a maintainer decision; both interpretations are given. |
| `UNDOCUMENTED` | Behavior exists in code with no documentation, test, or evident purpose. |
| `WITHDRAWN` | No longer applies; kept for ID stability with a note explaining why. |

## Per-requirement format

```markdown
### REQ-<PREFIX>-<CAT>-<NNN>
**Requirement:** ipcalc MUST/SHOULD/MAY <behavior> when <condition>,
so that <rationale>.
**Strength:** MUST | SHOULD | MAY | MUST NOT | SHOULD NOT
**Status:** DERIVED | SPECIFIED | REVIEW | AMBIGUOUS | UNDOCUMENTED | WITHDRAWN
**Source:** <file>:<function or line> [, ...] ; ipcalc.1.md#<section>
**Acceptance:** `ipcalc <args>` → <expected output or exit status> —
  positive | negative ; test: <name in tests/meson.build> | none
**Links:** <related REQ-IDs>
```

## Document frontmatter

```yaml
---
title: <short title>
id-prefix: REQ-<PREFIX>
sources:
  - <file or glob>
---
```

## Conventions

- **Requirements describe observable behavior** — what a user invokes
  and what ipcalc prints or returns — and cite functions only as
  **Source** evidence.
- **Negative requirements are mandatory for `INPUT` and `ERR`**: for
  every accepted input form, state what is rejected and the exit
  status.
- **Field names and JSON types are contracts.** A requirement on an
  output field names it as printed in `NAME=value` mode and as the JSON
  key.
- **Both families.** A requirement states whether it applies to IPv4,
  IPv6, or both.
- **Normative language uses RFC 2119 keywords only** (MUST, MUST NOT,
  SHALL, SHALL NOT, SHOULD, SHOULD NOT, MAY, REQUIRED, RECOMMENDED,
  OPTIONAL). Informal equivalents ("needs to", "will", "can") MUST NOT
  express a normative obligation.

## Glossary

A requirement MUST NOT redefine a term listed here. If it needs a
meaning not covered, add the term here first.

| Term | Definition |
|------|------------|
| input address | The address as given on the command line, before any prefix is applied. |
| network address | The input address with all host bits cleared for the effective prefix. |
| hosts | Disambiguate per use: **usable hosts** (the `Hosts/Net` count and `HostMin`–`HostMax` range) or **addresses** (all 2^(width−prefix) addresses in the network). A requirement MUST state the /31, /32, /127 and /128 behavior explicitly. |
| decorated output | The default human-readable output, colored only when stdout is a TTY and `NO_COLOR` is unset. |
| `NAME=value` output | The shell-variable style output produced when specific info options (e.g. `-n`, `-b`) are given. |
| invalid input | Not a standalone term: name the input class (malformed address, prefix out of range for the family, conflicting options, …) and the exit status. |
| release | A version of ipcalc with a git tag (`1.0.0` or later). Behavior present only in unreleased commits is not a release's behavior. |
| information item | A value ipcalc prints — an address, mask, prefix, count, name, or list — as opposed to its presentation: label, order, alignment, separators, blank lines, and color. Two outputs carry the same information item when the value can be read from both, e.g. the prefix in `Netmask: 255.255.255.0 = 24` and in `Network: 192.168.2.0/24`. |
