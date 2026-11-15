# Phonon crew voice candidates — 15 November 2026

Branch: `codex/speech-streaming`. Scope: official catalog research and `Tools/Phonon/voices.json`; no game-source/default-voice changes. These are **unauditioned candidates**, not approved casting or measured quality results.

| Crew | Catalog voice / ID | Catalog metadata | Selection rationale (inference) |
| --- | --- | --- | --- |
| Rex | Edwin / `zUCxzrOrugTSis03` | `en-gb`, male, Middle-age | Firm, concise articulation may suit the watchful helmsman. |
| Duncan | Hugh / `YyKcxPG7iIgS3jsK` | `en-gb`, male, Middle-age | Softer, more intimate delivery may contrast with Rex. |
| Ruby | Bryony / `gfP44RlHwr2kEwk7` | `en-gb`, female, Middle-age | Rich middle register and relaxed delivery may suit grounded dialogue. |

All three are British-locale candidates. **No explicitly Scottish voice was identified** among the 102 catalog entries, including 13 UK voices (six male, seven female). Locale `en-gb` does not establish a Scottish accent. Retain the existing Scottish voice if that specific accent is required until a suitable candidate is verified. Descriptions in the manifest are paraphrases; the linked catalog contains the original descriptions.

Ruby alternative: Maeve (`6PWnV0Nq4wu7RVBT`, female, Adult, `en-gb`), with a brisk, cheerful assistant-style description. Rex character alternative: Captain Silas Blackwater (`xPDpsV3F592srkjA`, locale `characters`), explicitly described as West Country with an older, rough, theatrical pirate delivery. Its gender and age fields are null; do not substitute inferred values. Neither alternative is in the default crew mapping.

## Provenance and integrity

- Publisher/repository: [gradium/phonon-voices](https://huggingface.co/gradium/phonon-voices).
- Pinned revision: `0ee17205f254b5abc43b266c8a0f215565c352f7`.
- [Pinned model card](https://huggingface.co/gradium/phonon-voices/blob/0ee17205f254b5abc43b266c8a0f215565c352f7/README.md), [catalog JSONL](https://huggingface.co/gradium/phonon-voices/resolve/0ee17205f254b5abc43b266c8a0f215565c352f7/voices.jsonl), [embedding metadata API](https://huggingface.co/api/models/gradium/phonon-voices/tree/0ee17205f254b5abc43b266c8a0f215565c352f7/voices?limit=1000).
- Catalog Git blob: `0bbfdc6749bb9bab0d038627040482ce9a8e5792` (Git object ID, not a raw-file SHA256).
- Each selected embedding is 256,136 bytes. The manifest records SHA256 from the official Git LFS metadata. These hashes were **not locally verified against downloaded artifacts**.
- Model card identifies Phonon S (31M) compatibility: format `ba7a5a45@300`, tensor `speaker_wavs`, shape `[1,512,125]`, float32. This is published compatibility information, not a local integration test.

The machine-readable manifest exposes `catalog`, `revision` and `crew.Rex/Duncan/Ruby` records with `voice_id`, `name`, `filename`, `sha256` and `bytes`. Stage files at `filename` relative to the manifest directory, verify hash and size, then pass those local paths to the adapter. No runtime catalog/network lookup is necessary. Candidates and alternates must not be silently interchanged after validation failure.

## Licence and redistribution

The pinned model card declares **Apache-2.0**. No standalone LICENSE or NOTICE file was listed at the repository root in the inspected revision. Before redistributing embeddings, include the [Apache 2.0 licence text](https://www.apache.org/licenses/LICENSE-2.0), retain applicable upstream attribution/notices, identify modified files if any, and preserve any supplied NOTICE material. Record Gradium, repository and revision in release credits/third-party notices. The voice-catalog declaration does not establish the licence of the separate speech model, Python package or their dependencies; audit those independently. This research does not establish jam eligibility or approval of the complete redistributed stack.

## Verification and handoff

Read only public model-card, catalog and metadata endpoints. Confirmed unique IDs, exact gender/age/locale metadata, catalog counts, and JSON parsing. No embedding/audio downloads, auditions, synthesis, engine/build/GPU jobs or changes to the current playable. Next: parent integrates local staging/verification, then evaluates the three voices with identical short crew lines and records intelligibility, distinction, accent, latency and cancellation behaviour. Root owns shared runtime/GPU scheduling.
