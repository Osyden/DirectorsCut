"""Read-only inventory of installed waterfall/whitewater VDB assets.

Run in Unreal Python after installing a pack. This does not spawn actors,
change vendor assets, or select a material on guessed parameter names.
Writes only Saved/WaterfallPackAudit.json. Optional script arguments:
  --root /Game/ExactPackFolder --load-limit 3

Without --root, candidate package paths must contain waterfall, whitewater
or splash. Cloud-tunnel SVTs are deliberately excluded from implicit loading.
"""
import argparse
import json
from pathlib import Path
import unreal


parser = argparse.ArgumentParser()
parser.add_argument("--root", default="/Game")
parser.add_argument("--load-limit", type=int, default=3)
args, _ = parser.parse_known_args()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous([args.root], force_rescan=False)
assets = registry.get_assets_by_path(args.root, recursive=True)
content = Path(unreal.Paths.project_content_dir())
records = []


def call(asset, name):
    try:
        return getattr(asset, name)()
    except Exception as exc:
        return {"unavailable": str(exc)}


for data in assets:
    package = str(data.package_name)
    if args.root == "/Game" and not any(word in package.lower() for word in ("waterfall", "whitewater", "splash")):
        continue
    class_name = str(data.asset_class_path.asset_name)
    file = content / (package.removeprefix("/Game/") + ".uasset")
    records.append({
        "package": package,
        "class": class_name,
        "uasset_bytes": file.stat().st_size if file.is_file() else None,
        "sidecar_bytes": sum(p.stat().st_size for p in file.parent.glob(file.stem + ".*")) if file.is_file() else None,
    })

volumes = [item for item in records if "SparseVolumeTexture" in item["class"]]
# Inspect the smallest sequences first to avoid loading an entire multigigabyte
# pack simply to find out what it contains. Remaining paths stay in the report.
volumes.sort(key=lambda item: item["sidecar_bytes"] or 0)
for item in volumes[:max(0, args.load_limit)]:
    asset = unreal.load_asset(item["package"])
    if not asset:
        item["load_error"] = True
        continue
    item["frames"] = call(asset, "get_num_frames")
    item["resolution"] = [call(asset, "get_size_x"), call(asset, "get_size_y"), call(asset, "get_size_z")]
    item["mip_levels"] = call(asset, "get_num_mip_levels")
    item["frame_transform"] = str(call(asset, "get_frame_transform"))

report = {
    "root": args.root,
    "asset_count": len(records),
    "svt_count": len(volumes),
    "svt_files_bytes": sum(item["sidecar_bytes"] or 0 for item in volumes),
    "note": "File bytes are disk footprint, not resident GPU memory or streaming bandwidth. No actors/assets modified.",
    "assets": sorted(records, key=lambda item: item["package"]),
}
destination = Path(unreal.Paths.project_saved_dir()) / "WaterfallPackAudit.json"
destination.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log_warning("DC_WATERFALL_AUDIT %d candidates, %d SVTs: %s" % (len(records), len(volumes), destination))
