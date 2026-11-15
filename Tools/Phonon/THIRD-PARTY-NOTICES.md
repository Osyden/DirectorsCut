# Phonon portable speech payload notices

- Runtime: `ptts` 0.7.1, Laurent Mazare / Gradium, https://github.com/gradium-ai/xn-ptts. Installed wheel declares `MIT OR Apache-2.0`. Original licence texts are retained in `licenses/`.
- Model: Gradium Phonon 31M English, https://huggingface.co/gradium/phonon-31m-en, revision `92741c5b07d085ff840e26fea8ce43110b87852f`, model card declares Apache-2.0. Original weights/config/tokenizer/default voice are copied without modification and checked against pinned hashes in `stage.py`.
- Voice embeddings: https://huggingface.co/gradium/phonon-voices, revision `0ee17205f254b5abc43b266c8a0f215565c352f7`, model card declares Apache-2.0. Selected Edwin, Hugh and Bryony embeddings are copied without modification. Metadata and checksums are in `voices.json`.
- Phonon builds on Pocket TTS and Mimi by Kyutai, as credited in the model card.

The Python bridge, benchmark and game adapter are project additions. The combined next-test payload contains an isolated CPython 3.13 runtime, NumPy 2.5.4 and the compiled `ptts` 0.7.1 wheel under `python/`, staged by `stage_python.py`. It requires no external Python installation and performs speech synthesis on the CPU. `stage.py` stages the model and voices separately; its output alone is incomplete without this runtime.

Retain `python/THIRD-PARTY-NOTICES.txt`, `python/licenses/`, the complete wheel metadata and licence directories under `python/Lib/site-packages/`, and the `ptts` wheel's CycloneDX SBOM. `python/payload-manifest.json` records copied runtime files and hashes; `python313._pth` isolates imports from system and user Python environments. Runtime notices include CPython, NumPy's bundled dependencies and Microsoft runtime DLLs; the presence of those files is not a separate redistribution grant. Applicable Microsoft redistribution terms still need release review. Windows system libraries remain OS prerequisites.

`staging.json` describes the combined payload layout, not a claim of native or second-machine acceptance. `stage_sidecars.ps1` checks payload completeness and hashes before packaging. Do not substitute the differently licensed Phonon 40M model.
