<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) PromptKit Contributors -->

---
name: anti-hallucination
type: guardrail
description: >
  Cross-cutting protocol that constrains LLM behavior to prevent fabrication,
  enforce epistemic honesty, and ensure outputs are grounded in provided context.
applicable_to: all
---

# Protocol: Anti-Hallucination Guardrails

This protocol MUST be applied to all tasks that produce artifacts consumed by
humans or downstream LLM passes. It defines epistemic constraints that prevent
fabrication and enforce intellectual honesty.

## Rules

### 1. Epistemic Labeling

Every claim in your output MUST be categorized as one of:

- **KNOWN**: Directly stated in or derivable from the provided context.
- **INFERRED**: A conclusion derived through a stated chain of logical steps
  from the context, with the reasoning chain made explicit.
- **ASSUMED**: Not established by context. The assumption MUST be flagged
  with `[ASSUMPTION]` and a justification for why it is reasonable.

**Data-driven tasks**: When the source data is authoritative machine
telemetry or tool output (e.g., profiler results, trace queries, compiler
diagnostics, monitoring metrics), direct observations and measurements
reported by the tool have implicit KNOWN status and do not require explicit
`[KNOWN]` labels. However, **causal explanations**, **inferred
correlations**, and **interpretations** of that data retain full labeling
requirements — these are INFERRED or ASSUMED claims even when derived
from authoritative measurements.

When the number of claims categorized as ASSUMED exceeds 30% of the total
number of categorized claims in your output, stop and request
additional context instead of proceeding.

### 2. Refusal to Fabricate

- Do NOT invent function names, API signatures, configuration values, file paths,
  version numbers, or behavioral details that are not present in the provided context.
- If a detail is needed but not provided, write `[UNKNOWN: <what is missing>]`
  as a placeholder.
- Do NOT generate plausible-sounding but unverified facts (e.g., "this function
  was introduced in version 3.2" without evidence).

### 3. Uncertainty Disclosure

- When multiple interpretations of a requirement or behavior are possible,
  enumerate them explicitly rather than choosing one silently.
- When a conclusion depends on 2 or more ASSUMED premises (per Rule 1), flag it
  explicitly: "Low confidence — this conclusion depends on [N] assumptions:
  [list each]. Verify by [specific action]."

### 4. Source Attribution

- When referencing information from the provided context, indicate where it
  came from (e.g., "per the requirements doc, section 3.2" or "based on line
  42 of `auth.c`").
- Do NOT cite sources that were not provided to you.

### 5. Scope Boundaries

- If a question falls outside the provided context, say so explicitly:
  "This question cannot be answered from the provided context. The following
  additional information is needed: [list]."
- Do NOT extrapolate beyond the provided scope to fill gaps.

<!-- END PromptKit base -->

---

<!-- BEGIN ipcalc extensions -->

## ipcalc-Specific Extensions

ipcalc is a small C codebase, but its value is numeric correctness and
stable output. A hallucinated address range, prefix, or output field is a
user-visible bug.

### Rule 1 (Epistemic Labeling) — ipcalc application

- Do not invent `FLAG_*` constants, `enum app_t` values, `ip_info_st`
  fields, or printf helpers. Read `ipcalc.h` before citing any of them.
- Do not invent command-line options or their short letters. The
  authoritative list is the `getopt_long` table in `main()` (`ipcalc.c`)
  and `ipcalc.1.md`; read both before citing an option.
- Do not state the address type/space of a range (private, reserved,
  documentation, multicast, …) from memory. Read the classification
  code (`ipv4_net_to_type()` / `ipv6_net_to_type()` in `ipcalc.c`); if
  citing the IANA registry itself, mark it `[UNKNOWN: verify against the
  IANA special-purpose address registry]` unless you have read it.
- Do not claim what ipcalc prints for an input without running it
  (`./build/ipcalc ...`) or reading the matching expected-output file
  in `tests/`. Output format details (field names, JSON keys,
  ordering) MUST come from evidence, not from expectation.
- Do not invent libmaxminddb API signatures; read `ipcalc-maxmind.c`
  for how the project calls them.

### Rule 2 (Refusal to Fabricate) — ipcalc application

- Do not assert that `ipcalc.1.md` documents a behavior or option
  without reading the relevant section — drift between the man page,
  `usage()`, and the code is itself a finding.
- Do not assert arithmetic results for edge prefixes (/0, /31, /32,
  /127, /128) or edge addresses (`0.0.0.0`, `255.255.255.255`, `::`,
  `ffff:…`) by reasoning alone; these are where ipcalc has had bugs.
  Run the binary.

<!-- END ipcalc extensions -->
