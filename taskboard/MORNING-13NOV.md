# Morning hand-over, Fri 13 Nov (Claude)

What changed overnight, what to play, and what I could not check without a person at the controls. Detail and measurements are in [STATUS.md](STATUS.md).

## Play this build

`E:\projects\directorscut\Builds\Next\Windows\DirectorsCut.exe` (source `c000aea`, voice sidecars included). `Builds\Windows` is still the old 12 Nov build; I have not replaced it.

Measured in that package at 1280x720: about 115 fps on the open sea, 100 in the storm front, 90 in Atlantis. At 4K (editor build, DLSS 67%): 84 to 102.

## What you reported, and where it stands

| Your report | Now |
| --- | --- |
| Crew vibrating up and down | Fixed. It was the hull trembling (the ship asset's own physics), not the crew. |
| Stuttering | Half-second freeze entering the storm front removed. Other stutter not reproduced; if it is still there, run with `-DCHitch` and send me `Saved/Logs`. |
| Water not moving past the ship | The sea streams aft at 12 m/s (a small patch to the water plugin's shader in your engine folder; `Tools/pyx.sh Tools/patch_drw_current.py --undo` removes it). |
| No waves or foam round the ruins | A foam line laps at the stone where each ruin meets the sea. It is on the stone, not the water. |
| Ship needs bobbing | She heaves, pitches and rolls with the waves; crew, lanterns, fires and markers move with her. |
| Ship dragged sideways when turning | She gathers and carries her swing, yaws up to 6 degrees into the turn and heels. |
| Rain does not splash or wet anything | Splashes on the open decks; timber darkens and shines in rain and spray. |

## Also new

- Storm front: a wall of towering cloud on the horizon that closes on the ship, with a fog skirt, heavy rain and lightning inside.
- Sunlit ruins on the open sea in the first minute; fog banks among the ruins in Atlantis.
- White water along the hull and a wake astern.
- Sharper HUD text; a clear low sun for the ending; crew stand-ins painted dark instead of chrome.
- Codex's maelstrom (crater, Kaiju arms) is merged and in the build.

## Please check by hand

1. Walking the deck and taking the wheel and the guns while the ship moves. Unattended tests pass, but nobody has held the controls.
2. Sound, and the voice orders with a microphone.
3. Whether the ship's motion is too much or too little (`-DCShipMotion=1.0` is full, default 0.8, `-DCNoShipMotion` off). It costs roughly a tenth of the frame rate.

## Decisions for you

- Crew characters: they are still mannequins. A sailor or pirate character pack would fix the weakest thing on screen.
- Whether `Builds\Next` should replace `Builds\Windows`.
- Build size is 6.4 GB with the language model; see SUBMISSION.md for the options if itch.io's limit applies.
