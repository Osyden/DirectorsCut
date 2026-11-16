# Moving route-joint ballistic correction

Owner cannons_helm, isolated codex/route-weakpoint-lead fromb1569ee. Root owns engine/native lane.

Actual Obstacle review failed: aimed shots325/328/331 yielded zero BOSS_HIT and contact332. ResolveAim selected the near sphere surface with no motion compensation, because only DCEarlyThreat had lead. Route joint moves about880cm/s; actual swept radius is1400*0.6+15=855cm, smaller than its travel during ball flight.

Minimal correction tags only route targets DCMovingWeakPoint and admits that tag to the existing two-pass aim lead. Boss update publishes target displacement in world coordinates divided by world delta seconds, matching projectile/reload time even in accelerated review. Spawn/reconstruct zeros velocity; zero ride delta, seeks, large discontinuities and long hitches reset it. Slam/mouth/body, radius, joint pose, damage gates and cannon physics unchanged.

Tools/probe_route_weakpoint_lead.py passes120 cases replaying three recorded shots at15/30/60/120fps,1/2/3 time scales and camera parallax. Existing edge aim misses; moving-centre aim intersects unchanged855cm sphere. This is a numerical regression model, not native acceptance. git diff --check clean.

Root must rerun Obstacle: expect two BOSS_HIT, remaining1 then0, BOSS_INTERRUPT before332, no BOSS_CONTACT. In initial frame review the revised authored limb silhouette/root immersion and marker looked acceptable; the failure was ballistic. Separate Slam native already proved three hits (remaining2/1/0) then permanent interruption at349.39.

No engine/build/GPU launched, accepted/uploaded23:10 package untouched.

Root native acceptance after integration9a3fb97: compile succeeded; two hits at327.04/329.83 reduced remaining1/0, interruption at329.83 preceded collision332, and no contact event occurred. Evidence Saved/PostUpload/Native/Obstacle; original failed run retained as Obstacle-before-lead. This closes the native requirement above.
