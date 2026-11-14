# Draft itch.io page text

Draft for the owner to edit. Everything here describes what is in the build as of Thu 12 Nov.

## Title

Director's Cut

## One line

Captain a brigantine through a haunted fog, a burning sea and a maelstrom, by giving spoken orders to a crew that listens, answers and acts.

## Description

The feature we always wanted and never could ship: a crew you direct with your own voice.

You are the captain. The ship has a helm, two gun batteries, fire buckets and rigging, and you have three crew: Finch at the wheel, Marlow on the guns, Odessa the bosun. You cannot do it all yourself. Hold a key and say what you want:

- "Finch, hard to starboard!"
- "Port battery, fire!"
- "Odessa, put out that fire!"
- "Somebody get us away from those rocks!"

They understand plain speech, answer in their own words and voices, and get on with it. They also call out what they see: rocks off the bow, a ghost ship coming alongside, fire on deck, a torn sail.

The voyage is an eight-minute ride. Open sea at sunset. A storm front that swallows the ship. Drowned Atlantis in the fog: colossal statues, broken aqueducts, spectral ships, faces in the clouds. A portal of light, and the sea rolling up into a tunnel and dropping away. And at the end a maelstrom with something very large in the middle of it.

Everything the crew does runs on your own PC: speech recognition, a small language model and speech synthesis are bundled with the game. Nothing is sent anywhere.

## Controls

| | |
|---|---|
| Hold `V` (or middle mouse) | Speak an order |
| `Enter` | Type an order |
| `1` to `6` | Quick orders: hard to port, hard to starboard, steady, port battery fire, starboard battery fire, hold fire |
| `W` `A` `S` `D`, mouse | Walk the deck, look |
| `E` | Take the helm or a gun battery yourself; hold at a fire or the mainmast to work on it |
| At the helm: `A` `D` | Steer |
| At the guns: mouse, click | Aim and fire |
| `F5` | DLSS mode: Quality, Balanced, Performance, DLAA |
| `F6` | DLSS Frame Generation on/off |
| `R` (at the end) | Sail again |
| `Esc` | Quit |

No microphone? Typed orders and the number keys do everything voice does.

## Requirements

- Windows 10 or 11, 64-bit.
- A ray-tracing capable GPU. Built and tested on an NVIDIA RTX card with DLSS 4.5.
- About 6 GB of disk space. Video memory: the game and the language model together used about 4 GB at 1080p on the development PC; 8 GB cards should be comfortable.
- The first launch takes up to a minute before the crew's "mind" is ready; until then they use their stock lines.

## Made with

Unreal Engine 5.8, NVIDIA DLSS 4.5. Crew AI: whisper.cpp, llama.cpp (Qwen3.5-4B), Piper. Ship: Brigantine asset from Fab. Full list of sourced assets in the submission form.

## AI disclosure

The game's code, materials and tuning were written by AI coding agents (Claude Code, with Codex) under the direction of a human designer, during the jam. The crew in the game are driven by local AI models at runtime.

## Suggested changes for the text above, 14 Nov (Claude; for the owner to take or leave)

The page above describes Thursday's build. Against the build of 14 Nov, `2026-11-14_0235_0a152ce_guns-reach-down-slope`:

- **The voyage paragraph.** There is no burning sea, no ghost ships (they are off) and no faces in the clouds (parked). What is there now, in order: open sea at sunset; a wall of storm cloud that grows on the horizon with lightning inside it and a tunnel through its foot; the tunnel, lit from within by lightning; drowned Atlantis in layered fog, with statues, temples and a sea that rises like a half-pipe either side of the channel; the sea rolling up into a tunnel of water and dropping away; and the maelstrom, where the creature's arms reach for the ship, waterspouts and burning stones come at her, its young swim in to board, and it is beaten when the clock runs out. A possible wording:

  > The voyage is an eight-minute ride. Open sea at sunset. A wall of storm that grows out of nothing, with lightning in its belly and one way through. Drowned Atlantis in the fog: colossal statues, broken temples, a sea that climbs away on either hand. The water rolling up into a tunnel and dropping from under the keel. And at the end a maelstrom two miles wide, with something very large in the middle of it that throws waterspouts, fire and its own young at you until the storm takes it back.

- **Quick orders.** Worth saying that in the maelstrom "hard to port" and "hard to starboard" are how you dodge: one order to the helmsman clears a waterspout.
- **Disk space.** The build is now 7.7 GB, not about 6.
- **Made with.** Add: Ultra Dynamic Sky, Dynamic Real Water, Clouds Tunnel Vdb Effects Pack Loop (one.studio2), Easy Fog, Ancient Ruins, Claw Kaiju, Sonniss GDC audio bundle. Check each licence's wording for store pages before publishing; I have not.
- **Pictures.** Thirteen clean 4K frames without the HUD are in `E:\projects\directorscut\StoreShots\2026-11-14` (outside the repository); a contact sheet is in `taskboard/review/56-store-shots`.
