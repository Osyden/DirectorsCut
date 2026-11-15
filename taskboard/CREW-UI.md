# Crew names, portraits and speech hints

Branch: `codex/crew-ui`. Complete-content test checkout: `E:/Projects/DirectorsCut/Crew-UI`.

Integrated in main: feature `800bcc3`, combined source checkpoint `5aa954c`. This includes the separately verified helm-wheel pivot fix (`2ae43a8`).

Accepted playable build: `E:/Projects/DirectorsCut/Builds/2026-11-15_1139_crew-ui-helm/Windows/DirectorsCut.exe`. The existing `E:/Projects/DirectorsCut/Play Early Gameplay Packaged.cmd` now opens this build at 1600x900. Earlier 11:32/11:35 intermediate packages are superseded.

The visible crew are now **Rex**, **Duncan** and **Ruby**. Their existing cast, voices and gameplay roles are retained. Asset paths and configuration keys still use Finch/Marlow/Odessa so purchased content does not need renaming or reassembly.

The upper-right HUD shows a rendered portrait of each actual pirate, their name, a drawn wheel/cannon/fire/rigging icon and their live station state. The port cannon points left; starboard points right. It distinguishes moving, holding fire, repairing, extinguishing and standing by. The portraits are static textures, with no continuous scene-capture rendering cost. Capture support is in `DCPirateCastReview.cpp` (`-CrewPortraits`); `Tools/ACE/import_crew_portraits.py` imports the three captures. The release-content preflight requires their textures.

`STTVocabulary` in `Config/DefaultGame.ini` is sent to Whisper as its English context prompt. It includes the three names and **fire at will**, including named crew and battery variants. This is a recognition hint, not a strict vocabulary whitelist. Normal alternative phrasing still reaches the existing command parser/language-model path.

Conservative address correction accepts Recks/Wrecks, Dunkin/Dunken and Rubie when they begin an order and are followed by a supported command word. Ordinary prose such as “wrecks ahead” is untouched. Legacy crew names at the start of orders remain supported for existing test recordings/scripts.

Examples:

- `Rex, take the helm`
- `Duncan, fire at will`
- `Port battery, fire at will`
- `Starboard guns, fire at will`
- `Duncan, hold fire`
- `Ruby, put out the fire`
- `Ruby, repair the rigging`

## Verification

- UE 5.8 Editor build passed.
- `DirectorsCut.Commands.SupportedOrders`: 51 parser cases plus two address-normalization checks passed.
- `DirectorsCut.Commands.SpokenOrders`: all 20 synthesized speech round trips passed, using the actual configured vocabulary and local Whisper server. Includes all three names and four fire-at-will variants. Evidence: `Saved/CrewUI/Commands.log` and `Saved/ACE/SpokenOrders/transcripts.json`.
- Portrait import: all three `CREW_PORTRAIT_IMPORTED` markers present in `Saved/CrewUI/Import.log`.
- Runtime HUD reviewed at 1280x720 and 888x500. Wheel/cannon icons and HOLDING FIRE state were verified, followed by changed rigging/fire assignments with STANDING BY states. Evidence: `Saved/CrewUI/cannons-0014.png` and `Saved/CrewUI/duties-small-0014.png`.
- Combined Windows Development package succeeded. Its source/config matches main after newline normalization, and all three portrait assets match by hash. Main Editor rebuild also passed (`DirectorsCut/Saved/ACE/CrewUI-main-build.log`).
- The accepted package completed all three renamed crew lines on local GPU ACE, exited cleanly and reported `VOICE_SMOKE_SUCCESS mode=ACE cycles=1`. Evidence under its game folder: `Saved/ACE/Smoke/20261115_114240_ACE/runtime.log` and `voyage.png`. The fresh screenshot was inspected: ship, sea, sky, portraits, names and opposite cannon-icon directions are visible. This packaged capture was 888x500; the play shortcut/settings were restored to 1600x900 afterward.

Clean synthesized speech is not a live-microphone accuracy measurement. The owner's microphone, room noise and final dialogue mix still need hands-on review. The separate CUDA context and serialized ACE inference safeguards remain in place.
