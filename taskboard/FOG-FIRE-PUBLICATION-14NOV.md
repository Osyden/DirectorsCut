# Fog and fire publication — 14 November

Publication branch: `codex/fog-fire-polish`, based on `cf49b3b`.

This branch collects the weather chat's accepted storm-front, early lighting, city fog, exit concealment and deck-fire changes. It includes five original material assets, their three generators, and all 60 curated gameplay review screenshots from `storm-sculpt`, `city-fog-depth`, `city-fog-cohesion` and `deck-fire`.

The branch was assembled in an isolated worktree. Only the fog visibility changes from the shared DCTreadmill header are included; the cannon chat's hostile-query helpers are excluded. Cannon, crew, player, game-mode, maelstrom, ACE and submission changes from other chats are not included. The shared main worktree, its staged changes and its branch remain in place.

The isolated publication source successfully builds DirectorsCutEditor with Unreal 5.8 without the excluded cannon changes. Prior runtime verification includes continuous normal-speed storm/city captures, fog restoration across restart/seeks, normal-speed natural deck fires, close-up fire review and extinguish cleanup. See [weather](CODEX-EARLY-WEATHER-14NOV.md), [storm iterations](STORM-SCULPT-ITERATIONS.md), [city fog](CITY-FOG-DEPTH-14NOV.md) and [fire](DECK-FIRE-14NOV.md) for evidence and limitations. Historical mentions of uncommitted work in those notes describe their earlier working state; this branch is the publication snapshot.

The five new `.uasset` files use the repository's existing Git LFS rules. Screenshots are ordinary Git files. Purchased textures, packs, plugin binaries, builds, local logs and rejected experimental outputs are excluded. Fire materials reference already installed Paragon/Brigantine textures; those source packs are not redistributed here. No standalone game package was rebuilt for this publication.

The remaining strong forward city-fog glare and the reference artwork's richer turbulence are still documented open visual issues. Publication does not change those findings or claim AAA parity.
