<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) PromptKit Contributors -->

---
name: memory-safety-c
type: analysis
description: >
  Systematic protocol for analyzing memory safety issues in C codebases.
  Covers allocation/deallocation pairing, pointer lifecycle, buffer boundaries,
  and undefined behavior.
language: C
applicable_to:
  - investigate-bug
  - review-code
  - investigate-security
---

# Protocol: Memory Safety Analysis (C)

Apply this protocol when analyzing C code for memory safety defects. Execute
each phase in order. Do not skip phases — apparent simplicity often hides
subtle bugs.

## Phase 1: Allocation / Deallocation Pairing

For every allocation site (`malloc`, `calloc`, `realloc`, `strdup`, custom allocators):

1. Trace **all** code paths from allocation to deallocation.
2. Identify paths where deallocation is **missing** (leak) or **unreachable**
   (early return, exception-like longjmp, error branch).
3. Check for **double free**: paths where the same pointer is freed more than once.
4. Check for **mismatched APIs**: `malloc`/`free` vs `new`/`delete` vs custom
   allocator pairs.

## Phase 2: Pointer Lifecycle Analysis

For every pointer variable:

1. Determine its **ownership semantics**: who is responsible for freeing it?
   Is ownership transferred? Is it documented?
2. Check for **use-after-free**: any access to a pointer after its referent
   has been freed. Pay special attention to:
   - Pointers stored in structs or global state that outlive the allocation.
   - Pointers passed to callbacks or stored in event loops.
   - Conditional free followed by unconditional use.
3. Check for **dangling pointers**: pointers to stack variables that escape
   their scope (returned from function, stored in heap struct).
4. Verify **NULL checks** after allocation and after any operation that may
   invalidate a pointer (e.g., `realloc`).

## Phase 3: Buffer Boundary Analysis

For every buffer (stack arrays, heap allocations, string buffers):

1. Identify all **read and write accesses** to the buffer.
2. Verify that every access is **bounds-checked** or provably within bounds.
3. Check for **off-by-one errors** in loop conditions and index calculations.
4. Check `strncpy`, `snprintf`, `memcpy` calls for correct size arguments.
5. Identify any **user-controlled index or size** values that flow into
   buffer accesses without validation.

## Phase 4: Undefined Behavior Audit

Check for common sources of undefined behavior:

1. **Signed integer overflow** in size calculations.
2. **Null pointer dereference** on error paths.
3. **Uninitialized memory reads** — especially stack variables and struct
   fields after partial initialization.
4. **Type punning** violations (strict aliasing).
5. **Sequence point violations** in complex expressions.

## Output Format

For each finding, report:

```
[SEVERITY: Critical|High|Medium|Low]
Location: <file>:<line> or <function name>
Issue: <concise description>
Evidence: <code path or snippet demonstrating the issue>
Remediation: <specific fix recommendation>
Confidence: <High|Medium|Low — with justification if not High>
```

<!-- END PromptKit base -->

---

<!-- BEGIN ipcalc extensions -->

## ipcalc-Specific Extensions

ipcalc is a short-lived CLI: freeing memory before exit is not
required, and leaks on paths that end in `exit()` are not findings.
Out-of-bounds access, use of uninitialized data, and undefined
behavior are findings — they produce wrong output or crashes on user
input, and CI runs an ASan build.

### Phase 1 — Allocation / Deallocation Pairing (ipcalc)

- Allocations go through `safe_asprintf()` / `safe_strdup()`
  (`ipcalc-utils.c`), which exit on failure. New code SHOULD use them
  instead of bare `malloc`/`strdup`.
- Strings stored in `ip_info_st` are owned by that struct for the life
  of the process.

### Phase 3 — Buffer Boundary Analysis (ipcalc)

- Fixed-size buffers are used for address strings
  (`INET_ADDRSTRLEN`/`INET6_ADDRSTRLEN`), host counts, and reverse DNS
  names (`ipcalc-reverse.c`). Check every `snprintf`/`strcpy`/`strcat`
  into them against the worst-case input (full-length IPv6, /0 prefix).

### Phase 4 — Undefined Behavior Audit (ipcalc)

The recurring bug class in this codebase is shift and width UB in
prefix/mask arithmetic. Check specifically:

- Shifts by 32 (IPv4) or by ≥ 64 / 128 (IPv6 byte arithmetic in `ipv6.c` and `netsplit.c`) — e.g.
  computing a mask for prefix 0 or a host count for /0.
- Signed overflow when counting hosts or subnets for large networks.
- Conversions between `unsigned`, `int`, and 64-bit types when
  printing counts.

Exercise the edges: prefixes 0, 1, 31, 32 (IPv4) and 0, 1, 63, 64, 65,
127, 128 (IPv6), and addresses at both ends of the space.

<!-- END ipcalc extensions -->
