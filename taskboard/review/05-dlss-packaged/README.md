# DLSS 4.5 in the packaged build: frame rate and image evidence

Measured Thu 12 Nov on the packaged build (`Builds/Windows/DirectorsCut.exe`), rendering 3840x2160 offscreen (`-RenderOffscreen -ForceRes -ResX=3840 -ResY=2160`), crew AI off, on an NVIDIA RTX PRO 6000 Blackwell. Each run seeks to three ride times and logs a smoothed frame rate when it takes the screenshot.

| Mode | Internal resolution | Fog (2:30) | Maelstrom (8:00) | Maelstrom (8:30) | vs DLAA |
|---|---|---|---|---|---|
| DLAA (DLSS at 100%) | 3840x2160 | 85 fps | 94 fps | 95 fps | baseline |
| **DLSS Quality (67%), the game's default** | 2573x1447 | 114 fps | 135 fps | 135 fps | about +40% |
| DLSS Performance (50%) | 1920x1080 | 129 fps | 157 fps | 156 fps | about +60% |
| DLSS Ultra Performance (33%) | 1280x720 | 153 fps | 179 fps | 178 fps | about +85% |
| No DLSS (`-DCNoDLSS`) | engine default | 132 fps | 158 fps | 159 fps | — |

How to read this:

- The fair baseline for DLSS is **DLAA**: the same DLSS model at native resolution. Against that, Quality mode is about 40% faster.
- The "No DLSS" row is **not native 4K**. With DLSS off, Unreal's own upscaler (TSR) picks its own lower internal resolution at 4K, which is why it lands at the same speed as DLSS Performance. It is here to show that the alternative to DLSS in this project is another upscaler, not native rendering.
- This GPU is very fast and the game is partly CPU-limited on it, so the gaps are smaller than they would be on a mid-range card. I have no slower machine to measure on.
- One run per mode; treat differences of a few fps as noise.
- Frame Generation is integrated (`F6`) but not measured here: the game's own frame counter does not count generated frames.

Images:

- `crops_1to1_shot_0480.jpg`, `crops_1to1_shot_0150.jpg`: the same 768x432 region cut 1:1 from each 4K frame (rigging against bright water, where upscaling artefacts show), one column per mode.
- `frame_<time>_<mode>.jpg`: the whole frame for each mode, scaled to 1920x1080.

The frames are not identical between modes: the sea, spray and creature are animated and each run is a separate launch, so compare sharpness and stability of fine lines, not exact content.

Confirmation that DLSS is live in the packaged build: its log prints `NVIDIA NGX DLSS supported DLSS-SR=1 DLSS-RR=1` and `DLSS requested: super resolution at 67%`.
