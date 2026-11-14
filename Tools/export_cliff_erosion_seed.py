"""Export a measured, explicitly approximate XY heightfield from production cliffs.

Uses the existing standalone CPU-probe shim and actual CliffSurfacePoint source.
No Unreal process or GPU is used. MSVC and local engine source are required.
The highest intersection with the production profile triangles wins; overhangs
are reported and retained in raw profiles, not silently claimed to fit a heightfield.
All generated source/data must remain under the selected project's ignored Saved.
"""
import argparse
import hashlib
import importlib
import json
import math
import pathlib
import subprocess
import sys

import numpy as np

sys.dont_write_bytecode = True


EXPORT_MAIN = r"""
int main(){
 ADCMaelstrom m;
 constexpr int Source=SOURCE_VALUE;
 constexpr double Width=WIDTH_VALUE*100.,Cell=CELL_VALUE*100.;
 const double A=m.WaterfallAngle(Source),DA=UE_TWO_PI/DCCliffGenerator::MeshSectors;
 // Conservative first interval; tighten it after measuring actual toes.
 const double Broad=std::atan2(Width*.5,m.Radius*.35)+DA*3;
 int First=int(std::floor((A-Broad)/DA)),Last=int(std::ceil((A+Broad)/DA));
 double MinToe=1e30,MaxLip=0;
 auto angle=[&](int side){int s=(side%DCCliffGenerator::MeshSectors+DCCliffGenerator::MeshSectors)%DCCliffGenerator::MeshSectors;return s*UE_TWO_PI/DCCliffGenerator::MeshSectors;};
 for(int side=First;side<=Last;++side){
  MinToe=std::min(MinToe,m.CliffSurfacePoint(angle(side),0).Size2D());
  MaxLip=std::max(MaxLip,m.CliffSurfacePoint(angle(side),.84f).Size2D());
 }
 double MinR=std::floor((MinToe-2000)/Cell)*Cell;
 const double HalfAngle=std::atan2(Width*.5,MinR)+DA*2;
 First=int(std::floor((A-HalfAngle)/DA));Last=int(std::ceil((A+HalfAngle)/DA));
 MinToe=1e30;MaxLip=0;
 for(int side=First;side<=Last;++side){
  MinToe=std::min(MinToe,m.CliffSurfacePoint(angle(side),0).Size2D());
  MaxLip=std::max(MaxLip,m.CliffSurfacePoint(angle(side),.84f).Size2D());
 }
 // Discard the broad search's excess sea padding. This raises the lower
 // radial bound, so the already exported angular interval remains sufficient.
 MinR=std::floor((MinToe-2000)/Cell)*Cell;
 const double MaxR=std::ceil((MaxLip+20000)/Cell)*Cell;
 const int Columns=int(std::ceil(Width/Cell))+1,Rows=int(std::ceil((MaxR-MinR)/Cell))+1;
 // Clockwise tangent U and outward radial V form a right-handed XY frame.
 const FVector2D U(std::sin(A),-std::cos(A)),V(std::cos(A),std::sin(A));
 const FVector2D Origin=V*MinR-U*((Columns-1)*Cell*.5);
 FILE* raw=std::fopen("raw_profile_grid.bin","wb");
 if(!raw)return 2;
 for(int side=First;side<=Last;++side)for(int row=0;row<=DCCliffGenerator::MeshRows;++row){
  const FVector P=m.CliffSurfacePoint(angle(side),float(row)/DCCliffGenerator::MeshRows);
  const double xyz[]={P.X*.01,P.Y*.01,P.Z*.01};
  if(std::fwrite(xyz,sizeof(double),3,raw)!=3)return 3;
 }
 std::fclose(raw);
 FILE* samples=std::fopen("grid_queries.bin","wb");if(!samples)return 4;
 for(int y=0;y<Rows;++y)for(int x=0;x<Columns;++x){
  const FVector2D P=Origin+U*(x*Cell)+V*(y*Cell);
  const double values[]={P.X*.01,P.Y*.01,m.HeightAt(P,false)*.01,
   (m.WaterEdgeRadius(FMath::Atan2(P.Y,P.X))-P.Size())*.01};
  if(std::fwrite(values,sizeof(double),4,samples)!=4)return 5;
 }
 std::fclose(samples);
 FILE* meta=std::fopen("sampling_frame.json","w");if(!meta)return 6;
 std::fprintf(meta,"{\"source_index\":%d,\"source_angle_rad\":%.17g,\"rows\":%d,\"cols\":%d,\"profile_first_side\":%d,\"profile_last_side\":%d,\"profile_rows\":%d,\"mesh_sectors\":%d,\"cell_size_m\":%.17g,\"arena_radius_m\":%.17g,\"frame_origin_world_m\":[%.17g,%.17g,0],\"basis_u_world\":[%.17g,%.17g,0],\"basis_v_world\":[%.17g,%.17g,0],\"radial_bounds_m\":[%.17g,%.17g]}",
  Source,A,Rows,Columns,First,Last,DCCliffGenerator::MeshRows,DCCliffGenerator::MeshSectors,Cell*.01,m.Radius*.01,
  Origin.X*.01,Origin.Y*.01,U.X,U.Y,V.X,V.Y,MinR*.01,MaxR*.01);
 std::fclose(meta);
 std::printf("Production profiles: %d x %d; XY query grid: %d x %d at %.2fm\n",Last-First+1,DCCliffGenerator::MeshRows+1,Rows,Columns,Cell*.01);
 return 0;
}
"""


def source_snapshot(project, geometry_ref, engine):
    # Load the selected project's current probe helpers without modifying them.
    sys.path.insert(0, str(project / "Tools"))
    probe = importlib.import_module("probe_maelstrom_geometry")
    contacts = importlib.import_module("probe_waterfall_contacts")
    block = probe.block
    paths = ["DCMaelstrom.h", "DCMaelstrom.cpp", "DCMaelstromTerrain.cpp",
             "DCMaelstromBreakers.cpp", "DCMaelstromRoute.h", "DCCliffGenerator.h"]
    text = {name: (project / "Source/DirectorsCut" / name).read_text(encoding="utf-8-sig") for name in paths}
    pieces = []
    def take(name, marker):
        result = block(text[name], marker)
        pieces.append((name, marker, result))
        return result
    code = contacts.make_shim(text["DCMaelstrom.h"])
    local_engine = (engine / "Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp").read_text(encoding="utf-8-sig")
    code += block(local_engine, "namespace FMathPerlinHelpers") + ";\n" + block(local_engine, "float FMath::PerlinNoise2D")
    code += take("DCMaelstromRoute.h", "namespace DCMaelstromRoute")
    if "DCMaelstromBankWaves::" in text["DCMaelstromBreakers.cpp"]:
        name = "DCMaelstromBankWaves.h"
        text[name] = (project / "Source/DirectorsCut" / name).read_text(encoding="utf-8-sig")
        code += block(text[name], "namespace DCMaelstromBankWaves")
    code += take("DCCliffGenerator.h", "namespace DCCliffGenerator")
    code += take("DCMaelstrom.cpp", "float RimVariation")
    for name in ("GetRimRadius", "GetOverflowAmount", "GetRimJoin", "RimElevation", "BaseHeightAt", "HeightAt", "RimGroundHeight"):
        code += take("DCMaelstrom.cpp", "float ADCMaelstrom::" + name)
    for name in ("WaterEdgeRadius", "WaterfallAngle", "WaterfallWidth"):
        code += take("DCMaelstromTerrain.cpp", "float ADCMaelstrom::" + name)
    code += take("DCMaelstromTerrain.cpp", "FVector ADCMaelstrom::CliffSurfacePoint")
    code += block(text["DCMaelstromBreakers.cpp"], "float ADCMaelstrom::BreakerHeight")
    if geometry_ref:
        for name, marker, current in pieces:
            old = subprocess.check_output(["git", "show", f"{geometry_ref}:Source/DirectorsCut/{name}"], cwd=project).decode("utf-8-sig")
            if block(old, marker) != current:
                raise ValueError(f"Position source differs from {geometry_ref}: {name}: {marker}")
    sha = hashlib.sha256("\n".join(part[2] for part in pieces).encode()).hexdigest()
    metadata = {"source_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=project, text=True).strip(),
                "geometry_reference": geometry_ref, "position_functions_sha256": sha,
                "source_file_sha256": {name: hashlib.sha256(value.encode()).hexdigest() for name, value in text.items()}}
    return code, metadata


def intersect_profiles(profiles, queries, frame):
    """Vertical intersections with the actual two triangles per profile quad.

    The radial ray's angle selects one sector. Checking every segment/triangle
    in that sector retains overlapping radial folds, unlike sorting R to force
    a monotone profile. Choosing the highest hit is the explicit approximation.
    """
    shape = queries.shape[:2]
    points = queries[..., :2].reshape(-1, 2)
    source_angle = frame["source_angle_rad"]
    angles = source_angle + (np.arctan2(points[:, 1], points[:, 0])-source_angle+math.pi) % (2*math.pi)-math.pi
    # Use measured profile angles, retaining production float-coordinate
    # rounding rather than reconstructing a subtly different double grid.
    profile_angles = source_angle + (np.arctan2(profiles[:, 0, 1], profiles[:, 0, 0])-source_angle+math.pi) % (2*math.pi)-math.pi
    sectors = np.searchsorted(profile_angles, angles, side="right")-1
    if sectors.min() < 0 or sectors.max() >= len(profiles)-1:
        raise ValueError("Query grid escapes the exported profile interval")
    height = np.empty(len(points), dtype=np.float64)
    selected_v = np.zeros(len(points), dtype=np.float64)
    counts = np.zeros(len(points), dtype=np.int16)
    span = np.zeros(len(points), dtype=np.float64)
    degenerate = 0
    vv = np.arange(profiles.shape[1], dtype=np.float64)/(profiles.shape[1]-1)
    for sector in np.unique(sectors):
        ids = np.flatnonzero(sectors == sector)
        a, b, c, d = profiles[sector, :-1], profiles[sector+1, :-1], profiles[sector, 1:], profiles[sector+1, 1:]
        triangles = np.stack((np.stack((a, b, c), axis=1), np.stack((b, d, c), axis=1)), axis=1).reshape(-1, 3, 3)
        param = np.stack((np.stack((vv[:-1], vv[:-1], vv[1:]), axis=1),
                          np.stack((vv[:-1], vv[1:], vv[1:]), axis=1)), axis=1).reshape(-1, 3)
        origin = triangles[:, 0]
        ab, ac = triangles[:, 1]-origin, triangles[:, 2]-origin
        denom = ab[:, 0]*ac[:, 1]-ab[:, 1]*ac[:, 0]
        usable = np.abs(denom) > 1e-9
        degenerate += int((~usable).sum())
        denominator = np.where(usable, denom, 1)
        for start in range(0, len(ids), 256):
            batch = ids[start:start+256]
            delta = points[batch, None, :] - origin[None, :, :2]
            u = (delta[..., 0]*ac[None, :, 1]-delta[..., 1]*ac[None, :, 0])/denominator
            v = (ab[None, :, 0]*delta[..., 1]-ab[None, :, 1]*delta[..., 0])/denominator
            hit = usable & (u >= -1e-8) & (v >= -1e-8) & (u+v <= 1+1e-8)
            z = origin[None, :, 2]+u*ab[None, :, 2]+v*ac[None, :, 2]
            ranked = np.where(hit, z, -np.inf)
            chosen = ranked.argmax(axis=1)
            height[batch] = ranked[np.arange(len(batch)), chosen]
            selected_v[batch] = param[chosen, 0]+u[np.arange(len(batch)), chosen]*(param[chosen, 1]-param[chosen, 0])+v[np.arange(len(batch)), chosen]*(param[chosen, 2]-param[chosen, 0])
            ordered = np.sort(np.where(hit, z, np.inf), axis=1)
            with np.errstate(invalid="ignore"):
                distinct = np.concatenate((np.ones((len(batch), 1), dtype=bool), np.diff(ordered, axis=1) > .001), axis=1)
            counts[batch] = (distinct & np.isfinite(ordered)).sum(axis=1)
            valid = counts[batch] > 0
            span[batch[valid]] = height[batch[valid]] - ordered[valid, 0]
            # Out-of-surface padding is fixed and explicitly marked invalid.
            missing = batch[~valid]
            for index in missing:
                alpha = (angles[index]-profile_angles[sector])/(profile_angles[sector+1]-profile_angles[sector])
                profile = profiles[sector]*(1-alpha)+profiles[sector+1]*alpha
                nearest = np.abs(np.linalg.norm(profile[:, :2], axis=1)-np.linalg.norm(points[index])).argmin()
                height[index] = profile[nearest, 2]
                selected_v[index] = vv[nearest]
    height = height.reshape(shape)
    missing = counts.reshape(shape) == 0
    sea_padding = missing & (queries[..., 3] >= 0)
    height[sea_padding] = queries[..., 2][sea_padding]
    selected_v = selected_v.reshape(shape)
    selected_v[sea_padding] = 0
    return height, selected_v, counts.reshape(shape), span.reshape(shape), degenerate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--engine", type=pathlib.Path, default=pathlib.Path("E:/Utilities/UE_5.8"))
    parser.add_argument("--vcvars", type=pathlib.Path, default=pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Auxiliary/Build/vcvars64.bat"))
    parser.add_argument("--source", type=int, default=8)
    parser.add_argument("--width-m", type=float, default=500)
    parser.add_argument("--cell-size-m", type=float, default=2)
    parser.add_argument("--geometry-ref", help="Require identical position-generating functions to this Git revision; normal-only changes are allowed")
    parser.add_argument("--output", type=pathlib.Path, help="Output directory inside the selected project's Saved directory")
    args = parser.parse_args()
    if not 0 <= args.source <= 8 or not math.isfinite(args.width_m) or not 100 <= args.width_m <= 1200:
        parser.error("Source must be0..8 and sector width100..1200 metres")
    if not math.isfinite(args.cell_size_m) or not .5 <= args.cell_size_m <= 10:
        parser.error("Cell size must be0.5..10 metres")
    project = args.project_root.resolve()
    out = (args.output or project / "Saved/CliffErosion" / f"Source{args.source:02d}-Seed").resolve()
    if not out.is_relative_to((project / "Saved").resolve()):
        parser.error("All generated files must stay inside the selected project's Saved directory")
    out.mkdir(parents=True, exist_ok=True)
    code, provenance = source_snapshot(project, args.geometry_ref, args.engine)
    code += EXPORT_MAIN.replace("SOURCE_VALUE", str(args.source)).replace("WIDTH_VALUE", repr(args.width_m)).replace("CELL_VALUE", repr(args.cell_size_m))
    cpp, exe = out / "export_profiles.cpp", out / "export_profiles.exe"
    cpp.write_text(code, encoding="utf-8")
    batch = out / "build_export.cmd"
    batch.write_text(f'@echo off\ncall "{args.vcvars}" >nul\nif errorlevel 1 exit /b %errorlevel%\n'
                     f'cl /nologo /std:c++17 /EHsc /O2 "{cpp}" /Fe:"{exe}" /Fo:"{out / "export_profiles.obj"}"\n', encoding="utf-8")
    subprocess.run(["cmd", "/d", "/c", str(batch)], cwd=out, check=True)
    subprocess.run([str(exe)], cwd=out, check=True)
    frame = json.loads((out / "sampling_frame.json").read_text())
    profile_shape = (frame["profile_last_side"]-frame["profile_first_side"]+1, frame["profile_rows"]+1, 3)
    profiles = np.fromfile(out / "raw_profile_grid.bin", dtype="<f8").reshape(profile_shape)
    queries = np.fromfile(out / "grid_queries.bin", dtype="<f8").reshape(frame["rows"], frame["cols"], 4)
    height, selected_v, counts, span, degenerate = intersect_profiles(profiles, queries, frame)
    cell = frame["cell_size_m"]
    row, col = np.indices(height.shape)
    edge_distance = np.minimum.reduce((row, col, height.shape[0]-1-row, height.shape[1]-1-col))*cell
    valid = counts > 0
    protected = (~valid) | (edge_distance <= 12) | (queries[..., 3] >= -35) | (selected_v >= .80)
    dy, dx = np.gradient(height, cell)
    slope = np.hypot(dx, dy)
    taper = np.minimum.reduce((np.clip((edge_distance-12)/20, 0, 1),
                               np.clip((-queries[..., 3]-35)/25, 0, 1), np.clip((.80-selected_v)/.06, 0, 1)))
    erodibility = (.30+.70*slope/(.5+slope))*taper*(~protected)
    erosion_depth = 8+7*erodibility
    bedrock = height-erosion_depth
    metrics = {"grid_shape": list(height.shape), "grid_cells": int(height.size), "valid_surface_rays": int(valid.sum()),
               "fixed_padding_rays": int((~valid).sum()), "multivalued_rays": int((counts > 1).sum()),
               "multivalued_fraction_of_valid": float((counts > 1).sum()/max(1, valid.sum())),
               "max_distinct_intersections": int(counts.max()), "maximum_discarded_vertical_span_m": float(span.max()),
               "degenerate_xy_projection_triangles": degenerate, "protected_cells": int(protected.sum()),
               "active_erosion_cells": int((erodibility > 0).sum()), "height_min_m": float(height.min()),
               "height_max_m": float(height.max()), "erosion_depth_min_m": float(erosion_depth.min()),
               "erosion_depth_max_m": float(erosion_depth.max()), "all_finite": bool(np.isfinite(height).all() and np.isfinite(profiles).all())}
    manifest = {"schema": "directorscut.cliff_erosion_seed.v1", "approximation": "Uppermost vertical intersection with production radial-profile triangles. Overhangs and lower surfaces are discarded in height_m, retained in raw_profile_xyz_world_m.",
                "coordinate_frame": "Design-world XY equals maelstrom arena-local at identity, before the moving ship-centric arena pose. Height is absolute Z in metres.",
                "array_layout": "[outward-radial rows, clockwise-tangential columns]; frame origin is sample[0,0], Z=0; right-handed basis",
                "protection": "Fixed12m tile boundary, water and first35m dry shore, and V>=0.80 plateau/temple-support shoulder. Out-of-surface padding is fixed.",
                "frame": frame, "provenance": provenance, "metrics": metrics}
    arrays = {"height_m": height.astype(np.float32), "bedrock_m": bedrock.astype(np.float32),
              "erodibility": erodibility.astype(np.float32), "protected_mask": protected.astype(np.float32),
              "loose_m": (.5*erodibility).astype(np.float32), "cell_size_m": np.float64(cell),
              "frame_origin_world_m": np.asarray(frame["frame_origin_world_m"], dtype=np.float64),
              "basis_u_world": np.asarray(frame["basis_u_world"], dtype=np.float64),
              "basis_v_world": np.asarray(frame["basis_v_world"], dtype=np.float64), "basis_up_world": np.array((0., 0., 1.)),
              "world_xy_m": queries[..., :2], "u_m": np.arange(height.shape[1])*cell, "v_m": np.arange(height.shape[0])*cell,
              "raw_profile_xyz_world_m": profiles, "raw_profile_v": np.linspace(0, 1, profile_shape[1]),
              "raw_profile_angles_rad": frame["source_angle_rad"]+(np.arctan2(profiles[:, 0, 1], profiles[:, 0, 0])-frame["source_angle_rad"]+math.pi) % (2*math.pi)-math.pi,
              "surface_parameter_v": selected_v.astype(np.float32), "heightfield_valid_mask": valid.astype(np.uint8),
              "intersection_count": counts, "discarded_vertical_span_m": span.astype(np.float32),
              "shore_clearance_m": queries[..., 3].astype(np.float32), "metadata_json": np.asarray(json.dumps(manifest)),
              "seed_metrics_json": np.asarray(json.dumps(metrics))}
    for name, value in arrays.items():
        if np.issubdtype(value.dtype, np.number) and not np.isfinite(value).all():
            raise ValueError("Nonfinite exported array: " + name)
    if np.any(arrays["bedrock_m"] > arrays["height_m"]) or not metrics["active_erosion_cells"]:
        raise ValueError("Invalid bedrock bounds or no unprotected cliff surface")
    np.savez_compressed(out / "cliff_erosion_seed.npz", **arrays)
    (out / "cliff_erosion_seed.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({"output": str(out / "cliff_erosion_seed.npz"), **metrics}, indent=2))


if __name__ == "__main__":
    main()
