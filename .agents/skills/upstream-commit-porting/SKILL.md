---
name: upstream-commit-porting
description: Analyze commits a Godot/Gobot fork is behind upstream and selectively reimplement approved commit intents in the fork's own architecture.
---

# Upstream Commit Porting

Use this skill for recurring rounds of reviewing `godotengine/godot:master` history and implementing selected changes in the Gobot fork.

## Guardrails

- Treat upstream as an information source. Reimplement approved behavior using the fork's current names, structure, and conventions; do not copy commits, merge upstream source changes, or cherry-pick.
- Never apply upstream file changes without the user's explicit approval. A GitHub "behind" count measures ancestry, not remaining implementation work.
- A bookkeeping merge with `git merge -s ours` is a separate, optional history operation. Do it only when explicitly authorized. It must leave the fork tree identical, use `-s ours` (not `-X ours`), and be pushed only when authorized. Before pushing, inspect and disclose any local commits that will also be published.
- Read repository `AGENTS.md` and relevant local skills before editing. Follow their naming, scope, commit, and verification rules. Do not compile or run tests unless requested.
- Check the working tree before starting. Preserve unrelated changes and untracked files; never stage broadly.

## Review the upstream range

1. Identify the fork branch, its current upstream baseline, and the latest `upstream/master`. Fetch or otherwise verify the remote tip before claiming the range is current. If network or Git metadata permissions prevent that, say which ref is stale and do not present it as current.
2. Compare commit ancestry to find upstream-only commits. Exclude merge commits from the per-commit list: they are PR/history wrappers, not separate intents. Do not omit non-merge commits just because a merge contains them.
3. Inspect each commit's message and patch, then inspect corresponding fork paths and dependencies. Account for renamed, moved, heavily modified, and deleted code before deciding whether an intent applies.
4. Always classify commits limited to purged features as `SKIP-N/A`, including C#/.NET, Web, project migration, extensions/GDExtension, and the asset store. Do not restore removed features to apply an upstream change.

## Report entries

For every non-merge commit, include:

- **Intent:** one or two sentences describing what it fixes/adds and why, without a line-by-line diff summary.
- **State in my fork:** `unchanged`, `renamed (old -> new)`, `moved`, `heavily modified`, or `removed`, with paths/evidence as useful.
- **Classification:** exactly one of:
  - `PORT` — state where and how to adapt it.
  - `SKIP-N/A` — feature/code is gone or the failure cannot occur; explain why.
  - `SKIP-OPTIONAL` — cosmetic, docs, CI, optional vendor, unbuilt platform, or a choice the user may not want; explain.
  - `NEEDS-DECISION` — ask one concrete policy question.
- **Dependencies:** identify prerequisite commits or upstream APIs, and whether the fork has them.

Keep reports under `.sync/` when requested. When asked to remove completed entries, verify them against the actual fork history and remove only entries confirmed implemented; preserve all remaining dispositions and keep counts accurate.

## Implement an approved round

1. Implement only the classifications and exceptions the user selected for that round. Do not infer that all optional entries are approved unless the user says so.
2. Handle one upstream non-merge commit per local implementation commit, using the subject `Upstream <sha>: <intent summary>`. Recreate the intent in the fork's architecture; translate old identifiers and paths to current fork names. Prefer a simpler implementation when it preserves the behavior, and explain meaningful deviations.
3. Update all directly affected callers, docs/XML, bindings, compatibility code, and tests where applicable. Remove obsolete or dead code made unnecessary by the port. If a prerequisite was skipped, make the change stand alone when sound; otherwise mark it blocked and explain the dependency.
4. Inspect the final diff and status for unintended tracked or staged changes. Follow repository verification instructions; do not claim build or runtime success without running the requested verification.
5. Update the report when asked, removing completed entries only after confirming their local implementation commits. Do not change its upstream baseline until that tip has actually been reviewed.

## Clearing GitHub's behind count

If the user explicitly requests a bookkeeping checkpoint, verify the current upstream tip and explain that this changes ancestry but not the source tree. Record the fork tree hash before the `ours` merge, create the merge against that exact upstream tip, and verify the resulting tree hash matches the pre-merge hash and that the upstream tip is the second parent. Push only with authorization; check how many fork commits are not on `origin/master` and disclose them before publishing. Afterward, verify the fork branch is zero commits behind the checkpointed upstream ref. Future rounds start from that reviewed tip; new upstream commits will accumulate behind it.
