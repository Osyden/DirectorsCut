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
import copy
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
        self.ballistic_launch = profile.get("ballistic_launch")
        if self.ballistic_launch:
            self.ballistic_sources = np.asarray(self.ballistic_launch["cross_sources_local_cm"], dtype=float) / 100
            self.ballistic_offsets = np.asarray(self.ballistic_launch["cross_offsets_cm"], dtype=float) / 100
            self.ballistic_velocity = np.asarray(self.ballistic_launch["initial_velocity_local_cm_s"], dtype=float) / 100
            if (self.ballistic_sources.shape != (len(self.ballistic_offsets), 3)
                    or not np.all(np.diff(self.ballistic_offsets)>0)
                    or not np.isfinite(self.ballistic_sources).all()
                    or not np.isfinite(self.ballistic_velocity).all()
                    or not math.isclose(float(self.ballistic_launch["gravity_cm_s2"]), 981.0)):
                raise ValueError("Invalid physical ballistic launch contract")
        water = profile["receiving_water"]
        if "coverage_y_major" not in water:
            raise ValueError("Profile lacks receiving-water coverage; re-export the contact-trimmed core")
        self.water_count = int(water["count"])
        self.water_origin = np.asarray(water["origin_local_cm"][:2], dtype=float) / 100
        self.water_step = float(water["step_cm"]) / 100
        self.water_heights = np.asarray(water["heights_local_cm_y_major"], dtype=float).reshape(self.water_count, self.water_count) / 100
        self.water_coverage = np.asarray(water["coverage_y_major"], dtype=float).reshape(self.water_count, self.water_count)
        self.water_shore = None
        if "shore_clearance_cm_y_major" in water:
            self.water_shore = np.asarray(water["shore_clearance_cm_y_major"], dtype=float).reshape(self.water_count, self.water_count) / 100
        super().__init__(args)
        self.hero_breakup = bool(self.ballistic_launch and getattr(args, "hero_breakup", False))
        if self.hero_breakup:
            # Three unequal physical source channels, advected with their birth
            # phase. Preserve total emitted density mass; do not brighten the
            # fall by adding a uniform blanket of particles.
            original_mass = self.strength.sum()
            phase = 2*np.pi*self.birth/args.duration
            channel = (.04 + np.exp(-.5*((self.x+2.35)/.72)**2)
                       + .72*np.exp(-.5*((self.x-.05)/.49)**2)
                       + .85*np.exp(-.5*((self.x-2.45)/.81)**2))
            channel *= .72+.28*np.sin(phase+self.x*.83)**2
            self.strength *= channel
            self.strength *= original_mass/self.strength.sum()
            self.birth_phase = phase
            # Unequal sideways impact jets, correlated across the curtain.
            # Keep their bounded lateral footprint: the ship route is nearby.
            self.impact_vx = 4.2*np.sin(phase*2+self.x*.65)
            self.impact_vy = 2.0+3.0*(.5+.5*np.sin(phase+self.x*.9))
            self.impact_vz = 5.5+7.0*(.5+.5*np.sin(phase*2-self.x*.7))
            self.impact_drag = np.minimum(self.impact_drag,1.25)
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
        self.path_end_age = times[-1] / self.speed_scale
        self.flight = self.path_end_age + 1.5
        self.profile_ready = True
        # Locate each strand's first actual water entry. Lateral tracks and the
        # small air-side offset can contact at different times. A short ballistic
        # continuation handles a profile endpoint just above the receiving sea.
        self.flight, self.contact_valid = self.first_water_hit(self.fall_positions, self.flight, steps=128)
        if not np.any(self.contact_valid):
            raise ValueError("No fitted strand reaches covered receiving water; inspect ContactV/coverage export")
        self.contact_stats = {"strands": len(self.contact_valid), "strands_with_water_entry": int(self.contact_valid.sum()),
                              "unmatched_source_mass_fraction": float(self.strength[~self.contact_valid].sum()/max(self.strength.sum(), 1e-9))}
        self.impact_birth = np.mod(self.birth + self.flight, args.duration)
        self.foot = self.fall_positions(self.flight)
        entry_height, _ = self.water_surface(self.foot[:, :2])
        entry_depth = np.maximum(0, entry_height-self.foot[:, 2])[self.contact_valid]
        self.contact_stats.update({"entry_depth_m_median": float(np.median(entry_depth)),
                                   "entry_depth_m_p95": float(np.percentile(entry_depth, 95)),
                                   "entry_depth_m_max": float(entry_depth.max()),
                                   "surface_impact_model": "surface response at entry XY/time; no simulated underwater bubble transit"})
        # The submerged core endpoint is never lifted. Surface spray is a
        # separate response, anchored above the water at the entry XY/time.
        self.foot[:, 2] = entry_height + .15
        current = np.asarray(profile["current_local_cm_s"], dtype=float) / 100
        current[2] = 0
        self.current = current
        self.along = current / max(np.linalg.norm(current), 1e-6)
        self.across = np.array((self.along[1], -self.along[0], 0))
        self.impact_life, _ = self.first_water_hit(self.impact_positions, self.impact_life, steps=64, start_age=.03)
        # A separate subset leaves the core as short ballistic fragments. The
        # density remains close to the source rather than becoming rising smoke.
        self.spray_indices = np.arange(0, len(self.birth), 5)
        self.spray_launch_time = times[-1] * (.12 + .72 * (self.phase[:, 2] / (2 * np.pi)))
        if self.ballistic_launch:
            self.spray_launch_time = self.flight * (.10 + .72 * (self.phase[:, 2] / (2 * np.pi)))
            if self.hero_breakup:
                self.spray_indices = np.arange(0,len(self.birth),2)
                self.spray_launch_time = self.flight*(.40+.48*(self.phase[:,2]/(2*np.pi)))
        self.spray_birth = np.mod(self.birth + (self.spray_launch_time if self.ballistic_launch else self.spray_launch_time / self.speed_scale), args.duration)

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

    def water_surface(self, xy):
        n = self.water_count
        index = (xy - self.water_origin) / self.water_step
        inside = np.all((index >= 0) & (index <= n-1), axis=1)
        index = np.clip(index, 0, n - 1.000001)
        cell = np.floor(index).astype(int)
        f = index - cell
        x, y = cell[:, 0], cell[:, 1]
        def sample(grid):
            return ((grid[y, x]*(1-f[:, 0])+grid[y, x+1]*f[:, 0])*(1-f[:, 1])
                    +(grid[y+1, x]*(1-f[:, 0])+grid[y+1, x+1]*f[:, 0])*f[:, 1])
        covered = sample(self.water_shore) >= 0 if self.water_shore is not None else sample(self.water_coverage) >= .5
        return sample(self.water_heights), inside & covered

    def first_water_hit(self, trajectory, max_age, steps, start_age=0):
        found = np.zeros(len(max_age), dtype=bool)
        lo = np.full(len(max_age), start_age)
        hi = max_age.copy()
        previous = lo.copy()
        for step in range(1, steps+1):
            age = start_age + (max_age-start_age)*(step/steps)
            p = trajectory(age)
            height, covered = self.water_surface(p[:, :2])
            new = ~found & covered & (p[:, 2] <= height)
            lo[new], hi[new] = previous[new], age[new]
            found |= new
            previous = age
        for _ in range(20):
            age = (lo+hi)*.5
            p = trajectory(age)
            height, covered = self.water_surface(p[:, :2])
            below = covered & (p[:, 2] <= height)
            hi = np.where(found & below, age, hi)
            lo = np.where(found & ~below, age, lo)
        return np.where(found, hi, max_age), found

    def impact_positions(self, age):
        travel = self.impact_drag*(1-np.exp(-age/self.impact_drag))
        p = self.foot.copy()
        p += self.across*(self.impact_vx*travel)[:, None]
        p += self.along*(self.impact_vy*travel)[:, None] + self.current*(age*.25)[:, None]
        p[:, 2] += ((self.impact_vz+self.impact_g*self.impact_drag)*travel-self.impact_g*self.impact_drag*age)
        return p

    def keep_airborne(self, p, mass):
        height, covered = self.water_surface(p[:, :2])
        mass *= np.where(covered, smoothstep(0, .12, p[:, 2]-height), 1.0)
        active = mass > 0
        return p[active], mass[active]

    def fall_positions(self, age):
        if not getattr(self, "profile_ready", False):
            return super().fall_positions(age)
        if self.ballistic_launch:
            source = np.column_stack([np.interp(self.x, self.ballistic_offsets, self.ballistic_sources[:, axis]) for axis in range(3)])
            p = source + self.ballistic_velocity * age[:, None]
            p[:, 2] -= .5 * 9.81 * age ** 2
            # Modest coherent lateral breakup after launch; gravity and feeder
            # momentum set the full drop. Never stretch an80m cache to fit it.
            spread = .03 * np.maximum(0, age-.25)**2
            p[:, 0] += spread*np.sin(self.phase[:, 0])
            p[:, 1] += spread*.6*np.sin(self.phase[:, 1])
            if getattr(self,"hero_breakup",False):
                lower = smoothstep(1.9,5.0,age)
                # Continuous birth-phase waves move with the falling water.
                # The unequal channels separate/merge, without periodic missing
                # rectangles or lifting particles against gravity.
                p[:,0] += lower*(2.15*np.sin(age*.96+self.birth_phase+self.x*.49)
                                 +.65*np.sin(age*1.73-self.birth_phase*2+self.x*.83))
                p[:,1] += lower*(.70+.65*np.sin(age*1.1+self.birth_phase+self.x*.6))
            return p
        clock = np.minimum(age * self.speed_scale, self.path_time[-1])
        p = self.sample_width(clock, self.x + .25 * np.sin(age * 2.3 + self.phase[:, 0]))
        outward = .20 + np.abs(self.y) * .6 + .18 * np.sin(age * 3.1 + self.phase[:, 1])
        # Pure radial clearance matches the core's contact-safe offset. Raising
        # a sample in Z can put it inside a downward-facing rock ledge.
        p[:, 1] += outward
        coast = np.maximum(0, age-self.path_end_age)
        p += self.path_tangent[-1] * (self.path_speed[-1]*self.speed_scale*coast)[:, None]
        p[:, 2] -= .5*9.81*coast**2
        return p

    def particles(self, time_s, component):
        if component == "Falling":
            p, mass = super().particles(time_s, component)
            indices = self.spray_indices
            age = np.mod(round(time_s % self.a.duration, 12) - self.spray_birth[indices], self.a.duration)
            launch_age = self.spray_launch_time[indices] if self.ballistic_launch else self.spray_launch_time[indices]/self.speed_scale[indices]
            active = (age < .85) & (launch_age < self.flight[indices])
            if self.hero_breakup:
                active = (age < 1.75) & (launch_age < self.flight[indices])
            indices, age = indices[active], age[active]
            launch = self.spray_launch_time[indices]
            normal = np.tile((0.0, 1.0, 0.0), (len(indices), 1))
            if self.ballistic_launch:
                start = self.fall_positions(self.spray_launch_time)[indices]
                launch_velocity = np.broadcast_to(self.ballistic_velocity,(len(indices),3)).copy()
                launch_velocity[:,2] -= 9.81*launch
            else:
                start = self.sample_width(launch, self.x[indices])
                tangent = np.column_stack([np.interp(launch, self.path_time, self.path_tangent[:, axis]) for axis in range(3)])
                speed = np.interp(launch, self.path_time, self.path_speed) * self.speed_scale[indices]
                launch_velocity = tangent*speed[:,None]
            travel = .6 * (1 - np.exp(-age / .6))
            spray = start + launch_velocity * travel[:, None] + normal * (1.5 + 3 * travel)[:, None]
            spray[:, 2] -= .5 * 9.81 * age ** 2
            expected = self.sample_width(np.minimum(launch + age * self.speed_scale[indices], self.path_time[-1]), self.x[indices])
            clearance = np.ones(len(indices)) if self.ballistic_launch else spray[:, 1] - expected[:, 1]
            fade = smoothstep(0, .08, age) * (1-smoothstep(.55, .85, age)) * smoothstep(0, .4, clearance)
            spray_mass = self.mass * self.strength[indices] * fade * .35
            if self.hero_breakup:
                # Drag slows small released droplets, producing a lower fan
                # around the still accelerating core. Coherent eddies are
                # bounded to a few metres instead of a smoke-like giant cloud.
                tau=.85
                travel=tau*(1-np.exp(-age/tau))
                phase=self.birth_phase[indices]
                kick=np.column_stack((2.8*np.sin(phase+self.x[indices]*.8),
                                      1.2+1.2*np.sin(phase*2+self.x[indices]*.6)**2,
                                      np.zeros(len(indices))))
                spray=start+(launch_velocity+kick)*travel[:,None]
                spray[:,2] -= 9.81*tau*(age-travel)
                eddy=np.sin(np.pi*np.minimum(age/1.75,1))
                spray[:,0] += eddy*1.1*np.sin(phase+age*2.4)
                spray[:,1] += eddy*.8*np.sin(phase*2-age*1.8)
                fade=smoothstep(0,.10,age)*(1-smoothstep(1.15,1.75,age))
                spray_mass=self.mass*self.strength[indices]*fade*.30
            return self.keep_airborne(np.concatenate((p, spray)), np.concatenate((mass, spray_mass)))
        positions, masses = [], []
        base_age = np.mod(round(time_s % self.a.duration, 12) - self.impact_birth, self.a.duration)
        for cycle in range(math.ceil(float(self.impact_life.max()) / self.a.duration) + 1):
            age = base_age + cycle * self.a.duration
            p = self.impact_positions(age)
            fade = smoothstep(0, .07, age) * (1-smoothstep(self.impact_life-.35, self.impact_life, age))
            mass = self.mass * self.strength * fade * self.impact_mass
            active = (age < self.impact_life) & self.contact_valid
            positions.append(p[active]); masses.append(mass[active])
        return self.keep_airborne(np.concatenate(positions), np.concatenate(masses))

    def bounds(self, component):
        low, high = np.full(3, np.inf), np.full(3, -np.inf)
        for t in np.linspace(0, self.a.duration, 16, endpoint=False):
            p, mass = self.particles(float(t), component)
            p = p[mass > 1e-8]
            low = np.minimum(low, p.min(axis=0)); high = np.maximum(high, p.max(axis=0))
        return low - 4, high + 4


class CrestWaterfalls(FittedWaterfall):
    """Several separately fitted outlets in one unchanged physical SVT frame."""
    def __init__(self,args,profile):
        self.a=args;self.profile=profile;self.models=[];self.transforms=[]
        origin=np.asarray(profile['local_origin_arena_cm'])/100
        basis=np.array([profile['basis_x_arena'],profile['basis_y_arena'],profile['basis_z_arena']]).T
        outlets=profile['ballistic_outlets']
        total_width=sum(float(p['source_half_width_cm'])*.02 for p in outlets)
        for i,p in enumerate(outlets):
            local=copy.copy(args);local.width_m=float(p['source_half_width_cm'])*.02
            local.emission_rate=args.emission_rate*local.width_m/total_width
            local.seed=args.seed+104729*i
            model=FittedWaterfall(local,p);self.models.append(model)
            b=np.array([p['basis_x_arena'],p['basis_y_arena'],p['basis_z_arena']]).T
            self.transforms.append((b.T@basis,(np.asarray(p['local_origin_arena_cm'])/100-origin)@basis))
        # The first outlet is the reference frame and its measured receiving
        # surface spans the entire group. Keep coverage, not just height.
        if not np.allclose(self.transforms[0][0],np.eye(3)) or not np.allclose(self.transforms[0][1],0):
            raise ValueError('First crest outlet must define the shared frame')
        self.flight=np.concatenate([m.flight for m in self.models])
        self.ballistic_launch=profile['ballistic_launch']
        self.contact_stats=dict(strands=sum(m.contact_stats['strands'] for m in self.models),
            strands_with_water_entry=sum(m.contact_stats['strands_with_water_entry'] for m in self.models),
            unmatched_source_mass_fraction=max(m.contact_stats['unmatched_source_mass_fraction'] for m in self.models),
            entry_depth_m_max=max(m.contact_stats['entry_depth_m_max'] for m in self.models),
            separate_outlets=len(outlets),outlet_contact_stats=[m.contact_stats for m in self.models])

    def water_surface(self,xy):
        return self.models[0].water_surface(xy)

    def particles(self,time_s,component):
        points=[];masses=[]
        for model,(rotation,offset) in zip(self.models,self.transforms):
            p,m=model.particles(time_s,component);points.append(p@rotation+offset);masses.append(m)
        return np.concatenate(points),np.concatenate(masses)


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
        self.receiving_height = self.receiving_coverage = None
        if isinstance(model, FittedWaterfall):
            x, y = np.meshgrid((np.arange(self.shape[0])+self.lo[0])*args.voxel_m,
                               (np.arange(self.shape[1])+self.lo[1])*args.voxel_m, indexing="ij")
            height, covered = model.water_surface(np.column_stack((x.ravel(), y.ravel())))
            self.receiving_height = height.reshape(self.shape[:2])
            self.receiving_coverage = covered.reshape(self.shape[:2])

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
        if self.receiving_height is not None:
            # Raster reconstruction must respect contact too; otherwise a blur
            # kernel reintroduces airborne mist below the sea after particle kill.
            for z in range(self.shape[2]):
                below = self.receiving_coverage & ((z+self.lo[2])*self.a.voxel_m <= self.receiving_height)
                density[:, :, z][below] = 0
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
    parser.add_argument("--hero-breakup", action="store_true", help="bounded unequal channels and lower aerodynamic spray for an explicit ballistic hero profile")
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
        model = CrestWaterfalls(args,profile) if profile.get('ballistic_outlets') else FittedWaterfall(args, profile)
    else:
        if args.hero_breakup:
            parser.error("--hero-breakup requires an explicit ballistic --profile")
        model = Waterfall(args)
    if args.hero_breakup and not getattr(model,"ballistic_launch",None):
        parser.error("--hero-breakup requires a ballistic_launch profile")
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
        manifest["coordinates"]["origin"] = "explicit fitted_profile.local_origin_arena_cm; do not infer it from bounds"
        manifest["physics"]["model"] = "gravity/drag path advection on actual core profile, short outward ballistic spray, current-aligned impact on sampled receiving water"
        if profile.get("ballistic_launch"):
            manifest["physics"]["model"] = "measured high-rim source with authored horizontal feeder momentum and gravity; secondary spray acquires drag after release"
            manifest["physics"]["ballistic_launch"] = profile["ballistic_launch"]
            manifest["physics"].pop("terminal_velocity_m_s",None)
            manifest["physics"]["core_drag"] = "none; source momentum plus gravity; drag applies to secondary spray"
            if profile.get('ballistic_outlets'):
                manifest['physics']['separate_outlets']=[dict(source_cm=p['source_local_cm'],
                    origin_arena_cm=p['local_origin_arena_cm'],width_m=p['source_half_width_cm']*.02,
                    launch=p['ballistic_launch']) for p in profile['ballistic_outlets']]
            if args.hero_breakup:
                manifest["physics"]["hero_breakup"] = {
                    "source_channels": 3, "source_density_mass_preserved": True,
                    "core_additional_lateral_envelope_m": 2.8,
                    "core_additional_outward_envelope_m": 1.35,
                    "secondary_lifetime_s": 1.75, "secondary_drag_s": .85,
                    "secondary_fraction": .5, "secondary_relative_density_weight": .30,
                    "model_limit": "authored aerodynamic density response, not conserved two-phase liquid mass; whole-rock/route sampling checked separately"}
        manifest["water_contact"] = {**model.contact_stats, "coverage_required": True,
                                      "outside_height_grid_is_water": False, "post_blur_submerged_density_mask": True,
                                      "core_endpoint_lift": False, "impact_timing": "first actual covered water entry per strand"}
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
