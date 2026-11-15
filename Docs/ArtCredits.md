# Credits and art attribution

Verified against current main source, installed assets, project integration records and official creator/Fab pages on 15 November 2026. The short credits below are ready for the game UI. Product names are attribution labels; their assets remain the work of the named creators.

## Requested team credits

- **Director:** Eva Jobse
- **Developers:** GPT-6 Astra, Claude Fable 5.2, Qwen 3.8 27B
- **Special thanks:** Epic Games, NVIDIA

The developer names above preserve the owner's requested wording.

## Art — UI-ready list

| Asset or pack | Creator |
| --- | --- |
| Brigantine Ship | MainStore |
| Dynamic Real Water | Natural Level Game Studio |
| Ultra Dynamic Sky / Ultra Dynamic Weather | Everett Gunther |
| Modular Ancient Ruins | archafizov |
| Kraken | PROTOFACTOR INC |
| Claw Kaiju Monster (Daigani) | HuyKhoi2407 |
| MHC: Pirates — MetaHuman Mega Bundle | Quang Phan |
| Easy Fog | William Faucher |
| Clouds Tunnel Vdb Effects Pack Loop | one.studio2 |
| Huge Nordic Coastal Cliff / Massive Nordic Coastal Cliff | Quixel Megascans |
| Paragon: Rampage, MetaHuman and Unreal mannequin assets | Epic Games |

## Verification and sources

| Credit | Official source and evidence of use |
| --- | --- |
| MainStore | [Brigantine Ship on Fab](https://www.fab.com/listings/4a58a293-8596-4466-9c36-a65fb1d63a79). `DCGame.cpp` loads `/Game/Brigantine/BP/BP_Brigantine`; `DCGunnery.cpp` uses its firing/impact effects; `DCAudio.cpp` uses its bell. Includes ship, guns, rigging and associated pack effects. |
| Natural Level Game Studio | [Creator documentation](https://naturallevel.dev/assets/dynamic-real-water/introduction/). The installed `DynamicRealWater.uplugin` explicitly names `CreatedBy: Natural Level Game Studio`, version 3.0; `DirectorsCut.uproject` enables it. `DCOcean` and the ride/water integration use its ocean simulation. Installed metadata: `E:/Utilities/UE_5.8/Engine/Plugins/Marketplace/DynamicR1d9938e98fdbV5/DynamicRealWater.uplugin`. |
| Everett Gunther | [Ultra Dynamic Sky on Fab](https://www.fab.com/listings/84fda27a-c79f-49c9-8458-82401fb37cfb). `DCRideDirector.cpp` loads the installed `/Game/UltraDynamicSky` sky/weather assets; the caldera sky configures the same system. Ultra Dynamic Weather belongs to this pack, not a separate unidentified creator. |
| archafizov | [Modular Ancient Ruins on Fab](https://www.fab.com/listings/be34078b-d989-4a06-b956-17072740dc4f). [SETUP.md](../SETUP.md) identifies this purchased pack and its `/Game/AncientRuins` installation. `DCRuins.cpp`, `DCMaelstrom.cpp` and `DCCloudFace.cpp` reference its meshes. This is not Dekogon's similarly named Ancient Ruins collection. |
| PROTOFACTOR INC | [Kraken on Fab](https://www.fab.com/listings/6c03486a-a6fe-4c81-b699-f0c533ebf87d). The listing's 19 animations match the installed Kraken. `DCLeviathan.cpp` defaults to `/Game/Kraken/Meshes/KRAKEN`; [boss integration record](../taskboard/BOSS-CALDERA-15NOV.md) records the shipped selection. |
| HuyKhoi2407 | [Claw Kaiju Monster on Fab](https://www.fab.com/listings/450a7af0-6025-419b-9e68-60e4b177d6a0). The page names both publisher and concept creator HuyKhoi2407. Although Kraken replaced the main boss, `DCMaelstrom.cpp` still loads the claw model and crawling/death animations for the encounter's smaller spawnlings. |
| Quang Phan | [MHC: Pirates — MetaHuman Mega Bundle on Fab](https://www.fab.com/listings/cd3450e5-b15d-4bc4-8148-67380d056f34). [Purchased cast record](../taskboard/PIRATE-CAST.md) identifies the Davy/Breeza presets and four outfits; `DCPirateCast` uses the assembled characters. The three crew portraits are renders of those characters. |
| William Faucher | [Easy Fog on Fab](https://www.fab.com/listings/0a1c0c21-ff23-4fb7-9dc5-7beb6e16ce64). `DCWeatherFX.cpp`, `DCTreadmill.cpp` and `DCMaelstrom.cpp` reference `/Game/EasyFog` for mist/fog assets. |
| one.studio2 | [Clouds Tunnel Vdb Effects Pack Loop on Fab](https://www.fab.com/listings/7f0d28f5-aff1-400a-8468-391b96652902). Installed `/Game/Clouds_Tunnel_Loop` assets include `CT_Clouds_Tunnel_Vdb_01` and related volumes; `DCFogFront.cpp`, `DCTreadmill.cpp` and `DCMaelstrom.cpp` reference the pack. |
| Quixel Megascans | [Massive Nordic Coastal Cliff, exact purchased listing](https://www.fab.com/listings/93b50c63-e6e4-4508-8c03-bbedbae99b13), [Quixel asset vfmjadufa](https://quixel.com/megascans/home?assetId=vfmjadufa). Local Fab source JSON identifies **Massive Nordic Coastal Cliff** `vfmjadufa` and **Huge Nordic Coastal Cliff** `vefkehbva`; imports live under `/Game/LocalScans`. `DCMaelstromInlet.cpp` references those scan meshes; the generated cliff materials also derive normal detail from the Nordic source. Import evidence: `E:/Projects/DirectorsCut/Maelstrom-Saturday/Saved/OwnedCliffImports/{vfmjadufa,vefkehbva}.json`; source metadata remains in the matching FabLibrary folders under `C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/`. |
| Epic Games | [Paragon: Rampage on Fab](https://www.fab.com/listings/0807cf74-08fd-4a33-8c8d-f33c9439fb1f), [MetaHuman](https://www.unrealengine.com/en-US/metahuman). `DCTreadmill.cpp` uses the Rampage creature for the giants; gameplay fireball/impact/water effects and rock material details reuse Paragon assets. The pirate cast is assembled on MetaHuman and driven by the installed Unreal mannequin animations; `DCCrew.cpp` and `DCPirateCast.cpp` retain those references. |

## Audio pack creators

If the credits include all sourced packs rather than only visual art, add this separate line:

**Sonniss GDC 2026 Game Audio Bundle — Just Sound Effects, 344 Audio, InMotionAudio, Jake Fielding, Epic Stock Media and SoundBits.**

[Bundle source](https://gdc.sonniss.com/); [project source-file catalogue](../taskboard/AUDIO-SHORTLIST.md); [submission attribution record](../taskboard/SUBMISSION.md). `Tools/import_audio.py` imports prepared recordings into `/Game/Sonniss`; the ten installed sounds supply sea, wind, rain, fire, thunder, braam, creature and impact audio. Voice/model and software attribution remains separately documented in [ThirdPartyVoices](ThirdPartyVoices/README.md) and `Docs/ThirdPartyACELLM`; this art list does not replace those notices.

## Historical assets not established as shipped art

- **Poly Haven — Quarry Cloudy**, photography by Dimitrios Savva, processing by Jarod Guest ([source](https://polyhaven.com/a/quarry_cloudy)): mentioned conditionally in the older submission draft. Current main has no Quarry Cloudy asset or runtime reference. Its sole `/Game/DC/Maelstrom/Sky/M_MaelstromCumulus` material references the engine volume-noise texture; the active sky code uses a realtime skylight. Do not add this to the UI as shipped sky art based solely on that older draft.
- **Poly Haven — Marble Cliff 01**, Amal Kumar: provenance exists in an isolated cliff-review source manifest, and optional Blender preview tooling references it. Current runtime cliff provenance identifies the Nordic Quixel source instead. It is not included in the UI list without evidence that those preview maps shipped.
- Installed `Ancient_Marchosias` and Giang Demon folders, and shortlisted VICECLAW/SharkM assets, are not credited as used in the default voyage merely because their names occur in the workspace. The installed default cast and default route use the verified assets above.

No creator was inferred from a similar product title alone. The ruins identity is backed by the project's installation record; the other rows combine an installed/runtime reference with creator metadata or an official listing.
