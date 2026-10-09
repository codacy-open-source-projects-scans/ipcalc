---
title: Project policy — build configurations and tests
id-prefix: REQ-GEN
sources:
  - AGENTS.md
  - meson.build, meson_options.txt, Makefile
  - .gitlab-ci.yml
  - tests/meson.build, tests/ipcalc-testrunner.sh
---

# Project policy — build configurations and tests

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
These requirements govern how ipcalc is built and tested; they apply to
every other document.

### REQ-GEN-BUILD-001
**Requirement:** The source MUST compile without warnings under
`-O2 -g -Werror` in every configuration CI builds: no geo backend;
libGeoIP linked; libGeoIP loaded at run time; libmaxminddb linked;
libmaxminddb loaded at run time — with both meson and the legacy
Makefile — and with
`-Db_sanitize=address,undefined -Dc_args="-fno-sanitize-recover=undefined" -Dwerror=true`.
**Strength:** MUST
**Status:** DERIVED
**Source:** .gitlab-ci.yml (jobs fedora.nogeo, fedora.geoip, fedora.dyn_geoip, fedora.maxmind, fedora.dyn_maxmind, fedora.sanitizers); AGENTS.md#Build and test
**Acceptance:** all CI jobs pass — positive ; test: CI
**Links:** REQ-GEO-BUILD-001

### REQ-GEN-BUILD-002
**Requirement:** The legacy Makefile MUST build ipcalc by compiling every
source file, so each geo backend source MUST compile to nothing when its
`USE_*` macro is undefined, and the Makefile MUST take the version
string from `meson.build`.
**Strength:** MUST
**Status:** DERIVED
**Source:** Makefile (`ipcalc:` rule lists all `.c` files; `VERSION=$(shell cat meson.build|…)`); ipcalc-maxmind.c (`#ifdef USE_MAXMIND`), ipcalc-geoip.c (`#ifdef USE_GEOIP`); AGENTS.md#Build and test
**Acceptance:** `make USE_MAXMIND=no USE_GEOIP=no USE_RUNTIME_LINKING=no && ./ipcalc -v` → `ipcalc <meson version>` — positive ; test: CI (`make` step of every fedora.* job)

### REQ-GEN-TEST-001
**Requirement:** Every test MUST be registered in `tests/meson.build` and
run through `tests/ipcalc-testrunner.sh` in one of its modes
(`--test-success`, `--test-failure`, `--test-status`, `--test-output`,
`--test-outfile`, `--test-equal`); `--test-outfile` expected-output files MUST live in
`tests/`, be named after the command's arguments, and contain the
combined stdout and stderr of the command run with stdout not a
terminal.
**Strength:** MUST
**Status:** DERIVED
**Source:** tests/meson.build; tests/ipcalc-testrunner.sh (`TestOutput` captures `2>&1`); AGENTS.md#Tests
**Acceptance:** `meson test -C build --list` lists every test; `ninja -C build test` passes — positive ; test: all

### REQ-GEN-TEST-002
**Requirement:** Each requirement's **Acceptance** criteria MUST be
covered by a test in `tests/meson.build`, including a `--test-failure`
(or output-checking) test for each rejected input or option
combination, and a bug fix MUST add a test that fails without the fix.
**Strength:** MUST
**Status:** DERIVED
**Source:** AGENTS.md#Requirements-first workflow; contrib/ai/personas/ipcalc-dev.md (Protocol: Testing)
**Acceptance:** review: every `REQ-*` cites a test, or lists it under its document's "Missing tests" — positive ; test: none

### REQ-GEN-TEST-003
**Requirement:** A test MUST check the property its name claims; a test
that greps output MUST match a pattern that cannot be satisfied by a
different field.
**Strength:** MUST
**Status:** DERIVED
**Source:** tests/meson.build (Random, RandomIPv6Implicit, RandomIPv6Explicit anchor on `^Network:` and `^Address:`)
**Acceptance:** review of `--test-success … | grep` tests; `ipcalc -r 32` or a non-private network fails the Random test's check — negative ; test: Random, RandomIPv6Implicit, RandomIPv6Explicit
**Links:** REQ-RANDOM-OUTPUT-001

### REQ-GEN-TEST-004
**Requirement:** The test suite MUST pass under AddressSanitizer and
UndefinedBehaviorSanitizer with undefined behavior made fatal, since
address and prefix arithmetic is the recurring source of defects and
an undefined shift can produce the expected output on one platform
and not another.
**Strength:** MUST
**Status:** DERIVED
**Source:** .gitlab-ci.yml (job fedora.sanitizers: `-Db_sanitize=address,undefined -Dc_args="-fno-sanitize-recover=undefined"`, `ninja -C build test`); contrib/ai/protocols/memory-safety-c.md (ipcalc extension, Phase 4)
**Acceptance:** `meson setup b -Db_sanitize=address,undefined -Dc_args="-fno-sanitize-recover=undefined" && ninja -C b test` → all pass — positive ; test: CI (fedora.sanitizers)
**Links:** REQ-GEN-BUILD-001

### REQ-GEN-TEST-005
**Requirement:** Every test in `tests/meson.build` MUST run in at least
one CI job, and each CI job MUST provide every external program used by
the tests its build configuration registers, so that no test exists
that CI never executes.
**Strength:** MUST
**Status:** DERIVED
**Source:** .gitlab-ci.yml (fedora.* jobs, `.fedora.latest.template` `dnf install` includes `jq`); tests/meson.build
**Acceptance:** the union of `meson test -C build --list` over all CI jobs
equals the set of `test(...)` names in `tests/meson.build` — positive ;
removing `jq` from the CI packages makes JsonEscapeHostname fail
(REQ-GEN-TEST-007) — negative ;
test: CI
**Links:** REQ-GEN-TEST-006, REQ-GEN-TEST-007

### REQ-GEN-TEST-006
**Requirement:** Every CI job built with sanitizers MUST run every test
its build configuration and platform allow, and MUST select its geo
backend explicitly rather than rely on auto-detection, so that sanitizer
coverage cannot shrink silently when a dependency changes.
**Strength:** MUST
**Status:** DERIVED
**Source:** .gitlab-ci.yml (fedora.sanitizers: `-Duse_maxminddb=enabled -Duse_geoip=disabled -Duse_runtime_linking=enabled`)
**Acceptance:** in fedora.sanitizers, `meson test -C build --list`
contains every test of fedora.dyn_maxmind's configuration, including
JsonEscapeHostname — positive ; with libmaxminddb uninstalled, the job's
`meson setup` fails instead of building another backend — negative ;
test: CI (fedora.sanitizers)
**Links:** REQ-GEN-TEST-004, REQ-GEN-TEST-005

### REQ-GEN-TEST-007
**Requirement:** In a build from the git source tree, a test that needs
an external program (e.g. `jq`) MUST be registered regardless of whether
that program is installed, and MUST fail, naming the missing program,
when it is not found; the absence of the program MUST NOT skip or
unregister the test, nor fail `meson setup`. Only the build
configuration (geo backend) and the host platform MAY determine whether
a test is registered. This requirement does not apply to release
tarballs or installed files.
**Strength:** MUST
**Status:** DERIVED
**Source:** tests/meson.build (JsonEscapeHostname: registered on Linux, checks `command -v jq`)
**Acceptance:** with jq on `PATH`, JsonEscapeHostname runs and passes —
positive ; without jq, `meson setup` succeeds and `meson test -C build
JsonEscapeHostname` fails with a log naming `jq` — negative ;
test: JsonEscapeHostname
**Links:** REQ-GEN-TEST-001, REQ-GEN-TEST-005

## Open items

None.
