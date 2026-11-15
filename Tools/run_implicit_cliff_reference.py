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
import math
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
constexpr bool SourceSkin = @SOURCE_SKIN@;
constexpr bool PlanesOnly = @PLANES_ONLY@;
constexpr double Neighbor = @NEIGHBOR@;

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

static FractureSet SkinFractures(const PointSet3& points,const Box& box) {
    // Three shared geological families, with finite extent and modest angular
    // variation. Centers belong to the actual curved source skin; no tiling.
    FractureSet result;
    const Vector3 canonical[3]={Vector3(.91,.24,.34),Vector3(-.30,.36,.88),Vector3(.48,.80,-.36)};
    const double area=box.Size().x*box.Size().y*Scale*Scale;
    const int count=std::min(80,std::max(18,int(std::sqrt(area)/1.7)));
    for(int i=0;i<count;++i) {
        const int family=i%3;
        Vector3 n=canonical[family]+Vector3(Random::Uniform(-.10,.10),Random::Uniform(-.10,.10),Random::Uniform(-.10,.10));
        n=Normalize(n);
        const Vector3 sourceN(n.x,n.z,-n.y);
        const int id=(i*7919+1237)%points.Size();
        result.fractures.push_back(Circle(points.At(id),sourceN,Random::Uniform(10.0,24.0)/Scale));
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
    Box tile = Connected ? Box(Vector3(-10,-10,-2),Vector3(10,10,2)) : Box(Vector3(0),10.0);
    PointSet3 points;
    if(SourceSkin) {
        std::ifstream input("sample_points_source.txt");
        size_t count;input>>count;
        if(!input || count<20)return 5;
        for(size_t i=0;i<count;++i) {double x,y,z;input>>x>>y>>z;if(!input)return 6;points.pts.push_back(Vector3(x,y,z));}
        tile=Box(points.pts);
    } else points=PoissonSamplingBox(tile,0.5,10000);
    FractureSet fractures = SourceSkin?SkinFractures(points,tile):SparseFractures(tile);
    size_t adjacency=0,rejected=0;
    for(int i=0;i<points.Size();++i)for(int j=i+1;j<points.Size();++j) {
        const Vector3 delta=points.At(j)-points.At(i);const double length=Magnitude(delta);
        if(length>Neighbor || length<1e-10)continue;
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
        structural << "{\"point_indices\":[";
        for(size_t j=0;j<clusters[i].ids.size();++j){if(j)structural<<',';structural<<clusters[i].ids[j];}
        structural << "],\"points_source\":[";
        for(size_t j=0;j<clusters[i].pts.size();++j) {
            if(j) structural << ',';
            Vec(structural,clusters[i].pts[j]);
        }
        structural << "]}";
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
    if(PlanesOnly) {
        std::ofstream meta("geometry.json");meta << std::setprecision(12);
        meta<<"{\"mesh_extracted\":false,\"vertices\":0,\"triangles\":0,\"adjacent_point_pairs\":"<<adjacency<<",\"fracture_blocked_adjacent_pairs\":"<<rejected;
        meta<<",\"input_box_source_min\":";Vec(meta,tile[0]);meta<<",\"input_box_source_max\":";Vec(meta,tile[1]);
        meta<<",\"field_box_source_min\":";Vec(meta,field->box[0]);meta<<",\"field_box_source_max\":";Vec(meta,field->box[1]);meta<<"}";
        return 0;
    }
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
    parser.add_argument("--mode", choices=("polyhedral", "connected-face", "source-skin"), default="polyhedral")
    parser.add_argument("--sample-points", type=Path)
    parser.add_argument("--planes-only", action="store_true")
    parser.add_argument("--skin-neighbor-m", type=float, default=2.6)
    parser.add_argument("--skin-max-block-m", type=float, default=9.0)
    parser.add_argument("--seed", type=int, default=1234)
    parser.add_argument("--metres-per-unit", type=float, default=1.5)
    parser.add_argument("--smoothing-m", type=float, default=.05)
    parser.add_argument("--resolution", type=int, default=200)
    parser.add_argument("--msbuild", type=Path, default=Path(DEFAULT_MSBUILD))
    args = parser.parse_args()
    if args.sample_points:
        args.mode = "source-skin"
    elif args.mode == "source-skin":
        parser.error("source-skin requires --sample-points JSON")
    if not (0 < args.metres_per_unit <= 2 and 0 < args.smoothing_m <= .25 and 64 <= args.resolution <= 256):
        parser.error("Proof bounds: scale (0,2], smoothing (0,.25] m, resolution64..256")
    if not (0 < args.skin_neighbor_m <= 5 and args.skin_neighbor_m < args.skin_max_block_m <= 24):
        parser.error("Skin proof needs positive neighbor<=5m and neighbor<max-block<=24m")
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
    source_samples = None
    if args.sample_points:
        source_samples = json.loads(args.sample_points.read_text())
        pts = source_samples.get("points_m", [])
        depth = source_samples.get("depth_below_face_m", [])
        if len(pts) < 20 or len(pts) != len(depth) or len(pts)>24000:
            raise ValueError("Skin proof needs20..24000 points and one depth per point")
        if any(len(p)!=3 or not all(math.isfinite(float(x)) for x in p) for p in pts) or not all(math.isfinite(float(d)) for d in depth):
            raise ValueError("Source points and depths must be finite")
        (output / "source_points.json").write_bytes(args.sample_points.read_bytes())
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
    header_code = replace_once(header.read_text(), 'SDFBlock(const std::vector<Plane>& pl, double sr);', 'SDFBlock(const std::vector<Plane>& pl, double sr, const Box& bounds);')
    header_code = replace_once(header_code, 'struct BlockCluster\n{\npublic:\n\tstd::vector<Vector3> pts;', 'struct BlockCluster\n{\npublic:\n\tstd::vector<Vector3> pts;\n\tstd::vector<int> ids;')
    header.write_text(header_code)
    code = replace_once(code, 'std::vector<Vector3> cluster;', 'std::vector<Vector3> cluster;\n        std::vector<int> clusterIds;')
    code = replace_once(code, 'cluster.push_back(q);', 'cluster.push_back(q);\n            clusterIds.push_back(index);')
    code = replace_once(code, 'clusters.push_back({ cluster });', 'clusters.push_back({ cluster, clusterIds });')
    neighbor = args.skin_neighbor_m/args.metres_per_unit if source_samples else 2.5
    max_block = args.skin_max_block_m/args.metres_per_unit if source_samples else 10.5
    code = replace_once(code, '2.5f * 2.5f', f'{neighbor:.17g} * {neighbor:.17g}')
    code = replace_once(code, '10.5f * 10.5f', f'{max_block:.17g} * {max_block:.17g}')
    code = replace_once(code, 'SDFBlock::SDFBlock(const std::vector<Plane>& pl, double sr) : SDFNode(Box(Plane::ConvexPoints(pl)).Extended(Vector3(0.01f)))', 'SDFBlock::SDFBlock(const std::vector<Plane>& pl, double sr, const Box& bounds) : SDFNode(bounds.Extended(Vector3(0.01f)))')
    code = replace_once(code, 'auto convex = Plane::ConvexPoints(planes);\n\t\tif (convex.size() > 0)', 'if (!allPts.empty())')
    code = replace_once(code, 'new SDFBlock(planes, smoothRadius)', 'new SDFBlock(planes, smoothRadius, Box(allPts))')
    code = replace_once(code, '#include <algorithm>', '#include <algorithm>\n#include <fstream>\n#include <iomanip>')
    code = replace_once(code, 'std::vector<SDFNode*> primitives;', 'std::vector<SDFNode*> primitives;\n    std::ofstream planeExport("block_planes_source.json");\n    planeExport << std::setprecision(17) << "[";\n    int exportedBlocks=0;')
    code = replace_once(code, 'primitives.push_back(new SDFBlock(planes, smoothRadius, Box(allPts)));', '''primitives.push_back(new SDFBlock(planes, smoothRadius, Box(allPts)));
            if(exportedBlocks++)planeExport<<',';
            planeExport<<"{\\\"cluster_index\\\":"<<k<<",\\\"planes_source\\\":[";
            for(size_t pi=0;pi<planes.size();++pi){if(pi)planeExport<<',';
                const Vector3 n=planes[pi].Normal();
                planeExport<<'['<<n.x<<','<<n.y<<','<<n.z<<','<<Dot(n,planes[pi].Point())<<']';}
            planeExport<<"]}";''')
    code = replace_once(code, 'return SDFUnionSphereLOD::OptimizedBVH(primitives, 0.5);', 'planeExport << "]";\n    planeExport.close();\n    return SDFUnionSphereLOD::OptimizedBVH(primitives, 0.5);')
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
    harness = HARNESS.replace('@SCALE@', repr(args.metres_per_unit)).replace('@SEED@', str(args.seed)).replace('@RESOLUTION@', str(args.resolution)).replace('@CONNECTED@', 'true' if args.mode == 'connected-face' else 'false').replace('@SOURCE_SKIN@', 'true' if source_samples else 'false').replace('@NEIGHBOR@', repr(neighbor)).replace('@PLANES_ONLY@', 'true' if args.planes_only else 'false')
    (source / "Code/Source/main.cpp").write_text(harness)
    started = time.perf_counter()
    with (output / "build.log").open("w") as log:
        subprocess.run([str(args.msbuild), str(source / "VS2022/RockFracturing.sln"), "/m:2", "/p:Configuration=Release", "/p:Platform=x64", "/nologo", "/verbosity:minimal"], cwd=source, stdout=log, stderr=subprocess.STDOUT, check=True)
    run = output / "Geometry"
    run.mkdir()
    if source_samples:
        with (run / "sample_points_source.txt").open("w") as f:
            f.write(f"{len(source_samples['points_m'])}\n")
            for x,y,z in source_samples['points_m']:
                f.write(f"{x/args.metres_per_unit:.17g} {z/args.metres_per_unit:.17g} {-y/args.metres_per_unit:.17g}\n")
    with (output / "run.log").open("w") as log:
        subprocess.run([str(source / "VS2022/Out/Release/RockFracturing-TVC2020.exe")], cwd=run, stdout=log, stderr=subprocess.STDOUT, check=True)
    metadata = json.loads((run / "geometry.json").read_text())
    structure = json.loads((run / "structural_seed.json").read_text())
    plane_blocks = json.loads((run / "block_planes_source.json").read_text())
    physical_blocks = []
    for block in plane_blocks:
        cluster = structure['clusters'][block['cluster_index']]
        ids = cluster['point_indices']
        points_m = [[p[0]*args.metres_per_unit,-p[2]*args.metres_per_unit,p[1]*args.metres_per_unit] for p in cluster['points_source']]
        depths = [source_samples['depth_below_face_m'][i] for i in ids] if source_samples else []
        physical_blocks.append({'cluster_index': block['cluster_index'], 'point_indices': ids, 'points_m': points_m,
            'planes': [{'normal':[p[0],-p[2],p[1]],'distance_m':p[3]*args.metres_per_unit} for p in block['planes_source']],
            'bounds_m_min':[min(p[i] for p in points_m) for i in range(3)], 'bounds_m_max':[max(p[i] for p in points_m) for i in range(3)],
            'depth_below_face_m':depths,'depth_min_m':min(depths) if depths else None,'depth_max_m':max(depths) if depths else None,'depth_mean_m':sum(depths)/len(depths) if depths else None})
    block_export={'schema':'implicit_cliff_planes_v1','units':'metres','up_axis':'Z','front_direction':[0,1,0], 'inside_convention':'dot(normal,position_m)-distance_m <= 0', 'blocks':physical_blocks,
        'source_points_path':str(args.sample_points.resolve()) if source_samples else None, 'source_points_sha256':sha(args.sample_points) if source_samples else None,
        'source_metadata':{k:v for k,v in source_samples.items() if k not in ('points_m','depth_below_face_m')} if source_samples else None,
        'fractures':[{'center_m':[d['center'][0]*args.metres_per_unit,-d['center'][2]*args.metres_per_unit,d['center'][1]*args.metres_per_unit], 'normal':[d['normal'][0],-d['normal'][2],d['normal'][1]], 'radius_m':d['radius']*args.metres_per_unit, 'family':i%3 if source_samples else None} for i,d in enumerate(structure['fractures'])]}
    (run / 'block_planes_m.json').write_text(json.dumps(block_export,separators=(',',':'))+'\n')
    metadata.update({"schema": "implicit_cliff_reference_v1", "source_kind": "physical_implicit_blocks_offline_proof", "upstream_revision": UPSTREAM_REV, "upstream_url": "https://github.com/aparis69/Rock-fracturing", "adapter_sha256": sha(Path(__file__)), "mode": args.mode, "seed": args.seed, "metres_per_source_unit": args.metres_per_unit, "units": "metres", "up_axis": "Z", "front_direction": [0,1,0], "source_to_output_rotation_rows": [[1,0,0],[0,0,-1],[0,1,0]], "resolution": [args.resolution]*3, "smoothing_m": args.smoothing_m, "image_warp": False, "image_loaded": False, "nominal_input_domain_width_m": 20*args.metres_per_unit, "backing_source_box": [[-10.1,-10.1,1],[10.1,10.1,4]] if args.mode == "connected-face" else None, "cluster_rear_projection_source_z": 2.0 if args.mode == "connected-face" else None, "normal_method": "area-weighted recomputation after full physical coordinate conversion; winding verified against signed field", "mesh_sha256": sha(run / "sample.obj") if not args.planes_only else None, "structural_seed_sha256": sha(run / "structural_seed.json"), "build_and_run_seconds": time.perf_counter()-started, "arena_constraints_integrated": False, "source_snapshot": str(source), "msbuild": str(args.msbuild), "derived_changes": ["12 finite isotropic unit-normal fracture discs of6..13.5m radius at default scale; upstream zero-normal stub unused", "direct convex-hull input-point block bounds, bypassing broken Matrix4/Plane::ConvexPoints", "physical coordinate mapping and normal recomputation", "zero image warp and no image load", "low physical smoothing", "connected mode rear-extends hulls into one shared backing"]})
    metadata['block_planes_sha256'] = sha(run / 'block_planes_m.json')
    metadata['mesh_extracted'] = not args.planes_only
    if args.planes_only:
        metadata['resolution'] = None
        metadata['normal_method'] = 'Original unit convex-hull plane normals converted by proper rotation; no mesh normals or marching cubes generated'
    if source_samples:
        used = [i for b in physical_blocks for i in b['point_indices']]
        if len(used) != len(set(used)):
            raise RuntimeError('A source point belongs to multiple output clusters')
        metadata.update({
            'source_kind': 'source_defined_curved_cliff_cluster_proof',
            'nominal_input_domain_width_m': (metadata['input_box_source_max'][0]-metadata['input_box_source_min'][0])*args.metres_per_unit,
            'source_points_path': str(args.sample_points.resolve()),
            'source_points_sha256': sha(args.sample_points),
            'source_point_count': len(source_samples['points_m']),
            'used_point_count': len(used),
            'excluded_point_count': len(source_samples['points_m'])-len(used),
            'cluster_count': len(physical_blocks),
            'source_frame': source_samples.get('frame'),
            'depth_semantics': 'Copied source depth below original face; supplied approximation, not a new geometric distance calculation',
            'neighbor_m': args.skin_neighbor_m,
            'maximum_cluster_point_separation_m': args.skin_max_block_m,
            'fracture_count': len(block_export['fractures']),
            'fracture_families_canonical': [[.91,.24,.34],[-.30,.36,.88],[.48,.80,-.36]],
            'fracture_normal_component_jitter': .10,
            'fracture_radius_m_range': [10.,24.],
            'derived_changes': [
                'Authoritative actual curved source points; exact original point IDs and supplied depth retained',
                'Three coherent finite fracture families in one common physical frame; no stamps or scaling',
                'Direct convex-hull input-point bounds bypass broken upstream matrix/plane reconstruction',
                'Zero image warp/load; physical plane equations exported; no rectangular backing',
                'Source constrained clustering only: root owns retained core, exposure selection and final solids',
            ],
        })
    (run / "metadata.json").write_text(json.dumps(metadata, indent=2)+"\n")
    print(json.dumps({"mesh": str(run / "sample.obj") if not args.planes_only else None, "metadata": str(run / "metadata.json"), "vertices": metadata["vertices"], "triangles": metadata["triangles"], "seconds": metadata["build_and_run_seconds"]}, indent=2), flush=True)


if __name__ == "__main__":
    main()
