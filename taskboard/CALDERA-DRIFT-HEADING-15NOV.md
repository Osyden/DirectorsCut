# Caldera drift heading

Source branch `codex/caldera-drift-heading`, based on `cd5683e`. Root owns native review. Landing/music remains voice_audio's work; this patch changes only helm heading, review flags/logging and related checks.

The user clarified that the ship should drift sideways, not rotate strongly into its radial movement. Fog's normal physical helm yaw is around8°, whereas the previous caldera implementation could turn the entire arena40° as velocity changed. The caldera now scales its drift heading by0.16, caps the bow deviation at10°, eases at1.8/s, and limits heading movement to6°/s. Its base heading remains tangent to the unchanged circular route.

Radial velocity, acceleration, countersteering, damaged-rigging authority, braking limits, inlet guard, wave coordinate mapping and boss commitments are unchanged. The visual heading follows the drift gently rather than saturating at40° almost immediately.

## CPU checks

`Tools/test_boss_caldera.ps1`: **56,715 checks pass** at15/30/60/120fps.

- Full six-second D then A: peak heading8.912°, maximum heading rate6°/s. Visual heading crosses the tangent1.733s after reversal. Actual carried radial velocity reverses in0.733s, preserving dodge responsiveness.
- Release settles to the tangent:0.015° residual heading after6s. Neutral coast remains10.99m.
- **1,152 committed obstacle scenarios pass**, including two/three-second reaction, intact/damaged rigging, lane extremes, initial radial momentum and initial visual heading−10/0/+10°. Sweeps now use the production smoothed heading, not an instantaneous-yaw approximation.
- Existing route/limit, cannon arc, attack schedule and damage-budget checks pass. Native route automation gains heading-amplitude and no-snap checks; not run by this agent.

## Native review

```
-DCStartAt=346 -DCRunThrough -DCStation=helm -DCMaelstromSteerCycle=6 -DCMaelstromRouteLog -DCAutoShots=348,350,352,354,356,358,360,364
```

`DCMaelstromSteerCycle=6` alternates six seconds of held D/A from ride310; ride346 is the start of a full D interval after the inlet outward guard has cleared. This is nonshipping review only. `MAEL_ROUTE` logs every0.5s with input, radial position/velocity, actual yaw and target yaw. Review the smaller horizon movement, gradual counter-rudder and readable sideways drift from the ordinary helm. No material regeneration is required.

When merging with voice_audio's landing patch, preserve both changes in the adjacent reset area: this patch adds a separate `if(Reset) { HelmYaw=0.f; }` and does not replace landing reset/contacts/impact code.
