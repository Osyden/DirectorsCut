# Helm wheel hub correction

Follow-up on `codex/early-gameplay`, based on main `d370da9`.

The reported pivot defect was still present: steering rotated the `SK_Wheel` component around its imported origin at the base of the stand. At 45.5 degrees of steering, the wheel and drum moved 80.4612 cm out of their supports. The pedestal is separate fixed geometry.

The player now caches the authored hub bone (`Bone_001`) in the component parent's space. Steering compensates the component position while rotating around that hub. Leaving the helm restores both the original position and rotation. Steering input, ship response, the existing wheel animation blueprint and content assets are unchanged. A missing hub bone leaves the visual transform unchanged and emits a warning.

## Verification

Both directions were captured through the existing `DCHelmSequence` input path, with neutral frames before and after. The corrected wheel and drum stay seated in the supports. The editor-runtime maximum hub drift is 0.0002 cm, compared with 80.4612 cm before. Leaving the station while turned produced zero measured location or hub error.

- [Before, right steering](review/early-gameplay/helm-pivot-15nov/before-right.png) / [after](review/early-gameplay/helm-pivot-15nov/after-right.png)
- [Before, left steering](review/early-gameplay/helm-pivot-15nov/before-left.png) / [after](review/early-gameplay/helm-pivot-15nov/after-left.png)
- [Measurements](review/early-gameplay/helm-pivot-15nov/metrics.json), [baseline events](review/early-gameplay/helm-pivot-15nov/before-results.txt), [fixed events](review/early-gameplay/helm-pivot-15nov/after-results.txt)

`Tools/inspect_helm_wheel.py` provides a read-only asset inspection in a transient editor world. Both review scripts accept `-Case helm`; add `-DiagnosticArguments '-DCHelmLeaveAt=10.5'` to exercise the actual leave-station path while turning. Runtime telemetry is opt-in through `-DCHelmLog` in non-Shipping builds.

Editor and Windows Development builds passed. Build/cook/archive finished in 119 seconds after the release-content preflight passed. All 85 Source/Config/project manifest entries match the reviewed checkout. The actual Windows executable passed the same left/right/leave sequence with four fresh screenshots. Its maximum hub drift is below 0.001 cm and release restores the original position exactly to the logged precision. The wheel stays seated in both support holes in the reviewed [right](review/early-gameplay/helm-pivot-15nov/packaged-right.png), [left](review/early-gameplay/helm-pivot-15nov/packaged-left.png) and [released](review/early-gameplay/helm-pivot-15nov/packaged-released.png) captures. [Packaged events](review/early-gameplay/helm-pivot-15nov/packaged-results.txt).

Accepted package: `E:/Projects/DirectorsCut/Builds/2026-11-15_1129_helm-pivot/Windows/DirectorsCut.exe`. Voice/audio were disabled for this visual steering check. Existing optional profiler/NGX startup diagnostics and the legacy ship pawn EnableInput message remain; no missing game object/package, fatal, assertion or Blueprint runtime failure was found. The earlier full-route and voice checks were not repeated or represented as tests of this executable.

Other chats' uncommitted taskboard files were backed up and hashed before integration. This patch does not edit their files, shared content assets or the taskboard README.
