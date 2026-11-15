# Water ruins restored

Owner: tunnel_boss, codex/water-ruins-restore, based on main 7cafdfa. Source only; no assets, engine launches, builds or GPU usage. Selected/uploading 23:10 package untouched.

The old course put all obstacles 26-34m off centre with a fitted9m radius and5m hull half-width, so a straight centre route always cleared.60% were crags without pillar wash.

The new course uses five existing broken-column clusters26s apart, at lateral positions -6.5,+18,-14,+10,-21m. Positions commit at spawn. Each cluster stays inside its fitted9m radius. Opposite passing lanes at33m clear even a fully yawed hull, and successive longitudinal occupied spans have328m between them at city speed. All use existing cooked column meshes, wake material and local-space pillar splash.

Stone now subtracts the same ship-frame SeaRise as water and banks, fixing foundations exposed by hard steering. Stone remains fixed; only foam and wash sample instantaneous rendered waves including the trough. Roots sit300cm below mean sea with8m foundations. Fitted upright tops retain6m above the conservative future wave envelope.

A water cluster gives one heavy collision event with existing hull damage/jolt/crunch plus a waterline splash. Continuous pushaway/braking remains active; repeated grinding damage is suppressed for that cluster only. Other terrain is unchanged. Lookout warnings begin170m ahead, retry rejection and use weak-actor/current-threat predicates.

## Validation

Tools/probe_water_ruins.py:33,640 checks pass. Production layout and DCSteering are tested at120/30/10Hz, with2s reaction and full/35% rigging authority. Worst sampled clearance13.9m for central approaches to the passing lane. Checks cover full-hull gaps, banks, waves across every helm offset, submerged foundations and one impact under sustained contact. This does not guarantee every late player input.

DirectorsCut.Navigation.CityPillarClearance updated to test deliberate avoidance instead of the obsolete always-empty centre. Native tests belong to root and were not run here.

## Root review

Continuous start60: first spawn near64, pass near107, then about133/159/185/211. Relevant logs: CITY_WATER_RUIN, PILLAR_FIT, PILLAR_CONTACT, LOOKOUT_WARNING, CITY_WATER_IMPACT.

- Contact/helm: -DCStartAt=60 -DCTimeScale=2 -DCStation=helm -DCSteer=0 -DCNoCurrent -DCOrders=Hold_fire -DCPillarLog -DCLaneLog -DCAutoShots=101,104,106,108,111
- Side footing view: add -DCShotCam=-4000,-6000,1700,-18,37 and use shots106/107/108.
- Passing route: -DCStartAt=60 -DCTimeScale=2 -DCStation=helm -DCSteer=1 -DCPillarLog -DCLaneLog -DCAutoShots=108,134,160,186,212
- Native automation: DirectorsCut.Navigation.CityPillarClearance, plus existing -DCNavigationVerify sweep regression.

Require visible submerged bases, foam meeting water, one CITY_WATER_IMPACT per actor with splash/jolt and lateral separation, and no bank overlap at maximum helm. A neutral path through an actual gap in an irregular cluster can miss; the cluster collision radius is deliberately conservative. Appearance, exact contact and splash strength remain native acceptance checks.
