"""Generate original, deterministic waterfall whitewater VDB loops offline.

Run with Blender's bundled Python, not Blender's UI. Only numpy and openvdb are
required. No third-party data is read. Coordinates: X across, Y downstream away
from the cliff, Z up. Computation uses metres; VDB transforms use centimetres for
Unreal. Origin is the floor below the lip, NOT the volume centre.

This is an art-directed ballistic particle model, not a liquid/air fluid solver.
The water core, surface foam and individual near-camera droplets remain separate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
import time
import zlib


# Keep these handles alive: CPython removes DLL search paths when they are closed.
_DLL_HANDLES = []
if os.name == "nt":
    candidates = [Path(sys.executable).resolve().parents[3]]
    if os.environ.get("BLENDER_ROOT"):
        candidates.insert(0, Path(os.environ["BLENDER_ROOT"]))
    for base in candidates:
        for directory in (base, base / "blender.shared", base / "blender.crt"):
            if directory.is_dir():
                _DLL_HANDLES.append(os.add_dll_directory(str(directory)))

import numpy as np
import openvdb as vdb


def smoothstep(lo, hi, x):
    t = np.clip((x - lo) / (hi - lo), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def digest(path):
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def png_gray(path, pixels):
    """Write a diagnostic density projection using only the standard library."""
    pixels = np.ascontiguousarray(pixels, dtype=np.uint8)
    height, width = pixels.shape
    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload
                + struct.pack(">I", zlib.crc32(tag + payload) & 0xffffffff))
    scanlines = b"".join(b"\x00" + row.tobytes() for row in pixels)
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b""))


class Waterfall:
    def __init__(self, args):
        self.a = args
        rng = np.random.default_rng(args.seed)
        count = math.ceil(args.emission_rate * args.duration)
        self.rate = count / args.duration
        self.birth = (np.arange(count) + rng.random(count)) / self.rate
        self.x = rng.uniform(-args.width_m / 2, args.width_m / 2, count)
        self.y = rng.normal(0, min(args.depth_m * .045, .8), count)
        self.terminal = rng.uniform(26, 35, count)
        self.initial_down = rng.uniform(1.0, 3.0, count)
        self.tau = self.terminal / 9.81
        self.phase = rng.uniform(0, 2 * np.pi, (count, 3))
        self.lateral = rng.normal(0, .40, count)
        # Persistent source channels and periodically pulsing gaps break a sheet
        # into coherently moving strands; no arbitrary 3D noise volume is used.
        source = (.52 + .28 * np.sin(self.x * .91 + .8)
                  + .19 * np.sin(self.x * 1.77 - 1.4))
        pulse = (.72 + .28 * np.sin(2 * np.pi * self.birth / args.duration
                                    + self.x * .56))
        edge = 1 - smoothstep(args.width_m * .40, args.width_m * .50, np.abs(self.x))
        self.strength = np.clip(source, .10, 1.05) * pulse * edge * rng.uniform(.65, 1.4, count)
        lo, hi = np.zeros(count), np.full(count, args.height_m / 15 + 8)
        for _ in range(38):
            mid = (lo + hi) / 2
            below = self.drop(mid) > args.height_m
            hi = np.where(below, mid, hi)
            lo = np.where(below, lo, mid)
        self.flight = (lo + hi) / 2
        self.impact_birth = np.mod(self.birth + self.flight, args.duration)
        self.foot = self.fall_positions(self.flight)
        mist = rng.random(count) < .22
        self.impact_g = np.where(mist, .8, 9.81)
        self.impact_drag = np.where(mist, .7, 1.6)
        self.impact_vz = np.where(mist, rng.uniform(1.4, 3.2, count), rng.uniform(5.5, 13, count))
        self.impact_vx = rng.normal(0, 3.5, count)
        self.impact_vy = rng.uniform(1.5, 7.5, count)
        self.impact_life = np.where(mist, rng.uniform(2.5, 4.0, count), rng.uniform(1.4, 2.7, count))
        self.impact_mass = np.where(mist, .18, .70)
        # Mass is normalized by emission rate and voxel volume so changing either
        # does not radically alter the apparent density.
        self.mass = args.width_m * 9.0 / self.rate / (args.voxel_m ** 3)

    def drop(self, age):
        return (self.terminal * age + (self.initial_down - self.terminal)
                * self.tau * (1 - np.exp(-age / self.tau)))

    def fall_positions(self, age):
        a = self.a
        fade = 1 - np.exp(-age / 1.5)
        # Downward acceleration remains monotonic; small lateral flutter follows
        # each particle continuously rather than changing random seeds per frame.
        x = (self.x + self.lateral * age + fade * .42
             * (np.sin(age * 2.3 + self.phase[:, 0]) - np.sin(self.phase[:, 0])))
        y = (self.y + a.downstream_m * (1 - np.exp(-age / 2.4))
             / (1 - np.exp(-self.flight / 2.4)) if hasattr(self, "flight")
             else self.y + a.downstream_m * age / 5)
        y += fade * .32 * (np.sin(age * 3.1 + self.phase[:, 1]) - np.sin(self.phase[:, 1]))
        z = a.height_m - self.drop(age)
        return np.column_stack((x, y, z))

    def particles(self, time_s, component):
        # Emissions repeat, while particles may live longer than one loop. Include
        # previous cycles rather than resetting surviving particles at the seam.
        t = round(time_s % self.a.duration, 12)
        birth = self.birth if component == "Falling" else self.impact_birth
        lifetime = self.flight if component == "Falling" else self.impact_life
        base_age = np.mod(t - birth, self.a.duration)
        positions, masses = [], []
        for cycle in range(math.ceil(float(lifetime.max()) / self.a.duration) + 1):
            age = base_age + cycle * self.a.duration
            active = age < lifetime
            if not np.any(active):
                continue
            if component == "Falling":
                p = self.fall_positions(age)
                fade = smoothstep(0, .12, age) * (1 - smoothstep(lifetime - .15, lifetime, age))
                mass = self.mass * self.strength * fade
            else:
                drag = self.impact_drag
                travel = drag * (1 - np.exp(-age / drag))
                p = self.foot.copy()
                p[:, 0] += self.impact_vx * travel + .35 * age
                p[:, 1] += self.impact_vy * travel + 1.25 * age
                p[:, 2] = ((self.impact_vz + self.impact_g * drag) * travel
                           - self.impact_g * drag * age)
                fade = smoothstep(0, .07, age) * (1 - smoothstep(lifetime - .35, lifetime, age))
                fade *= smoothstep(-.15, .25, p[:, 2])
                mass = self.mass * self.strength * fade * self.impact_mass
                active &= p[:, 2] > -.15
            positions.append(p[active])
            masses.append(mass[active])
        return np.concatenate(positions), np.concatenate(masses)


class FittedWaterfall(Waterfall):
    """Advect particles on the exported physical path; never stretch a cache."""
    def __init__(self, args, profile):
        self.profile = profile
        super().__init__(args)
        points = np.asarray(profile["samples_local_cm"], dtype=float) / 100
        length = np.linalg.norm(np.diff(points, axis=0), axis=1)
        keep = np.concatenate(([True], length > .001))
        points = points[keep]
        lengths = np.linalg.norm(np.diff(points, axis=0), axis=1)
        if len(points) < 2 or lengths.sum() <= 1:
            raise ValueError("Profile needs a nondegenerate physical centreline")
        original_s = np.concatenate(([0.0], np.cumsum(lengths)))
        # One-metre path stations preserve fractures and reduce speed-integration
        # dependence on the number of cliff rows used by the exporter.
        self.path_s = np.linspace(0, original_s[-1], math.ceil(original_s[-1]) + 1)
        self.path = np.column_stack([np.interp(self.path_s, original_s, points[:, axis]) for axis in range(3)])
        self.cross_offsets = None
        if "cross_sections_local_cm" in profile:
            sections = np.asarray(profile["cross_sections_local_cm"], dtype=float)[keep] / 100
            self.cross_offsets = np.asarray(profile["cross_offsets_cm"], dtype=float) / 100
            if sections.shape != (len(points), len(self.cross_offsets), 3):
                raise ValueError("Cross-section sample shape does not match the centreline")
            self.cross_tracks = np.empty((len(self.path_s), len(self.cross_offsets), 3))
            for track in range(len(self.cross_offsets)):
                for axis in range(3):
                    self.cross_tracks[:, track, axis] = np.interp(self.path_s, original_s, sections[:, track, axis])
        delta = np.diff(self.path, axis=0)
        ds = np.linalg.norm(delta, axis=1)
        tangent = delta / np.maximum(ds[:, None], 1e-6)
        speed = np.empty(len(self.path_s))
        speed[0] = 12.0  # this is a lower reach already fed by the upstream fall
        times = np.zeros(len(self.path_s))
        for i, step in enumerate(ds):
            acceleration = 9.81 * (-tangent[i, 2] - speed[i] ** 2 / 32 ** 2)
            speed[i + 1] = math.sqrt(max(3 ** 2, speed[i] ** 2 + 2 * acceleration * step))
            times[i + 1] = times[i] + step / max((speed[i] + speed[i + 1]) * .5, 1)
        self.path_time = times
        self.path_speed = speed
        self.path_tangent = np.vstack((tangent, tangent[-1]))
        normals = np.column_stack((np.zeros(len(tangent)), -tangent[:, 2], tangent[:, 1]))
        normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-6)
        normals[normals[:, 1] < 0] *= -1
        self.path_normal = np.vstack((normals, normals[-1]))
        self.speed_scale = .85 + .30 * (self.terminal - 26) / 9
        self.flight = times[-1] / self.speed_scale
        self.impact_birth = np.mod(self.birth + self.flight, args.duration)
        self.profile_ready = True
        self.foot = self.fall_positions(self.flight)
        self.foot[:, 2] = self.water_height(self.foot[:, :2]) + .15
        current = np.asarray(profile["current_local_cm_s"], dtype=float) / 100
        current[2] = 0
        self.current = current
        self.along = current / max(np.linalg.norm(current), 1e-6)
        self.across = np.array((self.along[1], -self.along[0], 0))
        # A separate subset leaves the core as short ballistic fragments. The
        # density remains close to the source rather than becoming rising smoke.
        self.spray_indices = np.arange(0, len(self.birth), 5)
        self.spray_launch_time = times[-1] * (.12 + .72 * (self.phase[:, 2] / (2 * np.pi)))
        self.spray_birth = np.mod(self.birth + self.spray_launch_time / self.speed_scale, args.duration)

    def sample_path(self, time_s):
        return np.column_stack([np.interp(time_s, self.path_time, self.path[:, axis]) for axis in range(3)])

    def sample_normal(self, time_s):
        normal = np.column_stack([np.interp(time_s, self.path_time, self.path_normal[:, axis]) for axis in range(3)])
        return normal / np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-6)

    def sample_width(self, time_s, across):
        if self.cross_offsets is None:
            p = self.sample_path(time_s)
            p[:, 0] += across
            return p
        i = np.clip(np.searchsorted(self.path_time, time_s, side="right")-1, 0, len(self.path_time)-2)
        j = np.clip(np.searchsorted(self.cross_offsets, across, side="right")-1, 0, len(self.cross_offsets)-2)
        t = np.clip((time_s-self.path_time[i])/(self.path_time[i+1]-self.path_time[i]), 0, 1)[:, None]
        x = np.clip((across-self.cross_offsets[j])/(self.cross_offsets[j+1]-self.cross_offsets[j]), 0, 1)[:, None]
        a = self.cross_tracks[i, j] * (1-x) + self.cross_tracks[i, j+1] * x
        b = self.cross_tracks[i+1, j] * (1-x) + self.cross_tracks[i+1, j+1] * x
        return a*(1-t) + b*t

    def water_height(self, xy):
        water = self.profile["receiving_water"]
        n = int(water["count"])
        height = np.asarray(water["heights_local_cm_y_major"], dtype=float).reshape(n, n) / 100
        origin = np.asarray(water["origin_local_cm"][:2], dtype=float) / 100
        index = (xy - origin) / (float(water["step_cm"]) / 100)
        index = np.clip(index, 0, n - 1.000001)
        cell = np.floor(index).astype(int)
        f = index - cell
        x, y = cell[:, 0], cell[:, 1]
        return ((height[y, x] * (1-f[:, 0]) + height[y, x+1] * f[:, 0]) * (1-f[:, 1])
                + (height[y+1, x] * (1-f[:, 0]) + height[y+1, x+1] * f[:, 0]) * f[:, 1])

    def fall_positions(self, age):
        if not getattr(self, "profile_ready", False):
            return super().fall_positions(age)
        clock = np.minimum(age * self.speed_scale, self.path_time[-1])
        p = self.sample_width(clock, self.x + .25 * np.sin(age * 2.3 + self.phase[:, 0]))
        outward = .20 + np.abs(self.y) * .6 + .18 * np.sin(age * 3.1 + self.phase[:, 1])
        # Pure radial clearance matches the core's contact-safe offset. Raising
        # a sample in Z can put it inside a downward-facing rock ledge.
        p[:, 1] += outward
        # Blend into the actual contact surface across the source width.
        join = smoothstep(self.flight - .15, self.flight, age)
        p[:, 2] += (self.water_height(p[:, :2]) + .15 - p[:, 2]) * join
        return p

    def particles(self, time_s, component):
        if component == "Falling":
            p, mass = super().particles(time_s, component)
            indices = self.spray_indices
            age = np.mod(round(time_s % self.a.duration, 12) - self.spray_birth[indices], self.a.duration)
            active = age < .85
            indices, age = indices[active], age[active]
            launch = self.spray_launch_time[indices]
            normal = np.tile((0.0, 1.0, 0.0), (len(indices), 1))
            start = self.sample_width(launch, self.x[indices])
            tangent = np.column_stack([np.interp(launch, self.path_time, self.path_tangent[:, axis]) for axis in range(3)])
            speed = np.interp(launch, self.path_time, self.path_speed) * self.speed_scale[indices]
            travel = .6 * (1 - np.exp(-age / .6))
            spray = start + tangent * (speed * travel)[:, None] + normal * (1.5 + 3 * travel)[:, None]
            spray[:, 2] -= .5 * 9.81 * age ** 2
            expected = self.sample_width(np.minimum(launch + age * self.speed_scale[indices], self.path_time[-1]), self.x[indices])
            clearance = spray[:, 1] - expected[:, 1]
            fade = smoothstep(0, .08, age) * (1-smoothstep(.55, .85, age)) * smoothstep(0, .4, clearance)
            spray_mass = self.mass * self.strength[indices] * fade * .35
            return np.concatenate((p, spray)), np.concatenate((mass, spray_mass))
        positions, masses = [], []
        base_age = np.mod(round(time_s % self.a.duration, 12) - self.impact_birth, self.a.duration)
        for cycle in range(math.ceil(float(self.impact_life.max()) / self.a.duration) + 1):
            age = base_age + cycle * self.a.duration
            drag = self.impact_drag
            travel = drag * (1 - np.exp(-age / drag))
            p = self.foot.copy()
            p += self.across * (self.impact_vx * travel)[:, None]
            p += self.along * (self.impact_vy * travel)[:, None] + self.current * (age * .25)[:, None]
            p[:, 2] += ((self.impact_vz + self.impact_g * drag) * travel - self.impact_g * drag * age)
            clearance = p[:, 2] - self.water_height(p[:, :2])
            fade = smoothstep(0, .07, age) * (1-smoothstep(self.impact_life-.35, self.impact_life, age))
            fade *= smoothstep(-.15, .25, clearance)
            mass = self.mass * self.strength * fade * self.impact_mass
            active = (age < self.impact_life) & (clearance > -.15)
            positions.append(p[active]); masses.append(mass[active])
        return np.concatenate(positions), np.concatenate(masses)

    def bounds(self, component):
        low, high = np.full(3, np.inf), np.full(3, -np.inf)
        for t in np.linspace(0, self.a.duration, 16, endpoint=False):
            p, mass = self.particles(float(t), component)
            p = p[mass > 1e-8]
            low = np.minimum(low, p.min(axis=0)); high = np.maximum(high, p.max(axis=0))
        return low - 4, high + 4


class Raster:
    def __init__(self, args, component, model=None):
        self.a = args
        self.component = component
        dx = args.width_m / 2
        if component == "Falling":
            low = (-dx - 7, -args.depth_m * .35, -1)
            high = (dx + 7, args.downstream_m + args.depth_m * .45, args.height_m + 2)
            self.blur = (.55, .55, 1.0)
        else:
            low = (-dx - 14, args.downstream_m - args.depth_m * .35, -1)
            high = (dx + 14, args.downstream_m + args.depth_m * 1.5, 12)
            self.blur = (.65, .65, .60)
        if isinstance(model, FittedWaterfall):
            low, high = model.bounds(component)
        self.lo = np.floor(np.array(low) / args.voxel_m).astype(int)
        self.hi = np.ceil(np.array(high) / args.voxel_m).astype(int)
        self.shape = tuple((self.hi - self.lo + 1).tolist())
        if math.prod(self.shape) > 12_000_000:
            raise ValueError("Preview volume exceeds 12 million voxels; increase --voxel-m.")
        self.anchors = [(x, y, z) for x in (0, self.shape[0] - 1)
                        for y in (0, self.shape[1] - 1) for z in (0, self.shape[2] - 1)]

    def make(self, model, t):
        points, mass = model.particles(t, self.component)
        p = points / self.a.voxel_m - self.lo
        base = np.floor(p).astype(np.int32)
        frac = p - base
        valid = np.all((base >= 0) & (base + 1 < np.array(self.shape)), axis=1)
        clipped_mass = float(mass[~valid].sum()) / max(float(mass.sum()), 1e-12)
        base, frac, mass = base[valid], frac[valid], mass[valid]
        density = np.zeros(self.shape, dtype=np.float32)
        for x in (0, 1):
            for y in (0, 1):
                for z in (0, 1):
                    weight = mass.copy()
                    for axis, bit in enumerate((x, y, z)):
                        weight *= frac[:, axis] if bit else 1 - frac[:, axis]
                    np.add.at(density, (base[:, 0] + x, base[:, 1] + y, base[:, 2] + z), weight)
        # Compact reconstruction kernels in metres: coherent strand geometry
        # comes from trajectories, smoothing only removes voxel/particle aliasing.
        for axis, radius_m in enumerate(self.blur):
            sigma = max(.35, radius_m / self.a.voxel_m)
            radius = math.ceil(2 * sigma)
            offsets = np.arange(-radius, radius + 1)
            kernel = np.exp(-.5 * (offsets / sigma) ** 2)
            kernel /= kernel.sum()
            pad = [(0, 0)] * 3
            pad[axis] = (radius, radius)
            padded = np.pad(density, pad)
            result = np.zeros_like(density)
            for offset, weight in enumerate(kernel):
                selection = [slice(None)] * 3
                selection[axis] = slice(offset, offset + self.shape[axis])
                result += padded[tuple(selection)] * weight
            density = result
        density[density < self.a.threshold] = 0
        # Tiny markers stabilize the active bounds. Their density is below an
        # 8-bit import quantum and far below visible extinction; no solid box.
        for anchor in self.anchors:
            density[anchor] = 1e-8
        return density, clipped_mass

    def write(self, path, density, frame):
        grid = vdb.FloatGrid()
        grid.name = "density"
        grid.gridClass = vdb.GridClass.FOG_VOLUME
        grid.transform = vdb.createLinearTransform(voxelSize=self.a.voxel_m * 100.0)
        grid.copyFromArray(density, ijk=tuple(int(v) for v in self.lo))
        grid["creator"] = "DirectorsCut procedural waterfall generator"
        grid["component"] = self.component
        grid["units"] = "centimeters"
        grid["frame"] = frame
        grid["fps"] = float(self.a.fps)
        grid["seed"] = self.a.seed
        grid["loop_duration_seconds"] = float(self.a.duration)
        vdb.write(str(path), grids=[grid])
        return grid


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1] / "Saved/GeneratedWaterfalls")
    parser.add_argument("--width-m", type=float, default=24.0)
    parser.add_argument("--height-m", type=float, default=80.0)
    parser.add_argument("--depth-m", type=float, default=14.0, help="cross-fall padding and impact envelope depth")
    parser.add_argument("--downstream-m", type=float, default=10.0, help="horizontal lip-to-impact displacement")
    parser.add_argument("--voxel-m", type=float, default=1.0)
    parser.add_argument("--fps", type=float, default=12.0)
    parser.add_argument("--duration", type=float, default=4.0)
    parser.add_argument("--emission-rate", type=float, default=3500.0)
    parser.add_argument("--seed", type=int, default=54117)
    parser.add_argument("--threshold", type=float, default=.00005)
    parser.add_argument("--profile", type=Path, help="actual lower water-core profile exported with -DCExportWaterfallProfile")
    args = parser.parse_args()
    positive = (args.width_m, args.height_m, args.depth_m, args.voxel_m,
                args.fps, args.duration, args.emission_rate)
    if not all(math.isfinite(x) and x > 0 for x in positive):
        parser.error("dimensions, resolution, rate and duration must be finite and positive")
    if not math.isfinite(args.downstream_m) or args.downstream_m < 0:
        parser.error("downstream distance must be finite and nonnegative")
    if not math.isfinite(args.threshold) or args.threshold < 0:
        parser.error("density threshold must be finite and nonnegative")
    frames = round(args.duration * args.fps)
    if frames < 4 or not math.isclose(frames, args.duration * args.fps, abs_tol=1e-6):
        parser.error("duration * fps must be an integer of at least four frames")
    if frames > 600 or args.emission_rate * args.duration > 500_000:
        parser.error("preview limit: 600 frames and 500000 emitted particles per cycle")
    args.output.mkdir(parents=True, exist_ok=True)
    # Never mix a changed sequence length with old numbered frames.
    for name in ("Falling", "Impact"):
        folder = args.output / name
        folder.mkdir(exist_ok=True)
        if any(folder.iterdir()):
            parser.error(f"Output sequence folder is nonempty: {folder}. Choose a new --output.")
    started = time.perf_counter()
    profile = None
    if args.profile:
        profile = json.loads(args.profile.read_text(encoding="utf-8-sig"))
        if profile.get("schema") != "directorscut.waterfall_profile.v1" or profile.get("units") != "centimeters":
            parser.error("Unsupported waterfall profile schema or units")
        args.width_m = min(args.width_m, float(profile["source_half_width_cm"]) * .02)
        model = FittedWaterfall(args, profile)
    else:
        model = Waterfall(args)
    manifest = {
        "schema": "directorscut.waterfall_vdb.v1",
        "generator": "Tools/generate_waterfall_vdb.py",
        "generator_sha256": digest(Path(__file__)),
        "config": {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()},
        "coordinates": {"units": "centimeters", "x": "across fall", "y": "downstream/out from cliff", "z": "up",
                        "origin": "water level vertically below lip", "pivot_at_centroid": False,
                        "lip_cm": [0, 0, args.height_m * 100],
                        "nominal_foot_cm": [0, args.downstream_m * 100, 0]},
        "frame_count": frames, "fps": args.fps, "duration_seconds": args.duration,
        "endpoint_duplicated": False, "grid": "density", "grid_type": "FloatGrid/fog volume",
        "first_frame": 0, "last_frame": frames - 1,
        "physics": {"gravity_m_s2": 9.81, "terminal_velocity_m_s": [26, 35],
                    "flight_seconds_min_max": [float(model.flight.min()), float(model.flight.max())],
                    "model": "periodic ballistic particles with drag; separate secondary droplets and drifting mist"},
        "sequences": {},
    }
    if profile:
        manifest["fitted_profile"] = {"file": str(args.profile.resolve()), "file_sha256": digest(args.profile),
                                      "profile_sha1": profile["profile_sha1"], "feed_index": profile["feed_index"],
                                      "local_origin_arena_cm": profile["local_origin_arena_cm"],
                                      "local_yaw_degrees": profile["local_yaw_degrees"],
                                      "path_length_cm": profile["path_length_cm"]}
        manifest["coordinates"]["lip_cm"] = profile["source_local_cm"]
        manifest["coordinates"]["nominal_foot_cm"] = profile["foot_local_cm"]
        manifest["physics"]["model"] = "gravity/drag path advection on actual core profile, short outward ballistic spray, current-aligned impact on sampled receiving water"
    for component in ("Falling", "Impact"):
        raster = Raster(args, component, model)
        metrics, changes, projections, side_projections = [], [], [], []
        first = previous = None
        peak, clipped = 0.0, 0.0
        for frame in range(frames):
            density, lost = raster.make(model, frame / args.fps)
            clipped = max(clipped, lost)
            peak = max(peak, float(density.max()))
            if not np.all(np.isfinite(density)) or float(density.min()) < 0:
                raise RuntimeError("Nonfinite or negative density")
            if first is None:
                first = density.copy()
            if previous is not None:
                changes.append(float(np.mean(np.abs(density - previous))))
            previous = density
            path = args.output / component / f"{component}_{frame:04d}.vdb"
            grid = raster.write(path, density, frame)
            bounds = [list(part) for part in grid.evalActiveVoxelBoundingBox()]
            expected = [raster.lo.tolist(), raster.hi.tolist()]
            if bounds != expected:
                raise RuntimeError(f"Unexpected active bounds: {bounds} != {expected}")
            metrics.append({"frame": frame, "file": str(path.relative_to(args.output)).replace("\\", "/"),
                            "bytes": path.stat().st_size, "sha256": digest(path),
                            "active_voxels": grid.activeVoxelCount(), "max_density": float(density.max()),
                            "density_sum": float(density.sum()), "clipped_particle_mass_fraction": lost})
            if frame in {0, frames // 4, frames // 2, 3 * frames // 4}:
                projections.append(np.flip(density.max(axis=1).T, axis=0))
                if profile:
                    side_projections.append(np.flip(density.max(axis=0).T, axis=0))
            if frame % max(1, frames // 8) == 0:
                print(f"{component}: {frame + 1}/{frames}, {time.perf_counter() - started:.1f}s", flush=True)
        repeat, _ = raster.make(model, args.duration)
        seam_error = float(np.max(np.abs(first - repeat)))
        wrap_step = float(np.mean(np.abs(previous - first)))
        # Validate the frame immediately across the boundary as well as exact
        # periodic equality; this detects lifecycle popping hidden by modulo.
        epsilon = .001
        before, _ = raster.make(model, args.duration - epsilon)
        after, _ = raster.make(model, epsilon)
        boundary_delta = float(np.mean(np.abs(after - before)))
        decoded = vdb.read(str(args.output / component / f"{component}_0000.vdb"), "density")
        roundtrip = np.zeros(raster.shape, dtype=np.float32)
        decoded.copyToArray(roundtrip, ijk=tuple(int(v) for v in raster.lo))
        roundtrip_error = float(np.max(np.abs(roundtrip - first)))
        if seam_error > 1e-6 or roundtrip_error > 1e-6:
            raise RuntimeError("Loop periodicity or VDB roundtrip verification failed")
        image = np.concatenate(projections, axis=1)
        image = np.uint8(np.clip((1 - np.exp(-image * 6)) * 255, 0, 255))
        image = np.repeat(np.repeat(image, 3, axis=0), 3, axis=1)
        png_gray(args.output / f"{component}_density_projection.png", image)
        if side_projections:
            side = np.concatenate(side_projections, axis=1)
            side = np.uint8(np.clip((1-np.exp(-side*6))*255, 0, 255))
            png_gray(args.output / f"{component}_side_density_projection.png", np.repeat(np.repeat(side, 2, axis=0), 2, axis=1))
        manifest["sequences"][component] = {
            "pattern": f"{component}/{component}_%04d.vdb", "voxel_size_cm": args.voxel_m * 100,
            "resolution_xyz": list(raster.shape), "index_bounds_inclusive": [raster.lo.tolist(), raster.hi.tolist()],
            "voxel_center_bounds_cm": [(raster.lo * args.voxel_m * 100).tolist(), (raster.hi * args.voxel_m * 100).tolist()],
            "voxel_outer_bounds_cm": [((raster.lo - .5) * args.voxel_m * 100).tolist(), ((raster.hi + .5) * args.voxel_m * 100).tolist()],
            "max_density": peak, "fixed_bounds_verified_every_frame": True,
            "max_clipped_particle_mass_fraction": clipped,
            "total_bytes": sum(item["bytes"] for item in metrics),
            "loop_check": {"t0_vs_period_max_abs": seam_error, "vdb_roundtrip_max_abs": roundtrip_error,
                           "wrap_frame_mean_abs_delta": wrap_step,
                           "interior_frame_mean_abs_delta_median": float(np.median(changes)),
                           "interior_frame_mean_abs_delta_p95": float(np.percentile(changes, 95)),
                           "boundary_2ms_mean_abs_delta": boundary_delta},
            "frames": metrics,
        }
    manifest["generation_seconds"] = time.perf_counter() - started
    manifest["total_vdb_bytes"] = sum(s["total_bytes"] for s in manifest["sequences"].values())
    manifest["verification"] = "CPU density/bounds/periodicity/VDB roundtrip checked; Unreal import, rendering and GPU cost unverified"
    (args.output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(args.output), "seconds": manifest["generation_seconds"],
                      "vdb_mib": manifest["total_vdb_bytes"] / 1024**2, "frames_per_sequence": frames}), flush=True)


if __name__ == "__main__":
    main()
