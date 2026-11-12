# DirectorsCut taskboard

| Task | Status | Owner | Done when |
| --- | --- | --- | --- |
| Initialize Git and Git LFS | Done | Codex | Baseline is on `origin/main` at `dcf3e85`. |
| Diagnose Unreal MCP startup | Done | Claude | Launch with `-ModelContextProtocolStartServer`; see STATUS.md. |
| Define the first playable slice | Done | Claude | See [PLAN.md](PLAN.md). |
| M1 greybox (see plan) | Done | Claude | C++ module builds, ride director runs all phases, typed order moves a crew member, packaged build runs. |
| M2 voice loop | In progress | Claude | Voice loop works in an unattended test; live mic untested; player-operated cannons and helm still to do. |
| DLSS 4.5 plugin | In progress | Claude | Enabled and active in the editor build; still to confirm in a packaged build. |

Claim a task here with your name and branch before editing. Use a separate Git worktree for parallel changes. Coordinate before editing the same Unreal map, Blueprint, or live editor session.

For a handoff, add a short Markdown note in this folder with the task, branch or commit, what changed, how it was verified, and the next step or blocker. Link the note from the task row and share the commit with the other agent. This board is in a public repo, so keep credentials and private logs out of it.

## Ask Codex from Claude Code

Open Claude Code at the repository root. In an already running session, use `/reload-plugins` and then `/codex:setup`. Use `/codex:review` for a read-only review or `/codex:rescue` to delegate a specific task. Give Codex the task and the files or branch to inspect. For edits, agree on file ownership first.

From a shell, a read-only question can also use:

```sh
codex -a never exec --sandbox read-only --ephemeral "Read taskboard/README.md and summarize the open tasks."
```

Plugin requests start or resume Codex jobs. Use the taskboard and commit references to pass context to a separate desktop chat.

## Unattended run (Thu 12 Nov)

Claude is building autonomously. Read [PLAN.md](PLAN.md) for the design, [STATUS.md](STATUS.md) for progress. To reach Claude, append to [INBOX.md](INBOX.md). Claude owns the live Unreal editor, `Source/` and `Content/DC/` until further notice.
