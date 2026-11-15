# Early statue targets and encounter density

Source branch `codex/fog-statue-polish`, based on main `ded0760`. No engine, editor, GPU or asset generation used. Root owns native validation and packaging.

Statues now take **four individual cannon hits**, previously three. Their first attack waits for11 seconds of actual clear cannon exposure before the unchanged3.5-second charge. This gives one cannon enough time to recognize the target, reload three times and land its last shot. Subsequent cooldown, damage, charge duration and global4-second charge spacing are unchanged.

An encounter-owned HUD delegate brackets the actual hittable eye position, with side, firing instruction and charge countdown. It appears only for a living, visible, cannon-reachable statue; masonry, concealment, death and phase exit suppress it. Offscreen targets get a directional marker. It reuses the existing boss marker placement policy, but neither DCGame nor boss source is changed. The existing overhead world charge cue remains.

Each of the four existing creature waves now has three creatures: **twelve total, previously nine**. Spawn times, alternating banks, supported placement, pursuing movement and firing gaps remain. The third member uses the same bound-fitted bank placement already used in the third wave. No route/architecture/steering geometry changes.

## Checks

`Tools/test_fog_route.ps1`: **296,684 CPU checks pass**. Includes full hull/route clearance, installed walker support bounds for all twelve creatures, statue marker eligibility, actual charge gate with all waves/statues/repeated attacks at15/30/60/120fps, and solo cannon/pass timing.

Four-hit final impact is14.0s after first visibility with2s recognition, three3s reloads and worst-case3s flight; first beam releases at14.5s. The shortest reachable pass across centre and native held-helm lanes±7219cm is17.65s, using actual210m range,1.9 broadside ratio, measured ship/bank dimensions and1500cm/s travel. It supports the14.5s release plus.7s beam impact. Occlusion or another charge can still cancel a particular statue's window, as before; native ignored-statue checks should confirm attacks remain meaningful in the authored scene.

No concurrent urgent charge demands appear in the all-wave replay. Damage/fire cap and released projectile handling are unchanged. Existing creature animation, pose clock and hand release are untouched.

## Native review

Root's usual native launch can use:

```
-DCStartAt=100 -DCRunThrough -DCStation=starboard -DCEarlyTargetReview -DCAutoShots=107,112,118,123,127
```

`DCEarlyTargetReview` is a nonshipping hook in the existing actual-cannon camera path; it selects a reachable live statue on that battery and applies normal yaw/pitch clamps. It never shoots automatically. Before a clear firing window, absence of the marker is expected.

For actual four-hit ballistic verification add `-DCStatueVerify` (start100; run until the existing harness exits at236→241). It verifies statue preemption, four shots with at most one miss, ignored creatures/statues delivering attacks, fire-cap compliance and phase cleanup. The existing `-DCEncounterVerify` retains its original moving-creature three-hit check.

Native build, marker visual/layout inspection, four-hit ballistic assertions and ignored-statue beam confirmation remain pending at handoff. No new material or asset generation is needed.
