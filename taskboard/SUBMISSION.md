# Submission notes

Supporting drafts for the itch.io entry. **Use [SUBMISSION-TRACKER.md](SUBMISSION-TRACKER.md) for current requirements, owners, evidence and open release gates.** On 14 Nov, Codex checked Epic's jam page and the relevant sections of its English Official Rules (12 Nov revision). The gameplay/asset prose below remains a draft to reconcile against the final candidate.

## Submission essentials

| Item | What I found | Status |
| --- | --- | --- |
| Deadline | Sun 15 Nov, 8:59 PM US Eastern = **Mon 16 Nov, 01:59 GMT**. Late entries are disqualified. | Plan to submit by Sun 15 Nov, 18:00 GMT. |
| Where | itch.io jam page, account **Shakkara** | Final game URL/upload pending |
| Video | 30 to 60 seconds of recorded gameplay, **1920x1080 or higher** | To record; accessible link needed |
| Team | One human entrant confirmed; team name and legal/Epic details belong in the form | Owner to fill in privately |
| Sourced assets | Allowed with the right licence, must be **listed at submission**, and are not scored | List below |
| AI use | Optional; neither rewarded nor penalised | Describe honestly, see below |
| Build size | Epic explicitly directs larger desktop builds to Butler or Blendo | Upload rehearsal outstanding; see "Build size" |
| NVIDIA prize | Extra NVIDIA questions at submission | Draft answers below |
| OpenAI awards | Describe OpenAI models/products used and how | Add Codex development and Whisper runtime speech recognition |
| Language | All entry information and game/video content must be English | Final review outstanding |

Sources: [Epic jam page](https://itch.io/jam/2026-epic-ai-megajam), [Official Rules](https://epicgames.box.com/s/a7xcgl07fqoieeb99sjtd3juhj4agc4z). Team eligibility, licences and final form completion remain open; see the tracker.

## Sourced assets to list

- **Brigantine ship** (Fab, purchased by the owner): hull, masts, sails, cannons, deck props, cannon smoke and cannon/impact sounds, and the UE mannequins and idle/walk animations used as pose drivers for the cast.
- **MHC: Pirates | MetaHuman Mega Bundle** by **Quang Phan** (Fab, purchased): Breeza and Davy presets; Sovereign Corsair, Dread Captain, Tide Duelist and Rogue Swashbuckler outfits, headwear, hair and accessories. Uses Epic **MetaHuman** rigging, faces and optimized assembly. Integrated for the captain, Finch, Marlow and Odessa on 15 Nov; see [cast setup](PIRATE-CAST.md). Include in the final release package and disclose as sourced assets.
- **Ultra Dynamic Sky** (Fab, purchased): sky, sun, volumetric clouds, weather, rain, lightning and thunder, and the travelling storm that forms the front.
- **Modular Ancient Ruins** by archafizov (Fab, purchased): every column, arch, temple piece, statue and rock in the drowned city and on the open sea. The layouts are built in code from its pieces.
- **Dynamic Real Water 3.0** by Natural Level (Fab, purchased): the sea outside the tunnel and the maelstrom.
- **Claw Kaiju Monster** (Fab, purchased): the boss creature in the maelstrom, with its animations.
- **Unreal Engine 5.8** engine content: basic shapes, default fonts, volumetric cloud default material, sky atmosphere.
- **NVIDIA DLSS 4.5 plugin** v8.7.2 for UE 5.8 (DLSS Super Resolution; Streamline).
- **llama.cpp** (MIT) with the **Qwen3.5-4B** language model weights.
- **whisper.cpp** (MIT) with OpenAI **Whisper base.en** weights.
- **Sonniss GDC 2026 Game Audio Bundle** (royalty-free licence): sea, wind, rain, fire, thunder, horn swell, creature and water-impact recordings from Just Sound Effects, 344 Audio, InMotionAudio, Jake Fielding, Epic Stock Media and SoundBits. Exact files in `AUDIO-SHORTLIST.md`.
- **Piper** text-to-speech with `en_GB-vctk-medium` (Finch, speaker p226/95), `en_GB-northern_english_male-medium` (Marlow), and `en_GB-alba-medium` (Odessa). See [voice attribution and source licence cards](../Docs/ThirdPartyVoices/README.md); Piper/eSpeak and other binary dependencies have separate notices. The old Alan voice is excluded from the new package.
- **MHC: Pirates | Metahuman Mega Bundle**, Quang Phan (Fab, purchased), plus Epic MetaHuman: four assembled outfits and faces. See [cast record](PIRATE-CAST.md).
- **NVIDIA ACE Audio2Face**, base plugin 2.5 with local Mark regressive model 2.3, ported to UE 5.8: GPU facial animation for the three crew. Retain both plugin and model notices; see [integration and test evidence](ACE-INTEGRATION.md).

Add the overnight build's **Easy Fog**, **Clouds Tunnel Vdb Effects Pack Loop** (one.studio2) and **Paragon: Rampage** (Epic Games). If the sky branch ships, add **Quarry Cloudy** from Poly Haven (branch provenance records CC0). Reconcile exact assets, model/voice licences and required notices before submission. Add new crew art only if actually shipped.

## Made during the jam

Project work includes the C++ module, ride timeline, procedural geometry/material work, layout and treatment of sourced scenery, voice-order pipeline, crew behaviour and HUD. Do not describe purchased ruins, statues, creatures, ship, textures or recordings as original art. Reconcile this description with the actual shipped assets and retained development history.

## How AI was used (for the description)

- **In the game:** local Whisper recognizes spoken orders; a keyword grammar handles supported commands and local Qwen interprets unfamiliar wording/rephrases replies. Piper synthesizes distinct crew voices, and NVIDIA ACE Audio2Face animates their MetaHuman faces on the GPU. These inference paths run locally without a cloud account. Network access is not needed for inference; do not claim that graphics plugins or libraries never attempt network access.
- **In development:** Claude Code and Codex implemented and iterated code, materials, gameplay, visuals and testing under the owner's direction, using Unreal MCP and headless editor scripts. Codex's contribution extends beyond repository setup.

## Draft answers for the NVIDIA questions

- **Feature used:** NVIDIA DLSS 4.5 for Unreal Engine (plugin v8.7.2, NGX 310.6): DLSS Super Resolution at Quality (67%) is on by default. DLSS Frame Generation is integrated and supported but off by default.
- **Why:** the ride leans on hardware ray-traced Lumen, volumetric fog and clouds, a displaced ocean and many dynamic lights (lightning, deck fires, lanterns). Rendering at 67% and upscaling with DLSS keeps that affordable.
- **Also used:** NVIDIA ACE Audio2Face Mark, with local GPU inference in a separate CUDA context. Piper audio drives synchronized mouth/jaw animation on the purchased pirate crew. The packaged 15 Nov ACE candidate passed nine crew lines with DLSS active. Keep final-candidate evidence; [integration record](ACE-INTEGRATION.md).
- **Not used:** the NvRTX engine branch (RTXDI, ReSTIR).
- To verify in a build: the log prints `NVIDIA NGX DLSS supported DLSS-SR=1` and `DLSS requested: super resolution at 67%`.

## Build size — updated 14 Nov

15 Nov ACE candidate `2026-11-15_0825_ace-gpu-safe/Windows`: **11,877,441,966 bytes across 606 files** before runtime-test logs. This includes selected local speech/LLM sidecars and ACE model weights. It is an uncompressed test candidate, not a selected/submitted final build; remeasure the eventual release.

The preserved `2026-11-14_0748_69449a3_handover/Windows` folder measures **8,716,367,310 bytes across 617 files** (8.72 GB / 8.12 GiB). This is not a compressed download measurement and predates today's branch integration. Measure the final candidate separately.

Epic's jam page explicitly supports larger desktop uploads using Butler/Blendo. [Butler's documentation](https://itch.io/docs/butler/pushing.html) currently gives a 30 GB uncompressed backend limit. Rehearse this route before removing gameplay AI for size reasons; no external host is presently needed by the size evidence. Account/destination and upload remain untested.

### Historical size work, 12 Nov

Done on 12 Nov: the AI servers were switched from CUDA builds to a Vulkan build of llama.cpp and a CPU build of whisper.cpp with the smaller base.en speech model. Measured on this PC: replies are as fast as before (about 240 tokens a second) and a spoken order registers in about 0.4 to 0.7 seconds. This removed about 2.7 GB and no longer needs an NVIDIA card for the crew AI.

The approximately 5 GB figure recorded on 12 Nov is obsolete. Model cuts are optional download-size optimizations, not a verified jam requirement.

## Measured on the development PC (13 Nov, editor build, DLSS Super Resolution at 67%)

- 1280x720: open sea 115 to 150 fps, inside the storm front 120 to 139, Atlantis 88 to 105.
- 3840x2160 (rendered offscreen): open sea 95, storm front 102, Atlantis 84.
- Packaged build at 1280x720: 105 to 142 fps across open sea, Atlantis and the maelstrom.

## Still to do before submitting

- [ ] Owner plays it with a real microphone and reports what breaks.
- [ ] Owner listens to the sound mix.
- [ ] Rehearse Butler upload and measure final candidate size.
- [ ] Record the 30 to 60 second gameplay video.
- [ ] Final packaged build from a clean checkout, run on a second machine if possible.
- [x] Read relevant Official Rules sections for submission requirements (Codex, 14 Nov).
- [ ] Owner confirms team eligibility and final licence/disclosure completeness.
