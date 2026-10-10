# AGENTS.md

This file provides guidance to AI Agents when working with code in this repository.

## Overview

`ipcalc` is a single-binary C command-line tool for IPv4/IPv6 network calculations (info, check, split, deaggregate, random network generation), with optional geo-IP lookup. Output is human-readable (colored), shell-variable style (`NAME=value`), or JSON.

## Requirements-first workflow

`doc/requirements/` is the normative description of what ipcalc must do (see `doc/requirements/README.md` for the document map, ID scheme, and per-requirement format). For any change that alters observable behavior — new feature, bug fix, or behavior-changing refactor — work in this order:

1. **Requirement first.**
   - Find the requirement(s) covering the area you're changing (search `doc/requirements/` for `REQ-*` IDs, or use the document map to find the file by mode).
   - If your change alters what an existing requirement describes, update that requirement first, following the protocol in `contrib/ai/protocols/` named in `doc/requirements/README.md`. Never leave a requirement describing the old behavior once the code changes. A requirement that is wrong for reasons unrelated to your change gets its own separate commit or series.
   - If no requirement covers the behavior, add one in the appropriate document (creating the document if the map lists it but it doesn't exist yet) — use `requirements-elicitation.md` for new behavior, `requirements-from-implementation.md` to first document the existing behavior of an area you're about to change.
   - For bug fixes: if the bug violates an existing requirement, cite its ID in the commit message. If the bug reveals a gap, add a requirement describing the *correct* behavior before fixing the code.
2. **Tests second.** Write or update the positive and negative tests implied by the requirement's **Acceptance** criteria before touching implementation code. For bug fixes, confirm the new test fails against the unmodified code.
3. **Code last.** Implement the change so the tests pass and the code, the requirement, `ipcalc.1.md`, `usage()`, and `NEWS` all agree.

A code change with no corresponding requirement update is incomplete, even if it builds and passes the tests.

Before declaring a change done, search `doc/requirements/` for requirements citing the files, functions, or options you touched and confirm each still holds — if one no longer does, either your change or that requirement is wrong (mark it `REVIEW` if unsure which); never leave code contradicting a `DERIVED` requirement. Run the full suite (`ninja -C build test`), not just the tests for the area you changed.

## AI personas and protocols

`contrib/ai/` holds prompt files imported from [PromptKit](https://github.com/microsoft/PromptKit), each with an ipcalc-specific extension section (see `contrib/ai/README.md` for provenance and how to update them). For extended work — features, bug investigation, code review, requirements maintenance — load `contrib/ai/personas/ipcalc-dev.md`, which indexes the protocols to apply, including the review protocol that walks the checklist below.

## Build and test

Meson/Ninja is the primary build:

```sh
meson setup build                    # auto-detects libmaxminddb, else no geo support
ninja -C build
ninja -C build test                  # full test suite
meson test -C build Split-Info       # run a single test by name (names come from tests/meson.build)
meson test -C build --list           # list test names
```

Geo backend options: `-Duse_maxminddb=enabled|disabled`, `-Duse_runtime_linking=enabled|disabled` (dlopen the geo library at runtime instead of linking). CI builds every combination with `-Dc_args="-O2 -g -Werror"` plus a sanitizer build (`-Db_sanitize=address,undefined -Dc_args="-fno-sanitize-recover=undefined" -Dwerror=true`) that must run every test its configuration allows, so code must be warning-free in all configurations, including with no geo backend.

The legacy `Makefile` must keep working too (CI runs it): `make USE_MAXMIND=no USE_RUNTIME_LINKING=no`. It compiles all `.c` files unconditionally, so the geo source file must compile to nothing when `USE_MAXMIND` is undefined. The version string is read from `meson.build`.

The man page `ipcalc.1` is generated from `ipcalc.1.md` with `ronn` (optional). Update `ipcalc.1.md` when changing CLI options, and add user-visible changes to `NEWS`.

## Architecture

- `ipcalc.c` — almost everything: `getopt_long` option parsing in `main()`, IPv4/IPv6 parsing into `ip_info_st` (`get_ipv4_info` / `get_ipv6_info`), address-space/class classification tables, random network generation, and all output formatting.
- `netsplit.c` — `-S/--split`: `show_split_networks_v4()` / `show_split_networks_v6()` split a network into equal subnets of the given prefix (separate implementations per family).
- `netcompare.c` — `--equals`/`--subnet-of`/`--overlaps`: `compare_networks()` compares the network `main()` parsed with the option argument.
- `deaggregate.c` — `-d/--deaggregate` (address range → minimal CIDR list).
- `ipcalc-reverse.c` — reverse DNS names; `ipcalc-utils.c` — `safe_asprintf`/`safe_strdup`/`safe_atoi`; `ipv6.c` — IPv6 helpers.
- `ipcalc-maxmind.c` — the libmaxminddb geo backend implementing `geo_ip_lookup()` / `geo_setup()`; `ipcalc.h` stubs these out when it is not built.

Behavior is driven by two things set in `main()`:
- `app` (`enum app_t` in `ipcalc.h`): which mode runs (info, check, split, deaggregate, version). Some option combinations across modes are rejected and are covered by `--test-failure` tests.
- `flags` (global bitmask, `FLAG_*` in `ipcalc.h`): what to show and how. With no specific `FLAG_SHOW_*` flags, the "modern" pretty summary (`FLAG_SHOW_MODERN_INFO`) is printed; specific flags select fields, printed as `NAME=value` by default. An option that selects a field is a row of `field_selecting_options[]`, which maps it to its `FLAG_SHOW_*` (or `FLAG_RESOLVE_*`) flag; giving one puts ipcalc in info mode and selects those fields instead of the summary. The output format (`enum output_format`: human, shell, JSON, value) comes from `--format` (`select_format()`), or, without it, from `-j`, `--no-decorate` and the selection (`legacy_format()`); `show_info_fields()` prints the selected fields in that format.

All output must go through the printf helpers. `default_printf`, `dist_printf` and `output_start`/`output_separate`/`output_stop` switch between colored, `--no-decorate`, and `--json` formats; `pretty_printf`/`pretty_dist_printf` print text only (colored or `--no-decorate`) and `json_printf`/`array_start`/`array_stop` print JSON only, so their caller chooses the format (as `show_info_fields()` does). JSON comma placement is tracked via the `jsonfirst` state (`JSON_FIRST`/`JSON_NEXT`/`JSON_ARRAY_*`) passed to each call. Color is disabled when stdout is not a TTY or `NO_COLOR` is set.

## Code style

- C99 standard; Linux kernel coding style (tabs, 8-space tab width, 80-column limit)
- Header guards: `#ifndef FILENAME_H` / `#define FILENAME_H` / `#endif /* FILENAME_H */`
- `#ifdef` in function bodies: ≤ 1 nesting level, ≤ 5 lines per branch; optional
  features use the stub pattern; `#endif` always annotated
- **Comments:** Prefer self-documenting code. Add a comment only when the *why* is
  non-obvious: a hidden constraint, a protocol expectation, or a workaround. Do not
  comment what the code does; well-named identifiers already do that. Older code
  does not always follow this; do not copy its comments as a model.

## Commit messages

- Describe what was changed in a short paragraph; do not include references to requirements.
- Every commit must have a `Signed-off-by: Name <email>` line (DCO requirement).
- Use `Resolves: #NNN` on the fixing commit; `Relates: #NNN` on related commits (e.g., tests).

## Tests

Tests are declared in `tests/meson.build` and executed by `tests/ipcalc-testrunner.sh` in one of these modes: `--test-success`, `--test-failure` (exit status only), `--test-status CODE CMD` (exact exit status), `--test-output`, `--test-equal`, and most commonly `--test-outfile CMD FILE`, which compares the full stdout+stderr of `CMD` with an expected-output file in `tests/` and requires exit status 0. Expected-output files are named after the arguments (e.g. `tests/i-10.0.0.1`, `tests/deaggregate-…`). To add a test, create the expected-output file and add a `test(...)` entry in `tests/meson.build`. Tests run with stdout not a TTY, so the expected output has no color codes.

`tests/debian-ipcalc/` holds the Debian ipcalc (Perl), against which the `DebianSplitHosts-*` tests compare `--split-hosts` (REQ-SPLIT-COMPAT-001). It is excluded from release tarballs via `.gitattributes`, and the tests are registered whenever it is present and fail if `perl` or its `bignum` module is missing; CI installs `perl-interpreter` and `perl-bignum`.

## Contribution checklist

Verify each item before declaring a change done; reviews check every box against the diff.

**Requirements (`doc/requirements/`):**
- [ ] Relevant `REQ-*` found or created before the code change
- [ ] Requirements whose behavior the patch changes are updated first
- [ ] No other `REQ-*` citing touched files/options is silently contradicted
- [ ] Code, requirement, `ipcalc.1.md`, and `usage()` agree

**Code:**
- [ ] Matches the surrounding style; comments follow **Code style** (*why* only, identifiers say *what*)
- [ ] All output goes through the printf helpers; JSON `jsonfirst` state threaded correctly
- [ ] Edge prefixes and address-space ends handled (no shift/overflow UB)
- [ ] Geo code compiles to nothing when `USE_MAXMIND` is undefined

**Change propagation:**
- [ ] Options: `getopt_long` table ↔ `usage()` ↔ `ipcalc.1.md` ↔ requirement ↔ test
- [ ] Output fields: default/modern ↔ `NAME=value` ↔ `--json` ↔ requirement ↔ expected-output files
- [ ] Geo backend (if touched): `ipcalc-maxmind.c` ↔ `ipcalc.h` stubs ↔ meson ↔ `Makefile`
- [ ] `NEWS` updated for user-visible changes

**Testing:**
- [ ] Positive test(s) registered in `tests/meson.build`
- [ ] Negative test(s) for rejected inputs/combinations
- [ ] Bug fix: test shown to fail before the fix
- [ ] `ninja -C build test` passes in full
- [ ] Legacy `make USE_MAXMIND=no USE_RUNTIME_LINKING=no` builds

**Commits:**
- [ ] Each commit self-contained, with `Signed-off-by:`
- [ ] Message describes the change in a short paragraph; `Resolves:`/`Relates:` cite the issue, if any
- [ ] A change's requirement update, tests, and code are in one commit; the requirements-first order is the order of work, not of commits
- [ ] Commit message cites the `REQ-*` IDs it adds, changes, or fixes
