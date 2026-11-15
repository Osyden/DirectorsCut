# Crew voice notices

Runtime synthesis uses the existing Piper installation and the following unmodified ONNX models from [rhasspy/piper-voices](https://huggingface.co/rhasspy/piper-voices). Retain the adjacent upstream model cards in distribution. The cards describe their source-data licences; do not assume Piper's software licence also licenses every voice.

| Character | Model / speaker | Source and attribution | Documented source licence |
| --- | --- | --- | --- |
| Finch | en_GB-vctk-medium, p226 / speaker ID 95 | CSTR VCTK Corpus, Junichi Yamagishi, Christophe Veaux, Kirsten MacDonald; [University of Edinburgh](https://datashare.ed.ac.uk/handle/10283/3443) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Marlow | en_GB-northern_english_male-medium | UK and Ireland English Dialect speech data set / OpenSLR 83, copyright 2018–2019 Google, Inc.; [OpenSLR](https://www.openslr.org/83/) | [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) |
| Odessa | en_GB-alba-medium | Alba speech corpus (2019), Cassia Valentini-Botinhao and Junichi Yamagishi, University of Edinburgh; [source](https://doi.org/10.7488/ds/2506) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |

The models are not fine-tuned or modified by this project. Runtime settings adjust speaking pace and generation noise. Generated dialogue is fictional crew speech, not a recording or endorsement by the corpus speakers. Attribution does not imply endorsement.

VCTK model SHA-256 verified against Hugging Face's LFS metadata: `4e9fc85ab9009385319fc6bae7f55577f8a2d7ee77fd9159a5500eb6531f41e6`.

The old Alan model is intentionally excluded by `Tools/ACE/stage_sidecars.ps1`: its model card refers to an upstream file stating “All Rights Reserved”, so its redistribution terms were not established. Do not package every voice from the development Sidecars folder indiscriminately. The Alan and ARU cards were fetched during evaluation; they are not selected runtime voices.

Piper, espeak-ng, ONNX Runtime, Whisper, llama.cpp and the language model have separate software/model notices. Preserve the binary distribution's notices and upstream source references alongside these voice notices. NVIDIA's plugin and Audio2Face model notices are staged separately; the model is not covered by the plugin's MIT licence.
