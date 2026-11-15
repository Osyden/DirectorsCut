#!/usr/bin/env python3
"""Reproduce a physical-unit, image-free implicit-block proof in ignored Saved.

Requires the inspected MIT reference checkout at exactly UPSTREAM_REV. It reads
committed upstream files (never its scratch modifications), writes a fresh local
build, and invokes installed MSBuild. No download, installation or Unreal work.

Algorithm attribution: Paris, Peytavie, Guerin, Dischler and Galin (2020),
Modeling Rocky Scenery using Implicit Blocks, TVC 36(10), 2251-2261.
https://github.com/aparis69/Rock-fracturing
The upstream MIT and bundled-header notices are copied with the ignored build.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

sys.dont_write_bytecode = True
UPSTREAM_REV = "b91965b3011ed269cbc3a051b00c9b284aaa2e36"
DEFAULT_MSBUILD = r"C:/Program Files/Microsoft Visual Studio/2022/Professional/MSBuild/Current/Bin/MSBuild.exe"


# This harness calls the published implementation; it does not reproduce its
# clustering/convex-hull/SDF algorithms. Source-space is Y-up. The proper rotation
# (x,y,z)->(x,-z,y) has determinant +1 and produces a Z-up metre mesh.
HARNESS = r'''
#include "blocks.h"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <iostream>

constexpr double Scale = @SCALE@;
constexpr int Resolution = @RESOLUTION@;
constexpr bool Connected = @CONNECTED@;

static Vector3 ToMetres(const Vector3& p) { return Vector3(p.x, -p.z, p.y) * Scale; }
static void Vec(std::ostream& s, const Vector3& p) { s << '[' << p.x << ',' << p.y << ',' << p.z << ']'; }

static FractureSet SparseFractures(const Box& tile) {
    // Upstream Sphere::RandomSurface is a zero-vector stub. Explicit isotropic
    // unit normals, finite nonperiodic discs and bounded count make this a
    // documented derived experiment with genuinely active fractures.
    FractureSet result;
    for(int i=0;i<12;++i) {
        const double y=Random::Uniform(-1.0,1.0);
        const double angle=Random::Uniform(0.0,6.283185307179586);
        const double radial=std::sqrt(std::max(0.0,1.0-y*y));
        const Vector3 n(radial*std::cos(angle),y,radial*std::sin(angle));
        result.fractures.push_back(Circle(tile.RandomInside(),n,Random::Uniform(4.0,9.0)));
    }
    return result;
}

// A single continuous backing. This is an offline exposed-face proof, not a
// claim to preserve arena boundaries or temple support.
struct BackedFace : public SDFNode {
    SDFNode* blocks;
    Box backing;
    BackedFace(SDFNode* child) : SDFNode(Box(child->box,
        Box(Vector3(-10.1,-10.1,1.0),Vector3(10.1,10.1,4.0)))),
        blocks(child), backing(Vector3(-10.1,-10.1,1.0),Vector3(10.1,10.1,4.0)) {}
    double Signed(const Vector3& p) const override {
        const Vector3 c = backing.Center();
        const Vector3 h = backing.Size() * .5;
        const Vector3 q(std::abs(p.x-c.x)-h.x,std::abs(p.y-c.y)-h.y,std::abs(p.z-c.z)-h.z);
        const Vector3 outside(std::max(q.x,0.0),std::max(q.y,0.0),std::max(q.z,0.0));
        const double d = Magnitude(outside) + std::min(std::max(q.x,std::max(q.y,q.z)),0.0);
        return std::min(d, blocks->Signed(p));
    }
};

int main() {
    srand(@SEED@);
    const Box tile = Connected ? Box(Vector3(-10,-10,-2),Vector3(10,10,2)) : Box(Vector3(0),10.0);
    PointSet3 points = PoissonSamplingBox(tile,0.5,10000);
    FractureSet fractures = SparseFractures(tile);
    size_t adjacency=0,rejected=0;
    for(int i=0;i<points.Size();++i)for(int j=i+1;j<points.Size();++j) {
        const Vector3 delta=points.At(j)-points.At(i);const double length=Magnitude(delta);
        if(length>2.5 || length<1e-10)continue;
        ++adjacency;const Ray ray(points.At(i),delta/length);
        for(int k=0;k<fractures.Size();++k) {double t;
            if(fractures.At(k).Intersect(ray,t) && t<length) {++rejected;break;}
        }
    }
    if(!rejected)return 4;
    auto clusters = ComputeBlockClusters(points,fractures);
    std::ofstream structural("structural_seed.json");
    structural << std::setprecision(12) << "{\"source_units\":\"source Y-up; scale in metadata\",\"fractures\":[";
    for (int i=0;i<fractures.Size();++i) {
        if(i) structural << ',';
        const Circle f=fractures.At(i);
        structural << "{\"center\":";Vec(structural,f.Center());
        structural << ",\"normal\":";Vec(structural,f.Normal());
        structural << ",\"radius\":" << f.Radius() << '}';
    }
    structural << "],\"clusters\":[";
    for(size_t i=0;i<clusters.size();++i) {
        if(i) structural << ',';
        structural << '[';
        for(size_t j=0;j<clusters[i].pts.size();++j) {
            if(j) structural << ',';
            Vec(structural,clusters[i].pts[j]);
        }
        structural << ']';
    }
    structural << "]}";
    structural.close();
    std::cout << points.Size() << " points; " << fractures.Size() << " fractures; " << clusters.size() << " clusters\n";
    if(Connected) {
        // Retain the original cluster points and add rearward copies before
        // computing its hull. Side plane sets can change; this is a derived
        // connected-face construction, not a guarantee of unchanged joints.
        // The original structural seed stays saved for comparison.
        for(auto& cluster:clusters) {
            const auto original=cluster.pts;
            for(const auto& p:original)
                cluster.pts.push_back(Vector3(p.x,p.y,std::max(p.z,2.0)));
        }
    }
    SDFNode* field=ComputeBlockSDF(clusters);
    if(Connected) field=new BackedFace(field);
    const Box extraction=field->box.Extended(Vector3(0.5));
    const Vector3 step=extraction.Size()/double(Resolution-1);
    MC::mcMesh mesh=PolygonizeSDF(extraction,field);
    // The patched polygonizer now returns physical source positions, not grid
    // indices. Validate outward winding against the actual field on a bounded
    // set, then recompute area-weighted normals from final metre triangles.
    int outward=0,inward=0;
    const size_t stride=std::max(size_t(1),mesh.indices.size()/3/128);
    for(size_t t=0;t<mesh.indices.size()/3;t+=stride) {
        auto a=mesh.vertices[mesh.indices[3*t]],b=mesh.vertices[mesh.indices[3*t+1]],c=mesh.vertices[mesh.indices[3*t+2]];
        Vector3 pa(a.x,a.y,a.z),pb(b.x,b.y,b.z),pc(c.x,c.y,c.z);
        Vector3 n=Cross(pb-pa,pc-pa);
        if(Magnitude(n)<1e-12) continue;
        n=Normalize(n); const Vector3 p=(pa+pb+pc)/3.0;
        const double delta=field->Signed(p+n*0.02)-field->Signed(p-n*0.02);
        if(delta>1e-8)++outward; else if(delta< -1e-8)++inward;
    }
    if(!outward && !inward) return 3;
    const bool flip=inward>outward;
    std::vector<Vector3> vertices,normals(mesh.vertices.size(),Vector3(0));
    for(const auto& p:mesh.vertices) vertices.push_back(ToMetres(Vector3(p.x,p.y,p.z)));
    size_t degenerate=0;
    std::vector<unsigned int> indices;
    for(size_t t=0;t<mesh.indices.size();t+=3) {
        unsigned int a=mesh.indices[t],b=mesh.indices[t+1],c=mesh.indices[t+2];
        if(flip)std::swap(b,c);
        const Vector3 n=Cross(vertices[b]-vertices[a],vertices[c]-vertices[a]);
        if(Magnitude(n)<1e-12) {++degenerate;continue;}
        indices.push_back(a);indices.push_back(b);indices.push_back(c);
        normals[a]+=n;normals[b]+=n;normals[c]+=n;
    }
    std::ofstream out("sample.obj");out << std::setprecision(12) << "# Physical metres, Z-up; front is +Y\ng ImplicitFace\n";
    for(const auto& p:vertices)out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    for(const auto& n:normals) {const Vector3 q=Magnitude(n)>1e-15?Normalize(n):Vector3(0,0,1);out << "vn " << q.x << ' ' << q.y << ' ' << q.z << '\n';}
    for(size_t t=0;t<indices.size();t+=3)out << "f " << indices[t]+1 << "//" << indices[t]+1 << ' ' << indices[t+1]+1 << "//" << indices[t+1]+1 << ' ' << indices[t+2]+1 << "//" << indices[t+2]+1 << '\n';
    out.close();
    std::ofstream meta("geometry.json");meta << std::setprecision(12);
    meta << "{\"input_box_source_min\":";Vec(meta,tile[0]);meta << ",\"input_box_source_max\":";Vec(meta,tile[1]);
    meta << ",\"extraction_box_source_min\":";Vec(meta,extraction[0]);meta << ",\"extraction_box_source_max\":";Vec(meta,extraction[1]);
    meta << ",\"grid_step_source\":";Vec(meta,step);
    meta << ",\"bounds_m_min\":";Vec(meta,Box(vertices)[0]);meta << ",\"bounds_m_max\":";Vec(meta,Box(vertices)[1]);
    meta << ",\"vertices\":" << vertices.size() << ",\"triangles\":" << indices.size()/3 << ",\"discarded_degenerate_triangles\":" << degenerate;
    meta << ",\"adjacent_point_pairs\":" << adjacency << ",\"fracture_blocked_adjacent_pairs\":" << rejected;
    meta << ",\"orientation_outward_votes\":" << outward << ",\"orientation_inward_votes\":" << inward << ",\"reversed_winding\":" << (flip?"true":"false") << "}";
    std::cout << "Exported physical sample: " << vertices.size() << " vertices, " << indices.size()/3 << " triangles\n";
    return 0;
}
'''


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_once(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"Expected one verified source fragment, found {text.count(old)}: {old[:70]}")
    return text.replace(old, new)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mode", choices=("polyhedral", "connected-face"), default="polyhedral")
    parser.add_argument("--seed", type=int, default=1234)
    parser.add_argument("--metres-per-unit", type=float, default=1.5)
    parser.add_argument("--smoothing-m", type=float, default=.05)
    parser.add_argument("--resolution", type=int, default=200)
    parser.add_argument("--msbuild", type=Path, default=Path(DEFAULT_MSBUILD))
    args = parser.parse_args()
    if not (0 < args.metres_per_unit <= 2 and 0 < args.smoothing_m <= .25 and 64 <= args.resolution <= 256):
        parser.error("Proof bounds: scale (0,2], smoothing (0,.25] m, resolution64..256")
    upstream, output = args.upstream.resolve(), args.output.resolve()
    project = Path(__file__).resolve().parents[1]
    if not output.is_relative_to(project / "Saved"):
        parser.error("Output must be under this worker's ignored Saved directory")
    if output.exists():
        parser.error("Use a new output directory; experiments are immutable")
    actual = subprocess.check_output(["git", "-C", str(upstream), "rev-parse", "HEAD"], text=True).strip()
    if actual != UPSTREAM_REV:
        raise ValueError(f"Unexpected upstream HEAD {actual}")
    output.mkdir(parents=True)
    source = output / "Source"
    files = subprocess.check_output(["git", "-C", str(upstream), "ls-tree", "-r", "--name-only", UPSTREAM_REV], text=True).splitlines()
    for name in files:
        if name == "LICENSE" or name.startswith("Code/") or name.startswith("VS2022/"):
            dest = source / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(subprocess.check_output(["git", "-C", str(upstream), "show", f"{UPSTREAM_REV}:{name}"]))
    blocks = source / "Code/Source/blocks.cpp"
    code = blocks.read_text()
    code = replace_once(code, '#include <queue>', '#include <queue>\n#include <algorithm>')
    code = replace_once(code, 'num_threads(16)', 'num_threads(8)')
    code = replace_once(code, 'const double smoothRadius = 0.25;', f'const double smoothRadius = {args.smoothing_m / args.metres_per_unit:.17g};')
    code = replace_once(code, 'new SDFGradientWarp(new SDFBlock(planes, smoothRadius))', 'new SDFBlock(planes, smoothRadius)')
    # The upstream Matrix4 indexing/plane intersection cannot provide reliable
    # bounds. Hull vertices already give conservative exact input-point bounds;
    # do not reconstruct them via Plane::ConvexPoints.
    header = source / "Code/Include/blocks.h"
    header.write_text(replace_once(header.read_text(), 'SDFBlock(const std::vector<Plane>& pl, double sr);', 'SDFBlock(const std::vector<Plane>& pl, double sr, const Box& bounds);'))
    code = replace_once(code, 'SDFBlock::SDFBlock(const std::vector<Plane>& pl, double sr) : SDFNode(Box(Plane::ConvexPoints(pl)).Extended(Vector3(0.01f)))', 'SDFBlock::SDFBlock(const std::vector<Plane>& pl, double sr, const Box& bounds) : SDFNode(bounds.Extended(Vector3(0.01f)))')
    code = replace_once(code, 'auto convex = Plane::ConvexPoints(planes);\n\t\tif (convex.size() > 0)', 'if (!allPts.empty())')
    code = replace_once(code, 'new SDFBlock(planes, smoothRadius)', 'new SDFBlock(planes, smoothRadius, Box(allPts))')
    code = replace_once(code, 'const int n = 200;', f'const int n = {args.resolution};')
    code = replace_once(code, 'MC::marching_cube(field, n, n, n, mesh);\n\treturn mesh;', '''MC::marching_cube(field, n, n, n, mesh);
    delete[] field;
    for (auto& p : mesh.vertices) {
        p.x = box[0].x + p.x * cellDiagonal.x;
        p.y = box[0].y + p.y * cellDiagonal.y;
        p.z = box[0].z + p.z * cellDiagonal.z;
    }
    return mesh;''')
    blocks.write_text(code)
    harness = HARNESS.replace('@SCALE@', repr(args.metres_per_unit)).replace('@SEED@', str(args.seed)).replace('@RESOLUTION@', str(args.resolution)).replace('@CONNECTED@', 'true' if args.mode == 'connected-face' else 'false')
    (source / "Code/Source/main.cpp").write_text(harness)
    started = time.perf_counter()
    with (output / "build.log").open("w") as log:
        subprocess.run([str(args.msbuild), str(source / "VS2022/RockFracturing.sln"), "/m:2", "/p:Configuration=Release", "/p:Platform=x64", "/nologo", "/verbosity:minimal"], cwd=source, stdout=log, stderr=subprocess.STDOUT, check=True)
    run = output / "Geometry"
    run.mkdir()
    with (output / "run.log").open("w") as log:
        subprocess.run([str(source / "VS2022/Out/Release/RockFracturing-TVC2020.exe")], cwd=run, stdout=log, stderr=subprocess.STDOUT, check=True)
    metadata = json.loads((run / "geometry.json").read_text())
    metadata.update({"schema": "implicit_cliff_reference_v1", "source_kind": "physical_implicit_blocks_offline_proof", "upstream_revision": UPSTREAM_REV, "upstream_url": "https://github.com/aparis69/Rock-fracturing", "adapter_sha256": sha(Path(__file__)), "mode": args.mode, "seed": args.seed, "metres_per_source_unit": args.metres_per_unit, "units": "metres", "up_axis": "Z", "front_direction": [0,1,0], "source_to_output_rotation_rows": [[1,0,0],[0,0,-1],[0,1,0]], "resolution": [args.resolution]*3, "smoothing_m": args.smoothing_m, "image_warp": False, "image_loaded": False, "nominal_input_domain_width_m": 20*args.metres_per_unit, "backing_source_box": [[-10.1,-10.1,1],[10.1,10.1,4]] if args.mode == "connected-face" else None, "cluster_rear_projection_source_z": 2.0 if args.mode == "connected-face" else None, "normal_method": "area-weighted recomputation after full physical coordinate conversion; winding verified against signed field", "mesh_sha256": sha(run / "sample.obj"), "structural_seed_sha256": sha(run / "structural_seed.json"), "build_and_run_seconds": time.perf_counter()-started, "arena_constraints_integrated": False, "source_snapshot": str(source), "msbuild": str(args.msbuild), "derived_changes": ["12 finite isotropic unit-normal fracture discs of6..13.5m radius at default scale; upstream zero-normal stub unused", "direct convex-hull input-point block bounds, bypassing broken Matrix4/Plane::ConvexPoints", "physical coordinate mapping and normal recomputation", "zero image warp and no image load", "low physical smoothing", "connected mode rear-extends hulls into one shared backing"]})
    (run / "metadata.json").write_text(json.dumps(metadata, indent=2)+"\n")
    print(json.dumps({"mesh": str(run / "sample.obj"), "metadata": str(run / "metadata.json"), "vertices": metadata["vertices"], "triangles": metadata["triangles"], "seconds": metadata["build_and_run_seconds"]}, indent=2), flush=True)


if __name__ == "__main__":
    main()
