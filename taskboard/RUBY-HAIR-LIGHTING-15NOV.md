# Ruby hair beside lanterns

User reported an excessively bright golden crown/backlight beside the ship lanterns during the 19:24 human playtest. Root owns this investigation. The human game remains running; no GPU render, build, material save or production lighting change was made.

`Tools/ACE/audit_hair_lighting.py` completed successfully with NullRHI. Local evidence is `Saved/AuditHairLighting.log`, `Saved/PirateCast/HairLightingAudit.json`, and exported material graph nodes in `Saved/PirateCast/HairLightingGraph`. It constructs and destroys temporary assembled actors, reproduces the runtime strand/binding assignment, records effective component materials, and never saves assets.

Findings:

- Ruby's restored hair uses the assembled `MI_WI_PirateHeadBandHair_None_3_Hair` in slot0, with `bIsHairCards=false`, `IsHelmet=false`, and a parent compiled for hair strands. The effective component material is retained across groom restoration; no cards/default-material mismatch was demonstrated.
- Parent `M_hair_v4` selects `MaterialExpressionSetMaterialAttributes_2` on desktop. That branch uses `MSM_Hair` and has no emissive output or input material attributes supplying emissive. The mobile branch also leaves emissive unconnected. This is not an authored self-illumination effect.
- Ruby's hair material uses `HairRoughness=.45`, `Spec0=.45`, `Spec1=.75`, melanin `.65`, and redness `.45`. These alone do not prove the excessive highlight's cause and have not been blindly changed.
- Her groom casts ordinary/dynamic/contact shadows. The four nearby director lanterns explicitly disable shadows in `DCRideDirector::AddLamp`, with a26m influence radius and12cm source radius. This is a plausible light-leak/backlighting cause, not yet a proven visual diagnosis.
- Engine review shows simply enabling deep shadows while ordinary lamp shadows remain off is insufficient in the traced deferred path: hair-shadow consumption depends on the light being classified as shadowed. Do not claim a deep-shadow-only toggle fixes it.

Next bounded review, once the human releases the GPU: reproduce a close Ruby/lantern view and compare one nearby lantern with ordinary plus hair deep shadows against the current baseline. Check appearance and frame cost before expanding to all lamps. If it does not improve the defect, isolate the hair highlight response rather than changing hair color or flattening all scene lighting. Preserve her authored red/brown hair, ACE face, groom binding and animation. No correction is visually accepted yet.

## Native comparison after playtest

`Saved/HairLightingReview/{Baseline,Shadowed,Midmast}/shot_0120.png` capture the same Ruby closeup at actual ride126s. All four shadowed lamps reduce the implausible bright hair response; shadowing only the paired midmast lamps retains that improvement. Root selected the two midmast lamps as the default, with ordinary and deep shadows enabled; distant lamp defaults and all hair materials remain unchanged. `-DCShadowLamps=0/6/15` reproduces baseline/pair/all.

Single capture timings were95fps/5.7ms GPU baseline,89fps/10.3ms GPU all four, and96fps/8.6ms GPU paired. These are diagnostic samples, not a benchmark or proof of equal frame cost. Final combined voyage performance still needs checking. Native editor build passes. The accepted19:24 package remains unchanged until a new package is selected.
