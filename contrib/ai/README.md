# AI Assistance Framework

Persona, protocol, and taxonomy files for AI coding agents working on
ipcalc. See [`AGENTS.md`](../../AGENTS.md) in the repository root for the
project guide and the Requirements-First Workflow.

These are Markdown prompt files, not source code: do not run formatters
or build checks on them, and do not modify them as part of a code patch
unless the task targets them.

## Personas

| File | Intended for |
|------|-------------|
| `personas/ipcalc-dev.md` | Feature work, bug fixes, code review, requirements maintenance |

Load the persona as a system prompt prefix before starting work.

## Protocols and taxonomies

| File | Used for |
|------|----------|
| `protocols/requirements-from-implementation.md` | Documenting what existing code guarantees in `doc/requirements/` |
| `protocols/requirements-elicitation.md` | Turning a feature request or bug report into requirements |
| `protocols/code-compliance-audit.md` | Checking a patch against `doc/requirements/` |
| `taxonomies/specification-drift.md` | Classifying compliance findings (D1–D16) |
| `protocols/root-cause-analysis.md` | Bug investigation |
| `protocols/memory-safety-c.md` | Address arithmetic and buffer handling |
| `protocols/anti-hallucination.md` | Always |
| `protocols/adversarial-falsification.md` | Before reporting any defect |
| `protocols/self-verification.md` | Before declaring work done |

## Provenance

Every file is imported verbatim from
[PromptKit](https://github.com/microsoft/PromptKit) (MIT license; each
file keeps its SPDX header), followed by an ipcalc-specific section
between `<!-- BEGIN ipcalc extensions -->` and
`<!-- END ipcalc extensions -->`. To update a file from PromptKit,
replace everything above `<!-- END PromptKit base -->` with the new
upstream version and keep the extension section. Make ipcalc-specific
changes only inside the extension section.
