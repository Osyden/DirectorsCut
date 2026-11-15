# AI helm wheel follow-up

Direct user feedback in the coordinated trailer chat: the helmsman should visibly turn the wheel while steering. Current player-only TickHelm owns the wheel mesh animation, so AI SetSteer changes ship movement without rotating that mesh.

Worker tunnel_boss prepared isolated commit70a2ec5 in E:/Projects/DirectorsCut/AI-Wheel, based on2a757bc. Only DCPlayer.cpp/.h change: a common TickWheelVisual reads shared Treadmill steering before the typing early return; authored rest/hub pivot is cached once per ship. Player station transitions no longer recapture/reset the wheel. Existing pivot compensation, steering sign,5/s interpolation and EndPlay restoration remain.

Ten source/lifecycle/pivot checks and24 existing cannon-water/wheel CPU checks pass. No native test or integration into main yet. Keep outside the frozen22:11 package. Next free GPU slot after human testing: compile in a coordinated checkout, review AI steering with no DCStation and DCHelmLog, player takeover/release, typed orders and restart. Require matching input/angle, near-zero hubDrift and smooth settling. Integrate only after native acceptance.
