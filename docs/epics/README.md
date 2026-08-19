# md-cubase-dongles: Epics, Stories & Tasks

This folder is the backlog for the Cubase 3 red-dongle microfirmware. It is
**committed** to the repo (visible in the tree), and the live dashboard
[`STATUS.md`](STATUS.md) is generated from the files below — don't edit it by
hand.

The full approved implementation plan (context, verification strategy, risks)
is mirrored here as the epic/story tree; the original narrative plan is kept by
the developer outside the repo.

## Hierarchy

```
docs/epics/
  cockpit.sh                     # regenerates STATUS.md from the files below
  STATUS.md                      # generated dashboard (epics grouped by iteration)
  ITERATIONS.md                  # iteration narrative (goal + outcome per iteration)
  DECISIONS.md                   # D-NN decisions + C-NN constraints
  templates/                     # copy these when adding new work
    epic.md
    story.md
  EPIC-01-<slug>/
    epic.md                      # the epic (carries `iteration: N`)
    STORY-01-<slug>.md           # a story (with its tasks inside)
    STORY-02-<slug>.md
  EPIC-02-<slug>/
    ...
```

- **Iteration**: a pass with one overarching goal, grouping several epics. Each
  `epic.md` carries an `iteration: N` field; the cockpit groups epics under
  their iteration and [`ITERATIONS.md`](ITERATIONS.md) holds the narrative.
- **Epic**: a folder `EPIC-NN-<slug>/` with an `epic.md`. A coarse capability.
- **Story**: a file `STORY-NN-<slug>.md` inside an epic folder. A shippable
  slice.
- **Task**: a GitHub-style checkbox line inside a story, `- [ ]` (open) or
  `- [x]` (done). The checkboxes drive the percentages in the cockpit.

`NN` is zero-padded and unique within its parent (epics are globally numbered;
stories restart at 01 inside each epic). Epic numbers are identity, not
execution order.

## Status field

Every `epic.md` and story carries YAML frontmatter. The cockpit reads `id`,
`title`, `status`; percentages come from the task checkboxes.

```yaml
---
id: STORY-01
epic: EPIC-01
title: Hash and register the reverse-engineered sources
status: done   # todo | in-progress | done | blocked | deferred
---
```

Keep `status` honest relative to the checkboxes: `todo` = no tasks done,
`done` = all done, `in-progress` = some, `blocked` = waiting on something
(note why), `deferred` = consciously postponed to a later iteration.

## Decisions & constraints

Cross-cutting decisions and hardware constraints live in
[`DECISIONS.md`](DECISIONS.md) (e.g. v1 scope, ROM3 ownership, the runtime
engine, the pin15 reset). Stories reference them as `D-NN` / `C-NN` instead of
re-arguing the point.

## Working rules

- **Strictly sequential**: one epic at a time; the ordering follows the
  iteration narrative. Iteration 1 (host-only, no hardware) comes first because
  it removes all correctness risk before any firmware is written.
- **User verification checkpoints**: after each epic, work STOPS. Diego builds,
  flashes, and exercises on the MultiDevice + ST (and against real Cubase) and
  approves before the next epic starts.
- **Gate #1**: EPIC-04 (fixed-word ROM3 drive) runs before the state-machine
  engine, because whether ROM3 can be *driven* at all (C-01, D-07) is the one
  critical unknown that needs hardware.

## Adding work

1. New epic: copy `templates/epic.md` into `EPIC-NN-<slug>/epic.md`.
2. New story: copy `templates/story.md` into the epic folder as
   `STORY-NN-<slug>.md`, list its tasks as checkboxes.
3. Regenerate the cockpit:

   ```bash
   ./docs/epics/cockpit.sh
   ```

## Cockpit

`cockpit.sh` reads every `EPIC-*/epic.md` and `EPIC-*/STORY-*.md`, counts the
task checkboxes, then writes `STATUS.md` with per-epic and overall progress
bars. No dependencies beyond `bash`/`grep`/`sed`. Run
`./docs/epics/cockpit.sh --stdout` to preview.
