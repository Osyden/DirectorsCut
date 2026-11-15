# Ruby hair lighting recurrence

Root owns codex/ruby-hair-local-lights, prepared in the existing root-only Credits-One-Page worktree after the credits commits. Final scope: DCRideDirector lantern shadow coverage only. The user is still playtesting; no build, engine launch or package modification is authorized until that reservation is released.

The earlier accepted correction enabled ordinary and deep shadows only on the two midmast lights (mask 6). Bow/stern lanterns retain 26-metre influence radii and can backlight crew as they change stations. Deck fire lights already cast ordinary shadows but did not request deep hair shadows. The previous material audit found no emissive input in Ruby's desktop hair shader, so this candidate keeps her material, colour, groom and ACE setup unchanged.

The user confirmed the recurrence happens beside a lantern. Final queued candidate: use mask 15 for all four lanterns. The initially proposed fire deep-shadow addition has been removed to keep this correction focused on the reported condition. Lamp brightness, radius, colour, fog scattering, fire lighting and hair materials are unchanged. The previous all-four comparison reduced the reported highlight, but does not prove the cause of this recurrence.

Validation pending: after playtest release, reproduce Ruby beside each relevant lantern and compare existing mask 6 against 15. Inspect skin/ship lighting and measure GPU impact before acceptance; additional shadowed lights can cost GPU time. Existing historical captures under Saved/HairLightingReview are useful references, not acceptance of this new build. Current selected 22:36 playable remains unchanged. Do not claim the recurrence fixed until rendered evidence supports it.
