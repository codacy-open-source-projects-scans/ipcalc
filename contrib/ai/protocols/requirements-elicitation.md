<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) PromptKit Contributors -->

---
name: requirements-elicitation
type: reasoning
description: >
  Protocol for extracting, structuring, and validating requirements
  from natural language descriptions. Produces precise, testable,
  unambiguous requirements with stable identifiers.
applicable_to:
  - author-requirements-doc
  - interactive-design
  - hardware-design-workflow
---

# Protocol: Requirements Elicitation

Apply this protocol when converting a natural language description of a feature,
system, or project into structured requirements. The goal is to produce
requirements that are **precise, testable, unambiguous, and traceable**.

## Phase 1: Scope Extraction

From the provided description:

1. Identify the **core objective**: what problem does this solve? For whom?
2. Identify **explicit constraints**: performance targets, compatibility
   requirements, regulatory requirements, deadlines.
3. Identify **implicit constraints**: assumptions about the environment,
   platform, or existing system that are not stated but required.
   Flag each with `[IMPLICIT]`.
4. Define **what is in scope** and **what is out of scope**. When the
   boundary is unclear, enumerate the ambiguity and ask for clarification.

## Phase 2: Requirement Decomposition

For each capability described:

1. Break it into **atomic requirements** — each requirement describes
   exactly one testable behavior or constraint.
2. Use **RFC 2119 keywords** precisely:
   - MUST / MUST NOT — absolute requirement or prohibition
   - SHALL / SHALL NOT — equivalent to MUST (used in some standards)
   - SHOULD / SHOULD NOT — recommended but not absolute
   - MAY — truly optional
3. Assign a **stable identifier**: `REQ-<CATEGORY>-<NNN>`
   - Category is a short domain tag (e.g., AUTH, PERF, DATA, UI)
   - Number is sequential within the category
4. Write each requirement in the form:
   ```
   REQ-<CAT>-<NNN>: The system MUST/SHALL/SHOULD/MAY <behavior>
   when <condition> so that <rationale>.
   ```

## Phase 3: Ambiguity Detection

Review each requirement for language that introduces non-deterministic
interpretation. Apply the ambiguity pattern categories below
systematically; these categories are aligned with the
`prompt-determinism-analysis` protocol:

1. **Vague adjectives**: "fast," "responsive," "secure," "scalable,"
   "user-friendly" — replace with measurable criteria.
2. **Unquantified quantities**: "handle many users," "large files" —
   replace with specific numbers or ranges.
3. **Implicit behavior**: "the system handles errors" — what errors?
   What does "handle" mean? Retry? Log? Alert? Fail open? Fail closed?
4. **Undefined terms**: if a term could mean different things to different
   readers, add it to a glossary with a precise definition.
5. **Missing negative requirements**: for every "the system MUST do X,"
   consider "the system MUST NOT do Y" (e.g., "MUST NOT expose PII in logs").
6. **Open-ended enumerations**: "support formats like PDF, Word, etc." —
   enumerate the complete list or define the selection criterion
   (e.g., "all formats supported by the pandoc library").
7. **Hedge words in requirements**: "the system could support" or
   "consider adding" — these must be resolved to MUST, SHOULD, or MAY.
   If the user cannot decide, classify as MAY and flag for review.
8. **Missing conditional branches**: "if the user is authenticated,
   show the dashboard" — what happens if NOT authenticated? Add
   explicit else-branches for every conditional requirement.
9. **Unanchored comparatives**: "faster than the current system" —
   anchor to measurable baselines (e.g., "response time under 200ms
   at the 95th percentile, compared to the current 500ms").

## Phase 4: Dependency and Conflict Analysis

1. Identify **dependencies** between requirements: which requirements
   must be satisfied before others can be implemented or tested?
2. Check for **conflicts**: requirements that contradict each other
   or create impossible constraints.
3. Check for **completeness**: are there scenarios or edge cases
   that no requirement covers? If so, draft candidate requirements
   and flag them as `[CANDIDATE]` for review.

## Phase 5: Acceptance Criteria

For each requirement:

1. Define at least one **acceptance criterion** — a concrete test that
   determines whether the requirement is met.
2. Acceptance criteria should be:
   - **Specific**: describes exact inputs, actions, and expected outputs.
   - **Measurable**: pass/fail is objective, not subjective.
   - **Independent**: testable without requiring other requirements to be met
     (where possible).

<!-- END PromptKit base -->

---

<!-- BEGIN ipcalc extensions -->

## ipcalc-Specific Extensions

Apply these alongside the base phases when turning a feature request or
bug report into requirements in `doc/requirements/`.

### Phase 1 — Scope Extraction (ipcalc)

- **Name the mode.** State which `app` mode the requirement belongs to
  (info, check, split, deaggregate, random generation, version), and
  whether it adds a new option, changes an existing option's behavior,
  or changes output.
- **Name the families.** State whether the requirement applies to IPv4,
  IPv6, or both. If only one, add an explicit requirement for what
  happens with the other family (reject with an error, or ignore).
- **Name the output formats.** A new or changed field MUST be specified
  for the default, `NAME=value`, and `--json` outputs.
- **Flag implicit constraints**:
  - `[IMPLICIT: geo backend]` if it depends on libmaxminddb/libGeoIP;
    the requirement MUST state the behavior when ipcalc is built
    without a geo backend.
  - `[IMPLICIT: network]` if it depends on DNS resolution.
  - `[IMPLICIT: compatibility]` if it changes an existing field name,
    JSON type, or exit status — scripts parse this output.

### Phase 2 — Requirement Decomposition (ipcalc)

Use the category tags defined in `doc/requirements/README.md`
(`INPUT`, `CALC`, `OUTPUT`, `ERR`, `COMPAT`, `BUILD`, `TEST`).

**Negative requirements are mandatory for `INPUT` and `ERR`.** For each
accepted input form, write what MUST be rejected (malformed address,
prefix out of range for the family, conflicting options) and the
resulting exit status.

### Phase 3 — Ambiguity Detection (ipcalc)

Check each requirement against the Glossary in
`doc/requirements/README.md`. Terms that are ambiguous in ipcalc:

- **"network" / "address"**: the input as given, or the network
  address computed from it?
- **"hosts"**: usable host addresses (excluding network/broadcast where
  applicable) or all addresses in the prefix? State the /31, /32 and
  IPv6 behavior explicitly.
- **"invalid"**: name the exact input class and the exit status.

### Phase 5 — Acceptance Criteria (ipcalc)

Every acceptance criterion MUST be expressible as a test in
`tests/meson.build`: a command line plus either an expected-output file
(`--test-outfile`), an exit status (`--test-success`/`--test-failure`),
or an output comparison (`--test-output`/`--test-equal`). Include at
least one positive and one negative criterion per requirement where a
negative case exists.

<!-- END ipcalc extensions -->
