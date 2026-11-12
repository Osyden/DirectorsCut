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

The voyage is a ten-minute ride. Open sea at sunset. A fog bank full of drowned ruins, colossal statues and spectral ships. The sea rolling up into a tunnel and dropping away. A channel of fire. And at the end a maelstrom with something very large in the middle of it.

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
- About 6 GB of disk space and 6 GB of free video memory (the language model shares the GPU with the game).
- The first launch takes up to a minute before the crew's "mind" is ready; until then they use their stock lines.

## Made with

Unreal Engine 5.8, NVIDIA DLSS 4.5. Crew AI: whisper.cpp, llama.cpp (Qwen3.5-4B), Piper. Ship: Brigantine asset from Fab. Full list of sourced assets in the submission form.

## AI disclosure

The game's code, materials and tuning were written by AI coding agents (Claude Code, with Codex) under the direction of a human designer, during the jam. The crew in the game are driven by local AI models at runtime.
