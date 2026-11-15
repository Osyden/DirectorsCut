# Waterfall VDB worker — exact pack and integration approach

User reference: [video K93PIxTAgvk](https://www.youtube.com/watch?v=K93PIxTAgvk). The creator's [Ultra Waterfall 3d Vdb Effects Pack Loop listing](https://www.fab.com/listings/74ff9686-5856-4d9d-92f7-a199cf356842) explicitly links that video and describes ten effects with density, colour and speed controls for UE5.5 and above. The [creator's Epic forum announcement](https://forums.unrealengine.com/t/one-studio2-ultra-waterfall-3d-vdb-effects-pack-loop/2725724) identifies the same product. The video itself could not be fetched in this session; this is a verified product identification, not a claim to have watched its motion.

## Finding

A waterfall-shaped baked VDB is a plausible fast route for the aerated falling-water volume and impact spray missing from our current sheets. It does not replace the cliff layout, opaque flowing-water core or surface foam advection. Start with one chosen fall and one impact region; do not enlarge a cloud tunnel to imitate water.

The pack is **not installed** in the main or Maelstrom-Saturday Content folders as of this audit. The existing Clouds_Tunnel_Loop contains ten SVTs totalling 13,565,610,766 bytes on disk. That number describes the existing cloud assets only, not the waterfall pack. The public waterfall listing does not supply voxel resolution, frame counts, disk size, memory use or real-time performance measurements. Those remain unknown until the assets can be inspected.

The exact useful install destination is `E:/Projects/DirectorsCut/Maelstrom-Saturday/DirectorsCut.uproject`. Installing into the main DirectorsCut project is also workable if the resulting Content folder is identified so the isolated checkout can reference the installed pack. No purchase or installation was performed by this worker.

## Implementation once installed

1. Run `Tools/audit_waterfall_pack.py` through Unreal Python with the pack root. It inventories package/class/size and loads at most three SVTs to read dimensions, animation frames and transforms. It changes no vendor content and writes only `Saved/WaterfallPackAudit.json`.
2. Inspect the pack's actual material and example actor, including its density channel, pivot, frame transforms, physical flow direction and loop seam. Use those measured properties instead of assuming a generic actor class or fixed frame count.
3. Place one volume against a generated cliff's actual lip and receiving-water contact. Use a rigid transform and consistent physical scale. Retain the bespoke water core behind the aerated volume and retain the surface foam trail at its foot.
4. Build an original wrapper/material instance referencing installed content, with no self-emission for ordinary water. If density changes are needed after rescaling, tune extinction against world-space thickness and the scene's existing sun/fog. Never stretch a thin volume across the whole crater.
5. Start with nonblocking streaming and one active hero volume. Compare one and two visible volumes from the same moving deck cameras at native resolution, log GPU and streaming cost, and check near/far mip changes and loop timing. Only then select the visible-volume budget and distant fallback.
6. Include a real hard/soft reference or explicit cook inclusion for the chosen asset; a runtime string load alone may omit it from a package. Keep the pack's data out of the public repository. Test entry, backward seek, re-entry and exit without persistent actors or global render overrides.

## Runtime constraints

Epic documents baked animated SVTs as a sequence of volume frames rendered with Heterogeneous Volume actors. Streaming bandwidth, mip selection and ray-march step size affect playback and quality; blocking requests are specifically discouraged for real-time playback. Use the shipped game renderer for evaluation rather than judging a path-traced promotional render. See [Heterogeneous Volumes](https://dev.epicgames.com/documentation/unreal-engine/heterogeneous-volumes-in-unreal-engine) and [Sparse Volume Textures](https://dev.epicgames.com/documentation/en-us/unreal-engine/sparse-volume-textures-in-unreal-engine).

Local UE5.8 headers confirm runtime controls for frame range/rate, looping, streaming mip bias, blocking requests, step factor and shadow step factor, plus SVT frame-count and resolution queries. No frame-count, material parameter or actor class has been guessed for the missing pack. No speculative runtime adapter is enabled in the game.

## Ownership

Root continues the cliff geometry, shared waterfall path and build/render process. This worker adds only the read-only audit tool and this note. No cloud, water-core/foam shader, live editor or runtime source was changed.
