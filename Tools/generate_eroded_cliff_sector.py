"""Original offline, closed-boundary heightfield erosion kernel (NumPy only).

Input NPZ: float32 height_m, bedrock_m, erodibility and protected_mask grids;
positive scalar cell_size_m. Optional loose_m seeds movable surface sediment.
Optional heightfield_valid_mask excludes nonphysical padding from rainfall and
all transport. Invalid padding must also be fully protected.
Other non-object arrays (origin/basis/profile metadata) pass through unchanged.
protected_mask==1 remains bitwise fixed. Bedrock is an immutable lower bound.

D8 available-water-limited flux carries suspended sediment conservatively.
Capacity-dependent erosion/deposition exchanges solid with the terrain. A
separate repose-limited thermal flux moves only tracked loose material. All
boundaries are closed; water, suspended sediment and loose sediment are saved.
This is an accelerated morphology tool, not a calibrated fluid/weather solver.

Example: python Tools/generate_eroded_cliff_sector.py --input sector.npz
         --output eroded.npz --steps 256 --seed 71421
Demo only: --demo-size 128 --output Saved/Erosion/demo.npz --steps 128
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import time

import numpy as np


@dataclass(frozen=True)
class Settings:
    steps: int = 256
    seed: int = 71421
    dt: float = 1.0
    rainfall_m: float = .002
    rainfall_jitter: float = .20
    flow_rate: float = 1.5
    water_fraction: float = .65
    sediment_capacity: float = 4.0
    erosion_rate: float = .25
    deposition_rate: float = .35
    evaporation_rate: float = .01
    thermal_rate: float = .20
    repose_degrees: float = 38.0
    initial_loose_m: float = .5
    max_erosion_cell_fraction: float = .025


def neighbours(shape):
    """Eight directed internal links; no np.roll wrapping or external outflow."""
    rows, cols = shape
    for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1),
                   (-1, -1), (-1, 1), (1, -1), (1, 1)):
        src = (slice(max(0, -dy), min(rows, rows-dy)),
               slice(max(0, -dx), min(cols, cols-dx)))
        dst = (slice(max(0, dy), min(rows, rows+dy)),
               slice(max(0, dx), min(cols, cols+dx)))
        yield src, dst, float(np.hypot(dy, dx))


def validate_seed(data):
    required = ("height_m", "bedrock_m", "erodibility", "protected_mask")
    for name in required:
        if name not in data:
            raise ValueError(f"Missing required grid: {name}")
        if data[name].dtype != np.float32 or data[name].ndim != 2:
            raise ValueError(f"{name} must be a 2D float32 array")
        if not np.isfinite(data[name]).all():
            raise ValueError(f"{name} contains non-finite values")
    shape = data["height_m"].shape
    if min(shape) < 3 or any(data[n].shape != shape for n in required):
        raise ValueError("Required grids must share a shape of at least 3x3")
    for name in ("erodibility", "protected_mask"):
        if np.any((data[name] < 0) | (data[name] > 1)):
            raise ValueError(f"{name} must lie in [0, 1]")
    if np.any(data["height_m"] < data["bedrock_m"]):
        raise ValueError("Initial height is below immutable bedrock")
    cell = np.asarray(data.get("cell_size_m", np.nan))
    if cell.size != 1 or not np.isfinite(cell).all() or float(cell.item()) <= 0:
        raise ValueError("cell_size_m must be one positive finite scalar")
    if "loose_m" in data:
        loose = data["loose_m"]
        if loose.shape != shape or not np.isfinite(loose).all() or np.any(loose < 0):
            raise ValueError("loose_m must be a finite nonnegative matching grid")
        if np.any(loose.astype(np.float64) > data["height_m"].astype(np.float64)-data["bedrock_m"]+1.e-6):
            raise ValueError("loose_m exceeds the material above bedrock")
    if "heightfield_valid_mask" in data:
        domain = data["heightfield_valid_mask"]
        if domain.shape != shape or not np.isfinite(domain).all() or np.any((domain != 0) & (domain != 1)):
            raise ValueError("heightfield_valid_mask must be a matching binary grid")
        if np.any((domain == 0) & (data["protected_mask"] != 1)):
            raise ValueError("Nonphysical padding must be fully protected")
    if any(value.dtype.hasobject for value in data.values()):
        raise ValueError("Object/pickled metadata is unsupported")
    return float(cell.item())


def validate_settings(cfg):
    if cfg.steps < 0 or cfg.dt <= 0 or not all(np.isfinite(v) for v in asdict(cfg).values()):
        raise ValueError("Steps must be nonnegative and all settings finite; dt must be positive")
    if not 0 < cfg.water_fraction <= 1 or not 0 <= cfg.rainfall_jitter <= 1:
        raise ValueError("water_fraction must be in (0,1], rainfall_jitter in [0,1]")
    if not 0 < cfg.repose_degrees < 90:
        raise ValueError("repose_degrees must lie in (0,90)")
    for name in ("rainfall_m", "flow_rate", "sediment_capacity", "erosion_rate",
                 "deposition_rate", "evaporation_rate", "thermal_rate", "initial_loose_m",
                 "max_erosion_cell_fraction"):
        if getattr(cfg, name) < 0:
            raise ValueError(f"{name} must be nonnegative")


def erode(data, cfg=Settings(), progress=None):
    """Return output arrays and metrics without mutating any input arrays."""
    cell = validate_seed(data)
    validate_settings(cfg)
    initial = data["height_m"].copy()
    height = initial.astype(np.float64)
    bedrock = data["bedrock_m"].astype(np.float64)
    erodibility = data["erodibility"].astype(np.float64)
    mobility = 1.-data["protected_mask"].astype(np.float64)
    fixed = mobility == 0
    domain = np.asarray(data.get("heightfield_valid_mask", np.ones(height.shape, dtype=bool)), dtype=bool)
    loose = (data["loose_m"].astype(np.float64).copy() if "loose_m" in data else
             np.minimum(height-bedrock, cfg.initial_loose_m)*erodibility*mobility)
    water = np.zeros_like(height)
    sediment = np.zeros_like(height)
    eroded = np.zeros_like(height)
    deposited = np.zeros_like(height)
    accumulation = np.zeros_like(height)
    flux = np.zeros((8, *height.shape), dtype=np.float64)
    incoming_water = np.zeros_like(height)
    incoming_sediment = np.zeros_like(height)
    links = tuple(neighbours(height.shape))
    rng = np.random.Generator(np.random.PCG64(cfg.seed))
    erosion_alpha = -np.expm1(-cfg.erosion_rate*cfg.dt)
    deposition_alpha = -np.expm1(-cfg.deposition_rate*cfg.dt)
    evaporation = -np.expm1(-cfg.evaporation_rate*cfg.dt)
    thermal_alpha = min(1., cfg.thermal_rate*cfg.dt)/8.
    repose = np.tan(np.deg2rad(cfg.repose_degrees))
    rainfall_total = evaporated_total = 0.
    initial_solid = float(np.sum(height-bedrock, dtype=np.float64))
    started = time.perf_counter()
    for step in range(cfg.steps):
        rain = cfg.rainfall_m*cfg.dt*rng.uniform(1-cfg.rainfall_jitter, 1+cfg.rainfall_jitter, height.shape)
        rain *= domain
        water += rain
        rainfall_total += float(np.sum(rain, dtype=np.float64))
        head = height+water
        max_water_slope = np.zeros_like(height)
        max_ground_slope = np.zeros_like(height)
        flux.fill(0)
        for k, (src, dst, distance) in enumerate(links):
            valid_link = domain[src] & domain[dst]
            slope = np.maximum((head[src]-head[dst])/(cell*distance), 0.)*valid_link
            flux[k][src] = slope
            np.maximum(max_water_slope[src], slope, out=max_water_slope[src])
            np.maximum(max_ground_slope[src], (height[src]-height[dst])/(cell*distance)*valid_link, out=max_ground_slope[src])
        weights = flux.sum(axis=0)
        budget = np.minimum(water*cfg.water_fraction, cfg.flow_rate*cfg.dt*max_water_slope)
        scale = np.divide(budget, weights, out=np.zeros_like(height), where=weights > 0)
        flux *= scale[None, :, :]
        outgoing_water = flux.sum(axis=0)
        concentration = np.divide(sediment, water, out=np.zeros_like(height), where=water > 0)
        incoming_water.fill(0)
        incoming_sediment.fill(0)
        for k, (src, dst, _) in enumerate(links):
            incoming_water[dst] += flux[k][src]
            incoming_sediment[dst] += flux[k][src]*concentration[src]
        water += incoming_water-outgoing_water
        sediment += incoming_sediment-outgoing_water*concentration
        accumulation += outgoing_water*cell*cell

        # Capacity has equivalent-solid-depth units. A steep flowing reach can
        # erode; a flatter receiving reach deposits transported material.
        throughput = np.maximum(outgoing_water, incoming_water)
        capacity = cfg.sediment_capacity*throughput*np.maximum(max_ground_slope, .005)
        erosion = np.minimum(np.maximum(capacity-sediment, 0.)*erosion_alpha*erodibility*mobility,
                             np.minimum(height-bedrock, cell*cfg.max_erosion_cell_fraction))
        deposition = np.maximum(sediment-capacity, 0.)*deposition_alpha*mobility
        height += deposition-erosion
        sediment += erosion-deposition
        loose = np.maximum(loose-erosion, 0.)+deposition
        eroded += erosion
        deposited += deposition

        # Repose-limited D8 thermal transport only moves tracked loose material.
        # Each directed flux is excluded at fixed source AND destination pads.
        flux.fill(0)
        for k, (src, dst, distance) in enumerate(links):
            excess = np.maximum(height[src]-height[dst]-repose*cell*distance, 0.)
            flux[k][src] = excess*thermal_alpha*mobility[src]*mobility[dst]*(domain[src] & domain[dst])
        demand = flux.sum(axis=0)
        thermal_scale = np.divide(np.minimum(loose, demand), demand, out=np.zeros_like(height), where=demand > 0)
        flux *= thermal_scale[None, :, :]
        outgoing = flux.sum(axis=0)
        incoming_water.fill(0)  # reuse scratch; this now holds incoming solids
        for k, (src, dst, _) in enumerate(links):
            incoming_water[dst] += flux[k][src]
        change = incoming_water-outgoing
        height += change
        loose += change
        eroded += outgoing
        deposited += incoming_water
        evaporated_total += float(np.sum(water, dtype=np.float64))*evaporation
        water *= 1.-evaporation
        if progress and ((step+1) % 32 == 0 or step+1 == cfg.steps):
            progress(step+1, cfg.steps, time.perf_counter()-started)

    # Check solver mass BEFORE float32 serialization. No clipping hides a
    # negative reservoir or unauthorized pad edit.
    for name, field in (("height", height), ("water", water), ("sediment", sediment), ("loose", loose)):
        if not np.isfinite(field).all():
            raise ArithmeticError(f"Non-finite solver {name}")
    if min(float(water.min()), float(sediment.min()), float(loose.min()), float((height-bedrock).min())) < -1.e-8:
        raise ArithmeticError("Negative reservoir or bedrock violation")
    if not np.array_equal(height[fixed], initial[fixed].astype(np.float64)):
        raise ArithmeticError("Protected height changed internally")
    solid_error = float(np.sum(height-bedrock)+np.sum(sediment)-initial_solid)
    water_error = float(np.sum(water)-rainfall_total+evaporated_total)
    if abs(solid_error) > max(1.e-7, initial_solid*1.e-10):
        raise ArithmeticError(f"Solid mass imbalance: {solid_error*cell*cell} m3")
    if abs(water_error) > max(1.e-8, rainfall_total*1.e-10):
        raise ArithmeticError(f"Water budget imbalance: {water_error*cell*cell} m3")
    if np.max(np.abs(height-(initial.astype(np.float64)-eroded+deposited))) > 1.e-7:
        raise ArithmeticError("Gross erosion/deposition does not reconstruct final height")

    final = height.astype(np.float32)
    final[fixed] = initial[fixed]  # retain signed zero bits too
    if not np.array_equal(final[fixed].view(np.uint32), initial[fixed].view(np.uint32)):
        raise ArithmeticError("Protected height is not bitwise fixed")
    grad_y, grad_x = np.gradient(height, cell)
    angle = np.arctan(np.hypot(grad_x, grad_y))
    curvature = np.gradient(grad_x, cell, axis=1)+np.gradient(grad_y, cell, axis=0)
    # Two gradient stencils are used for curvature. Suppress diagnostic masks
    # beside nonphysical padding rather than interpreting its fill height as rock.
    diagnostic_valid = domain.copy()
    for _ in range(2):
        valid_before = diagnostic_valid.copy()
        for src, dst, _ in links:
            diagnostic_valid[src] &= valid_before[dst]
    angle[~diagnostic_valid] = 0
    curvature[~diagnostic_valid] = 0
    flow_log = np.log1p(accumulation)
    arrays = {key: value.copy() for key, value in data.items()}
    arrays.update(height_m=final, water_m=water.astype(np.float32), suspended_m=sediment.astype(np.float32),
                  loose_m=loose.astype(np.float32), eroded_m=eroded.astype(np.float32), deposited_m=deposited.astype(np.float32),
                  flow_accumulation=accumulation.astype(np.float32),
                  flow_mask=(flow_log/max(float(flow_log.max()), 1.e-12)).astype(np.float32),
                  slope=(angle/(np.pi*.5)).astype(np.float32), slope_degrees=np.rad2deg(angle).astype(np.float32),
                  curvature=np.tanh(curvature*cell).astype(np.float32), curvature_m_inv=curvature.astype(np.float32))
    arrays["diagnostic_valid_mask"] = diagnostic_valid.astype(np.uint8)
    serial_error = float(np.sum(final.astype(np.float64)-bedrock)+np.sum(arrays["suspended_m"], dtype=np.float64)-initial_solid)*cell*cell
    quantization = .5*cell*cell*float(np.sum(np.abs(np.spacing(final)), dtype=np.float64)
                                    +np.sum(np.abs(np.spacing(arrays["suspended_m"])), dtype=np.float64))
    if abs(serial_error) > quantization+abs(solid_error)*cell*cell+1.e-7:
        raise ArithmeticError("Serialized mass error exceeds float32 rounding bound")
    metrics = dict(shape=list(height.shape), steps=cfg.steps, elapsed_seconds=time.perf_counter()-started,
                   boundary="closed D8", fixed_cells=int(np.count_nonzero(fixed)),
                   physical_cells=int(np.count_nonzero(domain)), excluded_padding_cells=int(np.count_nonzero(~domain)),
                   solver_solid_mass_error_m3=solid_error*cell*cell, serialized_solid_mass_error_m3=serial_error,
                   float32_quantization_bound_m3=quantization, water_budget_error_m3=water_error*cell*cell,
                   rainfall_m3=rainfall_total*cell*cell, evaporated_m3=evaporated_total*cell*cell,
                   suspended_m3=float(np.sum(sediment))*cell*cell,
                   gross_eroded_m3=float(np.sum(eroded))*cell*cell, gross_deposited_m3=float(np.sum(deposited))*cell*cell,
                   max_erosion_depth_m=float(np.max(initial-height)), max_deposition_height_m=float(np.max(height-initial)),
                   minimum_bedrock_clearance_m=float(np.min(height-bedrock)), settings=asdict(cfg))
    arrays["metrics_json"] = np.array(json.dumps(metrics, sort_keys=True))
    arrays["erosion_metadata_json"] = np.array(json.dumps({
        "generator": "original_numpy_d8_erosion_v1", "seed": cfg.seed, "numpy": np.__version__,
        "height_units": "absolute metres, unchanged input frame", "flow_accumulation_units": "cumulative outgoing water m3",
        "slope": "atan(gradient)/(pi/2), [0,1]", "curvature": "tanh(laplacian(height)*cell_size), [-1,1], positive concave",
        "eroded_m_deposited_m": "gross thickness including hydraulic and thermal exchange",
        "limits": "heightfield only; no overhangs or momentum; accelerated morphology; closed boundaries pond water; fixed pads are immutable",
    }, sort_keys=True))
    return arrays, metrics


def demo_seed(size=128, cell=2.):
    """Explicit synthetic test slope; never presented as actual crater data."""
    y, x = np.mgrid[:size, :size].astype(np.float64)*cell
    width = max(cell*(size-1), cell)
    height = 100.+.35*y+5.*np.exp(-((x-.45*width)/(.12*width))**2)+.3*np.sin(x*.19+y*.04)
    protected = np.zeros((size, size), np.float32)
    protected[[0, -1], :] = 1
    protected[:, [0, -1]] = 1
    protected[size//3:size//3+5, size//2:size//2+5] = 1
    return dict(height_m=height.astype(np.float32), bedrock_m=(height-10).astype(np.float32),
                erodibility=np.full((size, size), .8, np.float32), protected_mask=protected,
                cell_size_m=np.array(cell, np.float32), source_kind=np.array("DEMO synthetic slope; not actual crater"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--input", type=Path)
    source.add_argument("--demo-size", type=int)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--steps", type=int, default=256)
    parser.add_argument("--seed", type=int, default=71421)
    parser.add_argument("--rainfall-m", type=float, default=.002, help="Accelerated rain depth per model time unit")
    parser.add_argument("--thermal-rate", type=float, default=.20)
    parser.add_argument("--sediment-capacity", type=float, default=4.0)
    parser.add_argument("--erosion-rate", type=float, default=.25)
    parser.add_argument("--deposition-rate", type=float, default=.35)
    parser.add_argument("--max-erosion-cell-fraction", type=float, default=.025)
    args = parser.parse_args()
    if args.input and args.input.resolve() == args.output.resolve():
        parser.error("Input seed is immutable; choose a different output path")
    if args.demo_size is not None and args.demo_size < 3:
        parser.error("Demo grid must be at least 3x3")
    if args.input:
        with np.load(args.input, allow_pickle=False) as source_file:
            data = {key: source_file[key].copy() for key in source_file.files}
    else:
        data = demo_seed(args.demo_size)
        print("DEMO ONLY: synthetic test slope, not actual crater geometry", flush=True)
    cfg = Settings(steps=args.steps, seed=args.seed, rainfall_m=args.rainfall_m, thermal_rate=args.thermal_rate,
                   sediment_capacity=args.sediment_capacity, erosion_rate=args.erosion_rate,
                   deposition_rate=args.deposition_rate, max_erosion_cell_fraction=args.max_erosion_cell_fraction)
    arrays, metrics = erode(data, cfg, lambda step, total, elapsed: print(f"erosion {step}/{total} ({elapsed:.1f}s)", flush=True))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(args.output, **arrays)
    print(json.dumps(metrics, indent=2, sort_keys=True))
    print(f"Saved {args.output}")


if __name__ == "__main__":
    main()
