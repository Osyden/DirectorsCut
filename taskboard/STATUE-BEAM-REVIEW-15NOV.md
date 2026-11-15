# Empty-deck statue beam fixture

Owner cannons_helm; isolated codex/statue-beam-review from55b3c6f. Root owns native/build lane.

Non-shipping -DCStatueBeamReview suppresses only SpawnGroup creature waves. Real statues, timing, line-of-sight, beam, hull damage, fire cap and crew remain live. Normal play and shipping ignore the flag.

Use -DCNoMenu -DCStartAt=100 -DCOrders=Hold_fire -DCStation=starboard -DCStatueBeamReview. Do not combine with autofire, DCEncounterVerify, DCStatueVerify or DCNoEarlyEncounters. Fixed deck camera may replace station for fire visibility. First beam must log contact1 fires0->1 damage1.200 then contact2 fires1->2 damage0.000, endpoint separation>=240cm, count<=MaxFires. PASS/FAIL prints at second contact; app exits with0/1 two world-seconds later, after the1.2s sweep. Missing/blocked beam times out60 real seconds with exit1.

Read-only structural checks confirm all new fixture code/state are excluded from shipping; production beam/contact paths are not replaced. Native proof pending root. Accepted23:10 package untouched; no engine/build/GPU jobs run here.
