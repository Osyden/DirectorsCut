"""Serialize the original cliff FBX authoring triangles for Arena-cm queries.

This consumes the SAME seed/result NPZ as export_eroded_cliff_fbx.py and imports
its shared triangle builder. It never reads Nanite or any simplified fallback.
JSON arrays are compact original vertices, UVs, triangles and a grid-to-vertex
index (-1 absent); no interpolation over missing boundary cells is implied.
The C++ DCCliffQueryMesh.h consumes parsed arrays, with no JSON/runtime coupling.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import numpy as np

sys.dont_write_bytecode = True
from review_eroded_cliff_sector import load_pair, source_info, triangles


SCHEMA = "directorscut.cliff_query_mesh.v1"


def build_mesh(seed, result):
    if not source_info(seed)["clean"]:
        raise ValueError("Only an original clean authoring heightfield is supported")
    height = np.asarray(result["height_m"], dtype=np.float64)
    valid = np.asarray(seed["heightfield_valid_mask"]) > 0
    rows, cols = height.shape
    cell = float(seed["cell_size_m"])
    origin = np.asarray(seed["frame_origin_world_m"], np.float64)
    u, v = (np.asarray(seed[k], np.float64) for k in ("basis_u_world", "basis_v_world"))
    if not all(np.isfinite(a).all() for a in (height, origin, u, v)) or not np.isfinite(cell) or cell <= 0:
        raise ValueError("Non-finite grid/frame or non-positive cell size")
    if origin.shape != (3,) or u.shape != (3,) or v.shape != (3,):
        raise ValueError("Expected 3D frame vectors")
    if abs(u[2])+abs(v[2]) > 1.e-9 or not np.allclose([u@u, v@v, u@v], [1, 1, 0], rtol=0, atol=1.e-9) or np.cross(u, v)[2] < 0:
        raise ValueError("Only a right-handed orthonormal XY grid frame is supported")
    columns, lines = np.meshgrid(np.arange(cols)*cell, np.arange(rows)*cell)
    world = origin[None, None, :]+columns[..., None]*u+lines[..., None]*v
    if not np.allclose(world[..., :2], seed["world_xy_m"], rtol=0, atol=1.e-6):
        raise ValueError("Recorded world XY is inconsistent with the FBX grid frame")
    world[..., 2] = height # height is absolute arena Z, not height above frame origin
    faces = triangles(valid)
    if not len(faces):
        raise ValueError("No physical triangles to export")
    used = np.unique(faces)
    remap = np.full(rows*cols, -1, dtype=np.int32)
    remap[used] = np.arange(len(used), dtype=np.int32)
    positions = np.ascontiguousarray(world.reshape(-1, 3)[used]*100, dtype="<f8")
    indices = np.ascontiguousarray(remap[faces], dtype="<u4")
    # FBX UV0 is centered local physical XY metres /100, not arena XY.
    local = np.stack((columns-(cols-1)*cell*.5, lines-(rows-1)*cell*.5), axis=-1)
    uv = np.ascontiguousarray(local.reshape(-1, 2)[used]/100, dtype="<f4")
    arrays = {"vertices_arena_cm": positions, "triangles": indices,
              "uv0": uv, "grid_to_vertex": remap.astype("<i4")}
    digest = hashlib.sha256()
    for value in arrays.values():
        digest.update(value.tobytes(order="C"))
    centre = origin+u*(cols-1)*cell*.5+v*(rows-1)*cell*.5
    centre[2] = float(np.median(height[valid]))
    payload = {
        "schema": SCHEMA, "version": 1, "units": "arena centimetres before ship-centred pose",
        "surface": "original authoring triangles; single-valued XY; no overhang support",
        "rows": rows, "columns": cols, "cell_size_cm": cell*100,
        "frame_origin_arena_cm": (origin*100).tolist(), "basis_u_arena": u.tolist(), "basis_v_arena": v.tolist(),
        "fbx_local_origin_arena_cm": (centre*100).tolist(),
        "triangle_diagonal": "a-d: (a,b,d), (a,d,c); a=(row,col), b=(row,col+1), c=(row+1,col), d=(row+1,col+1)",
        "vertices": len(used), "triangle_count": len(faces),
        "geometry_sha256": digest.hexdigest(),
        "geometry_hash_encoding": "concatenate row-major little-endian vertices float64[3], triangles uint32[3], UV float32[2], grid map int32; no headers",
        "array_sha256": {k: hashlib.sha256(a.tobytes(order="C")).hexdigest() for k, a in arrays.items()},
        "array_shapes": {k: list(a.shape) for k, a in arrays.items()},
        "source_kind": source_info(seed)["kind"],
        "bounds_arena_cm": [positions.min(axis=0).tolist(), positions.max(axis=0).tolist()],
        "uv0_convention": "FBX physical local XY metres /100; centred on tile",
        **{k: value.ravel().tolist() for k, value in arrays.items()},
    }
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", required=True, type=Path)
    parser.add_argument("--eroded", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--fbx-metadata", type=Path, help="Verify the original FBX sidecar has matching source/frame/counts")
    args = parser.parse_args()
    output = args.output.resolve()
    if output.suffix.lower() != ".json" or "saved" not in [p.lower() for p in output.parts]:
        parser.error("Output must be JSON under ignored Saved")
    seed, result = load_pair(args.seed, args.eroded)
    payload = build_mesh(seed, result)
    payload["provenance"] = {
        "seed_path": str(args.seed.resolve()), "seed_sha256": hashlib.sha256(args.seed.read_bytes()).hexdigest(),
        "result_path": str(args.eroded.resolve()) if args.eroded else None,
        "result_sha256": hashlib.sha256(args.eroded.read_bytes()).hexdigest() if args.eroded else None,
        "generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    }
    if args.fbx_metadata:
        fbx = json.loads(args.fbx_metadata.read_text(encoding="utf-8"))
        for key in ("seed_sha256", "result_sha256"):
            if fbx[key] != payload["provenance"][key]:
                raise ValueError("FBX and query use different inputs: "+key)
        if fbx["vertices"] != payload["vertices"] or fbx["triangles"] != payload["triangle_count"]:
            raise ValueError("FBX and query topology counts differ")
        if not np.allclose(np.asarray(fbx["local_origin_arena_m"])*100, payload["fbx_local_origin_arena_cm"], rtol=0, atol=1.e-7):
            raise ValueError("FBX and query local-to-arena origins differ")
        payload["provenance"]["fbx_metadata_sha256"] = hashlib.sha256(args.fbx_metadata.read_bytes()).hexdigest()
        payload["provenance"]["fbx_sha256"] = fbx["fbx_sha256"]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, separators=(",", ":"), allow_nan=False)+"\n", encoding="utf-8")
    print(json.dumps({k: payload[k] for k in ("schema", "vertices", "triangle_count", "geometry_sha256")}, indent=2))
    print(f"Saved {output} ({output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
