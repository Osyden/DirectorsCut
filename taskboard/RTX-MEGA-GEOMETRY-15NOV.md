# RTX Mega Geometry decision — 15 November

Defer engine migration for this jam build. Keep the existing UE 5.8/Nanite pipeline and spend the remaining time on authored cliff geometry, materials, water and the boss.

RTX Mega Geometry accelerates ray-tracing acceleration structures for dense triangle clusters. Its SDK also demonstrates dynamically tessellated/displaced subdivision surfaces, but those still require supplied geometry and displacement data; it does not author realistic geology or textures. [NVIDIA SDK](https://github.com/NVIDIA-RTX/RTXMG).

NVIDIA documents Mega Geometry through its custom Unreal branches and lists an NvRTX 5.8 branch. Its public branch page describes the experimental path as not fully validated. We found no documented drop-in Mega Geometry plugin for the installed Launcher engine. The access-controlled 5.8 source and its exact limitations were not inspected. [NVIDIA Unreal branches](https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch).

An RTX 5090 plus Nanite/hardware Lumen does not establish that Mega Geometry is active. NVIDIA's Bonsai guide documents explicit NvRTX settings and a source-build workflow; do not treat those variables as a verified shortcut in stock UE 5.8. [Bonsai developer guide](https://dlss.download.nvidia.com/demos/bonsaidiorama/NVIDIA%20RTX%20Bonsai%20Diorama%20-%20Developer%20Guide.pdf).

No engine, driver or renderer migration was performed. Engine compilation, plugin compatibility, shader rebuilds, packaging and regression checks would consume time without fixing the failed stacked-cliff silhouette. A separate post-delivery experiment remains possible.
