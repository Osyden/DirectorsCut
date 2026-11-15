# Credits fit on one page

Root owns codex/credits-one-page and the credits-only change in DCMenu.cpp. The user reported the 22:36 build clipping Audio & Type below a scrollbar. After the source fix was prepared, the user explicitly replied: "Still testing—queue this fix". Do not launch an editor, build, packaging or visual review until the playtest is released. The selected 22:36 package stays unchanged.

The correction removes the credits scroll box, reduces heading/AI/thanks sizes and section gaps, and supplies explicit text wrapping widths. A top-aligned, down-only fit contains the entire measured page if its desired height exceeds the available panel. As requested, Art & Asset Packs and Thanks To entries use the same size 11 muted blue-grey font as Audio & Type. Gold section labels remain. Every acknowledgement is retained. Help continues to scroll independently.

Validation so far: source diff/whitespace and installed Slate API checks only. No compile or rendered-layout acceptance yet. Once released, compile and inspect the packaged credits at 1600x900 and 1280x720: all text including the final Cinzel licence must be visible without a scrollbar, with comfortable bottom padding. Check menu navigation and Start, then stage a fresh candidate without altering accepted builds.

User subsequently released testing and asked to fix credits/lantern now. Compilation passed. Native captures in Saved/CreditsLantern/Native/Credits900 and Credits720 confirm every credit through the final Cinzel licence fits without scrolling at 1600x900 and 1280x720, with bottom padding. Art and Thanks use the requested small muted blue-grey text. Both runs start a fresh voyage and exit 0. Packaged confirmation follows the combined fixes.
