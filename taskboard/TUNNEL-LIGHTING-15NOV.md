# Fog-to-water-tunnel lighting handoff

The user reports strange lighting at the tunnel end in the19:24 playable. Their screenshot timestamp19:45:20 matches the runtime log's `Phase -> The slide at240.0s` at19:45:20.064. This identifies the fog-city exit/water-tunnel entrance, rather than the later caldera exit. The frame includes an incoming fireball, so its warm local light must not be mistaken for the atmosphere defect.

Root owns the lighting review. The human playable remains running and unchanged; no build or GPU review is started during their test.

Source findings, not yet a confirmed visual cause:

- `TickUDS` abruptly enables sun light-shaft bloom at240s. Other environment values interpolate over6s, while city fog protection fades over8s. This is the only identified instantaneous lighting toggle at the reported moment.
- The director's effective UDS exposure bias rises from approximately2.00 to3.64 over the slide blend. City upper fog and reduced sun scattering are also withdrawn. These smooth changes may amplify glare after entry, but do not alone prove the exact screenshot defect.
- The portal water shell does not cast shadows. Its opaque interior and separately managed fog need comparison with the lighting transition before changing geometry or broad fog density.
- Earlier no-shafts tests at167s were inconclusive for this issue: shafts are already disabled at that time. They do not test the240s toggle.

Source correction prepared: defer the shaft enable until the existing caldera atmosphere handoff, one second before the configured Breach phase (currently269s). It follows phase data rather than hardcoding a separate transition. The later atmosphere already owns its bounded bloom settings. No exposure, fog density or portal geometry changes. Compilation and visual acceptance remain pending; the running human package is unchanged.

Next bounded review: capture the accepted package at232,238,240,242,246,250s, then run `Tools/review_tunnel_lighting.ps1` against the rebuilt editor after the human releases the GPU. It captures the same continuous first-person approach, comparing candidate baseline and light shafts disabled. It refuses to launch when a game/editor is already running. Compare tunnel mouth, rigging, deck and incoming-fireball variation separately, then check the269s caldera lighting handoff. If the defect remains, isolate portal shading and fog coverage. Do not ship an unverified global exposure/fog change.

Native continuous before/after captures are now in `Saved/PolishLighting/TunnelOld` (accepted1924 package) and `TunnelNew` (rebuilt editor), through273s. Both exit0. The240/242 views remain substantially alike, including bright rigging/sail contours; removing the source toggle has not demonstrated a fix for the reported appearance. Keep the issue open and investigate the shared sail/fog composition rather than claim the handoff change resolves it. No global exposure/fog change was made.
