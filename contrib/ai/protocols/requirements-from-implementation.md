<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) PromptKit Contributors -->

---
name: requirements-from-implementation
type: reasoning
description: >
  Systematic reasoning protocol for deriving structured requirements
  from existing source code. Transforms code understanding into
  testable, atomic requirements with acceptance criteria.
applicable_to:
  - reverse-engineer-requirements
  - review-code
---

# Protocol: Requirements from Implementation

Apply this protocol when deriving requirements from an existing codebase.
The goal is to produce a structured requirements document that captures
what the implementation provides — not how it provides it. Execute all
phases in order.

## Phase 1: API Surface Enumeration

Systematically catalog every public-facing element of the codebase:

1. **Functions and entry points**: Signatures, parameters, return types,
   error conditions. For each, note whether it is public API, internal,
   or a convenience wrapper.
2. **Types and data structures**: Structs, enums, unions, typedefs.
   Identify which are opaque (implementation detail) vs. transparent
   (part of the API contract).
3. **Metaprogramming and indirection constructs** (if applicable):
   Preprocessor macros (C/C++), decorators (Python), annotations (Java),
   attribute macros (Rust), code generation. Expand representative
   invocations to understand the actual behavior. Document parameters,
   their types, and constraints.
4. **Constants and configuration surfaces**: Compile-time switches,
   feature flags, tuning parameters. Identify which are user-facing
   configuration vs. internal implementation constants.
5. **Error handling patterns**: How does the API report errors? Return
   codes, errno, out-parameters, callbacks, exceptions? Catalog the
   error space.

Produce a structured enumeration (table or list) before proceeding.
This becomes the completeness checklist for later phases.

## Phase 2: Behavioral Contract Extraction

For each API element identified in Phase 1:

1. **Preconditions**: What must be true before the caller invokes this?
   Look for parameter validation, assertions, documented constraints,
   and implicit assumptions (e.g., "pointer must not be NULL" even if
   unchecked).
2. **Postconditions**: What is guaranteed after successful execution?
   What state changes occur? What values are returned?
3. **Error behavior**: What happens on invalid input, resource exhaustion,
   or concurrent access? Is the API fail-safe, fail-fast, or undefined?
4. **Side effects**: Does the function modify global state, allocate
   memory the caller must free, register callbacks, or interact with
   external systems?
5. **Ordering constraints**: Must certain functions be called before
   others? Is there an initialization/teardown protocol?
6. **Thread safety**: Can this be called concurrently? From any thread?
   What synchronization does the caller need to provide?

For each contract, cite the specific code evidence (file, line,
function) that establishes it.

## Phase 3: Essential vs. Incidental Classification

For every behavioral observation from Phase 2, classify it:

1. **Essential behavior**: Behavior that callers depend on and that
   defines the API's value. This becomes a requirement.
   - Test: "If this behavior changed, would existing correct callers break?"
   - Test: "Is this behavior documented, tested, or part of the type
     signature?"

2. **Incidental behavior**: Behavior that happens to be true in this
   implementation but is not part of the contract.
   - Test: "Could a correct reimplementation reasonably behave differently?"
   - Test: "Is this an optimization, ordering artifact, or implementation
     convenience?"

3. **Ambiguous behavior**: Cannot be classified without domain knowledge
   or explicit confirmation from stakeholders. Flag with `[AMBIGUOUS]`.

For ambiguous items, state the two interpretations and their implications
for requirements.

## Phase 4: Requirement Synthesis

Transform essential behaviors into structured requirements:

1. **Group by functional area**: Organize related behaviors into
   requirement categories (e.g., initialization, data processing,
   error handling, resource management).
2. **Write atomic requirements**: Each requirement captures exactly one
   testable behavior using RFC 2119 keywords (MUST, SHOULD, MAY).
3. **Derive acceptance criteria**: For each requirement, define at least
   one concrete, measurable test derived from the code's actual behavior.
   Prefer criteria that can be validated against the existing
   implementation as a reference oracle.
4. **Preserve semantic fidelity**: Requirements must faithfully represent
   what the implementation does, even if the behavior seems suboptimal.
   If behavior appears buggy but is established, note it as a requirement
   and flag: `[REVIEW: may be a defect in the reference implementation]`.
5. **Capture non-functional characteristics**: Performance bounds,
   resource usage patterns, concurrency guarantees, and platform
   requirements observed in the implementation.

## Phase 5: Completeness and Gap Analysis

1. **Coverage check**: Cross-reference the requirements against the
   API surface enumeration from Phase 1. Every public API element
   MUST have at least one associated requirement. Flag any gaps.
2. **Undocumented behavior**: Identify behaviors observed in the code
   that have no documentation, no tests, and no obvious purpose.
   These may be bugs, deprecated features, or undocumented contracts.
   Flag with `[UNDOCUMENTED]`.
3. **Missing error cases**: For each API element, verify that error
   conditions are covered by requirements. Missing error handling
   is a common gap.
4. **Cross-cutting concerns**: Verify that thread safety, resource
   lifecycle, and error propagation requirements are captured as
   cross-cutting requirements, not just per-function notes.

<!-- END PromptKit base -->

---

<!-- BEGIN ipcalc extensions -->

## ipcalc-Specific Extensions

ipcalc is a command-line program, so its "API" is its command line and
its output, not its C functions. Requirements MUST be stated in terms
of what a user invokes and observes; internal functions are cited only
as **Source** evidence.

### Phase 1 — API Surface Enumeration (ipcalc)

Enumerate in this order:

1. **Options**: every entry of the `getopt_long` table in `main()`
   (`ipcalc.c`), with its short letter, argument syntax, and the `app`
   mode (`enum app_t`) or `FLAG_*` bit it sets. Cross-check against
   `usage()` and `ipcalc.1.md`.
2. **Modes and option combinations**: which `app` modes exist, which
   options are valid in each, and which combinations `main()` rejects
   (these are covered by `--test-failure` tests in `tests/meson.build`).
3. **Accepted inputs**: address/prefix/netmask syntaxes for each family
   (`get_ipv4_info()`, `get_ipv6_info()`, `str_to_prefix()`), ranges for
   `-d`, the prefix or netmask for `-S`, and prefixes for `-r`.
4. **Outputs**: for each mode, the fields printed and their names in
   each output format — default/"modern" (`FLAG_SHOW_MODERN_INFO`),
   `NAME=value`, `--no-decorate`, and `--json` — and the conditions
   under which color is emitted.
5. **Exit status and diagnostics**: the exit code and stderr message for
   each class of error (bad input, invalid combination, allocation
   failure, lookup failure).
6. **Build configurations**: geo backends (`USE_MAXMIND`, `USE_GEOIP`,
   `USE_RUNTIME_LINKING`, none) and what output each one adds.

### Phase 2 — Behavioral Contract Extraction (ipcalc)

- **Thread safety** does not apply (single-threaded, short-lived
  process); replace it with **determinism**: is the output a pure
  function of the arguments, or does it depend on DNS, the geo
  database, randomness (`-r`), or the environment (`NO_COLOR`, TTY)?
  Requirements on non-deterministic output MUST state what is fixed
  (format, prefix length, address space) rather than the value.
- For every computed field, establish the boundary behavior at the
  edge prefixes and at the ends of the address space; these are
  essential behavior, not corner cases.

### Phase 3 — Essential vs. Incidental Classification (ipcalc)

- **Scripts depend on output.** Field names in `NAME=value` and JSON
  output, JSON value types (string vs. array), and the presence or
  absence of a field are **essential** — changing them breaks callers.
- Text and alignment of the default (decorated) output are
  **incidental** unless a test in `tests/` pins them; spacing changes
  there are not requirement changes.
- Behavior described in `ipcalc.1.md` is essential. If the code
  contradicts it, flag `[REVIEW: contradicts ipcalc.1.md — code or man
  page must change]`.
- Behavior pinned by an expected-output file in `tests/` is essential
  unless the file was clearly generated from buggy output; then flag
  `[REVIEW: test pins suspected defect]`.

### Phase 4 — Requirement Synthesis (ipcalc)

- Use the document map, ID scheme, and per-requirement format in
  `doc/requirements/README.md`.
- **Acceptance** criteria MUST name a concrete command line and the
  expected output or exit status, and cite the test in
  `tests/meson.build` that checks it (or state "no test").

### Phase 5 — Completeness and Gap Analysis (ipcalc)

Additional gap checks:

- Every option in the `getopt_long` table has a requirement, and is
  documented in both `usage()` and `ipcalc.1.md`.
- Every field printed in `NAME=value` mode has the same name in JSON
  mode, or the difference is a requirement.
- Every rejected option combination has a `--test-failure` test.
- Every mode has requirements for both IPv4 and IPv6 input, or a
  requirement that states the mode is single-family.

<!-- END ipcalc extensions -->
