# Phonon development adapter notices

- Runtime: `ptts` 0.7.1, Laurent Mazare / Gradium, https://github.com/gradium-ai/xn-ptts. Installed wheel declares `MIT OR Apache-2.0`. Original licence texts are retained in `licenses/`.
- Model: Gradium Phonon 31M English, https://huggingface.co/gradium/phonon-31m-en, revision `92741c5b07d085ff840e26fea8ce43110b87852f`, model card declares Apache-2.0. Original weights/config/tokenizer/default voice are copied without modification and checked against pinned hashes in `stage.py`.
- Voice embeddings: https://huggingface.co/gradium/phonon-voices, revision `0ee17205f254b5abc43b266c8a0f215565c352f7`, model card declares Apache-2.0. Selected Edwin, Hugh and Bryony embeddings are copied without modification. Metadata and checksums are in `voices.json`.
- Phonon builds on Pocket TTS and Mimi by Kyutai, as credited in the model card.

The Python bridge, benchmark and game adapter are project additions. This directory does not redistribute a Python runtime, NumPy, or the compiled `ptts` wheel. The local development environment is not a portable release payload. If distributing that environment, retain the Python/NumPy/runtime dependency licences and notices as well, including the `ptts` wheel's supplied CycloneDX SBOM and applicable dependency notices. Do not substitute the differently licensed Phonon 40M model.
