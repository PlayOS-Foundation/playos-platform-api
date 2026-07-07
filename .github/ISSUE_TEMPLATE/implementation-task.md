---
name: Implementation Task
about: A scoped task an AI agent or contributor can implement.
title: "Task: "
labels: [implementation, agent-ready]
assignees: []
---

## Goal

<!-- One sentence: what should exist after this task is complete. -->

## Source of truth (read first)

- `AGENTS.md`
- `.github/copilot-instructions.md`
- Relevant `playos-spec` chapters / RFCs:
  <!-- e.g. RFC-0003 Capability Model; book/src/05-capability-model/* -->

## Required output

<!-- Files to create or modify, and the public API/behavior expected. -->

## Acceptance criteria

- [ ] Builds: `cmake -B build && cmake --build build`
- [ ] Matches the spec contract referenced above
- [ ] No game-engine dependency in the Platform API public headers
- [ ] Optional features exposed via capabilities, not OS checks

## Constraints

- Follow this repository's `AGENTS.md`.
- Keep the change scoped to this task; do not refactor unrelated code.
