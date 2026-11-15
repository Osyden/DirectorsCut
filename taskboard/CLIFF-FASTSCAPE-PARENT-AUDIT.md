# Parent landform: independent constraint and geometry audit

15 November 2026. Read-only CPU audit in `codex/cliff-block-adapter-audit`, after merging root `e39928c`. Existing mesh helpers and bounded API checks only; no new audit framework, Unreal build, import or runtime changes.

**ParentA passes its declared coastal geometry contract but is visually rejected.** It is neither a drop-in sector replacement nor approved support for existing temples.

## Source, size and provenance

Actual artifacts are `Flow-Seams/Saved/FastscapeParent/ParentA`. Parent candidate SHA256: `cd763f05e964f6cd476f84bf11afc33ba94b337ed98e70d09138dc8e64240492`; crop candidate: `c4586b8da370377bf79d8cf1bcf2bb627381870530a38ba6545519178b266b59`.

The immutable input is `source08_measured_boundary_massif_v3_seed.npz`, SHA256 `2f45a1d758d298066e6ec5fa4580ecd0b454e1ef8ae6266abb318411c007ca6f`. Measured coast targets come from `Maelstrom-Saturday/Saved/CliffErosion/CurrentSource08/cliff_erosion_seed.npz`, SHA256 `8564969f5447949c72c598a9640fd5d86a20a99744bd0080c492711f38748200`. Both identities and the stored generator hash match the actual files.

The parent is 551 x 251 samples at 2 m spacing: 500 x 1100 m in the source frame. Its valid land spans 500 x 1088 m; the difference is original excluded water padding. The crop is exactly parent rows `[0,253)` and columns `[0,251)`, preserving the original 500 x 504 m array and 500 x 492 m valid footprint. Source XY and basis are exact; reconstructed parent world XY differs only by floating-point rounding below 4.6e-13 m. There is no mesh rescaling or vertical exaggeration.

Rows 253 through 550 contain 74,798 explicitly authored inland samples. This extends the domain inland only, with no lateral halo. Their elevations, coastal continuation and neighbouring arena support are not measured or certified. Closed lateral model boundaries can still constrain the catchment layout.

An independent replay poisoned every unconstrained old source height by +12,345 m and every unconstrained measured height by -23,456 m. New raw heights, initial heights, uplift and target arrays remained bitwise identical. The rejected old free interior is not reused.

## Exact coast, released seams and route limits

The original coastal mask plus one-cell collar freezes 1,311 valid vertices. All 2,000 original triangles touching coastal constraints retain their measured positions exactly, including the whole contact strip rather than only selected vertex samples. All 251 original outlets and the complete crop valid-domain mask remain exact. Inspection of actual final routing confirms those 251 outlets are the only base levels and finds zero valid-to-invalid receiver links.

This preserves the local land footprint and original shore contact geometry next to the route. It does not certify the complete ship route, the wider authored parent against neighbouring arena geometry, or any current waterfall contact above the fixed strip.

Old arbitrary crop seams are deliberately released: 678 of 992 external edges change in 3D. The 677 changed boundary vertices move vertically by -152.56 to +13.44 m relative to the measured source. This is an explicit integration requirement, not a seam pass. Adjacent accepted sectors must eventually be sampled from the same authoritative parent surface.

## Mesh and process checks

The parent has 127,334 vertices / 253,078 triangles and the crop 52,536 / 104,078. Each is one clean open manifold, with no degenerate or duplicate triangles, inconsistent winding, nonmanifold edges or unused vertices. Parent/crop have 1,588 / 992 external edges respectively. These are original physical heightfield triangles, with no overhang claim or smoothing added by the audit.

The process uses authored unequal uplift corridors, Fastscapelib stream-power incision and the previously checked valid-edge diffusion helper. No native ADI is used. Over 100 steps of 100 model years, active terrain receives 31,899,663.19 m³ of uplift, loses 2,033,572.15 m³ through stream-power removal and exchanges -67,437.24 m³ with fixed-height reservoirs. Net change is 29,798,653.81 m³; the independent combined ledger closes within 3.0e-9 m³. The per-node update identity closes within 1.5e-12 m. This is height-volume bookkeeping, not a transported sediment model or calibrated geological timescale.

## Visual and foundation rejection

I inspected the actual `ParentAWhole/WHOLE PARENT 500x1100m oblique_fractured.png` and `ParentACrop/wall_fractured.png` under the review worker's `Saved/ImplicitCliffReview/FastscapeParent`. Unequal crests exist, but the parent remains a long smooth raised ridge; the crop reads as two rounded hill lobes with many parallel shallow flutes. Broad uplift corridors dominate the shape rather than a convincing branching valley/ridge hierarchy. A correct wider domain and fixed coastline have not produced the required primary cliff structure. No native promotion follows this result.

Existing foundations are explicitly unsupported. Sampled terrain under Actors 33 and 34 is 63.5-96.1 m and 82.9-95.3 m below their old support levels. Actor 36 loses 17.6-31.8 m; Actor 48 ranges from 12.4 m below to 0.5 m above. Full footing coverage has not been certified. Old temple actors remain unchanged in the game.

Full numeric evidence is retained in the audit worker's ignored `Saved/WeatheredAudit/fastscape_parent_a.json`. No material, cloud, water, boss, gameplay or shipped scene change is implied.
