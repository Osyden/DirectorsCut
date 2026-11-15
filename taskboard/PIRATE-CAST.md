# Purchased pirate cast

Owner: Codex submission chat, `codex/ace-comparison`; selectively installed in main on 15 November 2026.

The purchased MHC Pirates bundle is rigged, textured, assembled and bound to the playable cast.

| Role | Preset | Outfit | Headwear |
| --- | --- | --- | --- |
| Player captain | Breeza | Sovereign Corsair | Female pirate hat |
| Rex | Davy | Dread Captain | Male pirate hat |
| Duncan | Davy | Tide Duelist | Male headband |
| Ruby | Breeza | Rogue Swashbuckler | Female headband |

Names were updated on 15 November; internal asset paths retain the original role names. See [crew HUD and speech hints](CREW-UI.md).

Editable characters: `/Game/PirateCast/Presets/{Captain,Finch,Marlow,Odessa}`. Actors: `/Game/PirateCast/Assembled/<Role>/BP_<Role>`. Original wardrobe accessories are retained. Duncan uses the bundle's male hat hairstyle; it contains no separate male headband hairstyle.

## Integration

`DCPirateCast` is a separate world subsystem. It attaches the configured MetaHuman to each existing crew/player pose driver. DCCrew and DCPlayer gameplay were not edited. The captain uses the existing Quinn idle/walk animations; crew retain their animation state transitions and orders.

The full captain is visible to external cameras. Her outfit is hidden for her first-person camera, with hidden shadows retained, to prevent hat, shoulder and coat clipping. This is a full-body external appearance, not separate first-person arms.

`DefaultGame.ini` enables the subsystem and cooks the assembled directory. Set `bEnabled=False` under `/Script/DirectorsCut.DCPirateCastSettings` to restore previous appearances. DLSS/Streamline remain enabled in main. Update 15 Nov: the three crew now have local GPU ACE facial animation connected to gameplay speech; see [integration and tests](ACE-INTEGRATION.md).

## Setup repairs

- Selected UE 5.8 as Fab export target and downloaded the purchased 2,420,087,249-byte package.
- Installed MetaHuman Creator Core Data. Epic authorization, all four rigs and texture requests completed.
- Enabled virtual textures, 16-bit bone indices, unlimited bone influences and experimental skeletal-mesh chunking.
- Assembled UE Optimized / Medium bodies, faces, clothing and textures in the full editor. TextureGraph does not initialize in the Python commandlet; assembly requires `-ExecutePythonScript` in a full editor process.
- UE 5.8 Medium groom optimization reduces strands to one curve and expects cards. These hairstyles have no cards. `restore_pirate_strands.py` duplicates the untouched vendor grooms and creates bindings to each assembled face. The subsystem assigns these full-strand grooms after Blueprint construction and uses hair LODs 1/3/5.
- Generated Blueprint construction can overwrite hair template edits. Runtime assignment is intentional. For independent use of a generated Blueprint, assign its `Gr_Hair_Strands` and `GB_Hair_Strands` after construction, or use the subsystem. Editable Creator presets retain their original vendor hair.
- The deprecated `ChaosClothAssetEditor` wrapper caused a USD startup crash during setup. It is not enabled; supported editor-core/dataflow plugins are used.

## Verification

- Main editor and Windows Development game targets build successfully.
- Four actor classes load with body/face meshes; final runtime dependency validation passes without missing game packages. Restored strand groups contain 5,269–14,302 curves each.
- All four outfits, individual characters, forward/down first-person views and moving crew were captured in main: `Saved/PirateCast/FinalReview/`.
- Actual ship scene binds all four actors and retains station orders. Evidence: `Saved/PirateCast/ShipReview.log` and `Saved/Screenshots/Auto/shot_00173.png`. That short offscreen capture reported 114 fps on the owner's machine; it is not a benchmark.
- Focused Windows asset cook passed: 857 packages cooked, 7 platform-skipped, zero errors and warnings. The cooked registry contains all four named Blueprints and restored groom/binding assets. Evidence: `Saved/PirateCast/Cook3.log`; packages were explicitly selected with `-PACKAGE=...`. The final jam package and clean-machine playtest remain separate release gates.
- Editor `-game` runs log startup errors from existing experimental EditorToolset/ToolsetRegistry Python scripts (`AgentSkill` / `PythonTestRunner` unavailable in game mode). They do not prevent the game review. Final asset-validation commandlet reports zero errors and warnings.

## Reproduction and asset handling

Tools are in ACE-Comparison's `Tools/ACE/`: prepare, assemble, restore_pirate_strands, validate_pirate_cast, and the full-editor wrapper build_pirate_cast.

`install_pirate_assets.ps1 -TargetProject <directory>` copies and SHA-256 verifies files. `-UpdateVerifiedAssets` only replaces a target matching the previous installation hash and backs it up first. Main has 640 purchased/generated files. Private manifest: `Saved/PirateCast/installed-assets.json`.

Both `/Content/PirateCastVendor/` and `/Content/PirateCast/` are ignored by Git. Do not publish source assets. Declare Quang Phan's bundle, Epic MetaHuman and the mannequin animations in the jam submission. Include this change in the next full release package and perform its release playtest.

Source: [MHC: Pirates | MetaHuman Mega Bundle by Quang Phan](https://www.fab.com/listings/cd3450e5-b15d-4bc4-8148-67380d056f34). APIs were checked against installed Epic UE 5.8 source and examples.
