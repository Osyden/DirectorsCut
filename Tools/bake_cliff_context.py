"""Bake an authored unique full-ring context material, with no source pixels.

UV0 is the accepted cliff's unwrapped arc/497.4188m, height/280m chart.
Broad low-contrast strata/mineral weathering are artistic masks, not simulated
erosion, measured geology, or actual surface-slope data. No geometry changes.
The only U repetition is the complete 10km ring circumference, preserving its
angular seam; there are no repeated 500m panels. Normal maps use GL/+V-up.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import zlib
import numpy as np


ARC_METRES = 497.418836818384
CIRCUMFERENCE = 2*np.pi*1600.
UV_MIN = np.array([-7., -1.25])
UV_MAX = np.array([13.3, 2.35])


def hash2(i, j, seed):
    q = (i.astype(np.uint32)*np.uint32(374761393)
         + j.astype(np.uint32)*np.uint32(668265263)
         + np.uint32(seed*2246822519 % 2**32))
    q = (q ^ (q >> 13))*np.uint32(1274126177)
    return ((q ^ (q >> 16)) & np.uint32(0xffffff)).astype(np.float32)/8388608.-1.


def noise(u, z, sx, sz, seed):
    """Smooth lattice field; continuous and periodic only around the full ring."""
    count = max(3, round(CIRCUMFERENCE/sx))
    x = u/(CIRCUMFERENCE/count)
    y = z/sz
    ix = np.floor(x).astype(np.int32)
    iy = np.floor(y).astype(np.int32)
    fx = x-ix
    fy = y-iy
    fx = fx*fx*(3.-2.*fx)
    fy = fy*fy*(3.-2.*fy)
    return ((hash2(ix % count, iy, seed)*(1-fx)+hash2((ix+1) % count, iy, seed)*fx)*(1-fy)
            +(hash2(ix % count, iy+1, seed)*(1-fx)+hash2((ix+1) % count, iy+1, seed)*fx)*fy)


def fractured_field(u, z):
    """Unequal 3–15m planar stone faces; cell colour is never randomized."""
    u = u+3.*noise(u,z,75.,63.,61)
    z = z+2.*noise(u,z,61.,49.,67)
    count = round(CIRCUMFERENCE/9.5)
    sx,sz = CIRCUMFERENCE/count,7.
    ix = np.floor(u/sx).astype(np.int32)
    iz = np.floor(z/sz).astype(np.int32)
    nearest = np.full(u.shape,np.inf)
    second = nearest.copy()
    height = np.zeros(u.shape)
    for oz in (-1,0,1):
        for ox in (-1,0,1):
            cx,cz = ix+ox,iz+oz
            hx = hash2(cx % count,cz,71)
            hz = hash2(cx % count,cz,73)
            dx = u-(cx+.5+.36*hx)*sx
            dz = z-(cz+.5+.36*hz)*sz
            distance = dx*dx+dz*dz
            choose = distance<nearest
            second = np.where(choose,nearest,np.minimum(second,distance))
            nearest = np.minimum(nearest,distance)
            plane = .35*hash2(cx % count,cz,79)+.34*hash2(cx % count,cz,83)*dx+.32*hash2(cx % count,cz,89)*dz
            height = np.where(choose,plane,height)
    joints = np.exp(-((np.sqrt(second)-np.sqrt(nearest))/.55)**2)
    return height,joints


def tga(path, a):
    if a.ndim == 2:
        a = np.repeat(a[..., None], 3, axis=2)
    h, w = a.shape[:2]
    path.write_bytes(struct.pack('<BBBHHBHHHHBB', 0,0,2,0,0,0,0,0,w,h,24,0)
                     + a[..., ::-1].tobytes())


def quantize(a):
    return np.clip(np.rint(a*255), 0, 255).astype(np.uint8)


def png(path, a):
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    h,w = a.shape[:2]
    raw = b''.join(b'\0'+row.tobytes() for row in a)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))
                     +chunk(b'IDAT',zlib.compress(raw,6))+chunk(b'IEND',b''))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--width', type=int, default=8192)
    p.add_argument('--height', type=int, default=2048)
    p.add_argument('--fractured', action='store_true', help='V2 darker coastal palette and readable meso fracture normals')
    args = p.parse_args()
    if not 512 <= args.width <= 8192 or not 128 <= args.height <= 8192:
        raise ValueError('Expected width512..8192 and height128..8192')
    started = time.time()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    w, h = args.width, args.height
    du, dv = (UV_MAX-UV_MIN)/np.array([w, h])
    u = ((np.arange(w)+.5)*du+UV_MIN[0])*ARC_METRES
    zs = ((np.arange(h)+.5)*dv+UV_MIN[1])*280.-200.
    color = np.empty((h,w,3), np.uint8)
    rough = np.empty((h,w), np.uint8)
    height = np.empty((h,w), np.float32)
    luminance_min, luminance_max = 1., 0.
    for row in range(0, h, 128):
        end = min(row+128, h)
        uu, zz = np.meshgrid(u, zs[row:end])
        broad = noise(uu,zz,380.,180.,3)
        fold = 12.*noise(uu,zz,710.,310.,11)
        # No linear U tilt: it would create a seam at the ring closure.
        formation = zz+fold
        bed = .65*noise(uu,formation,1200.,3.3,19)+.35*noise(uu,formation,460.,7.1,29)
        meso = noise(uu,zz,47.,29.,37)
        mineral = np.maximum(noise(uu,zz,160.,71.,43)-.3,0.)/.7
        facet,joints = fractured_field(uu,zz) if args.fractured else (None,0.)
        base = (.170 if args.fractured else .222)+.022*broad+.011*bed+.006*meso
        base -= .009*joints
        linear = base[...,None]*np.array([.981,1.008,1.027])
        linear += mineral[...,None]*np.array([.008,.003,-.004])
        linear = np.clip(linear,.10,.32)
        luminance_min = min(luminance_min,float(linear.min()))
        luminance_max = max(luminance_max,float(linear.max()))
        srgb = np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
        color[row:end] = quantize(srgb)
        rough[row:end] = quantize(np.clip(.79+.026*meso+.014*bed-.018*mineral+.015*joints,.70,.88))
        if args.fractured:
            ledges = .28*np.tanh(3.*noise(uu,formation,38.,5.,97))
            height[row:end] = facet+ledges+.04*noise(uu,zz,7.,3.2,53)
        else:
            height[row:end] = .018*bed+.009*noise(uu,zz,7.,3.2,53)
    dz, dx = np.gradient(height, dv*280., du*ARC_METRES)
    # Height gradients supply restrained authored perturbation, never WPO.
    if args.fractured:
        # Bound joint-edge derivatives to35deg; no displacement or cavity AO.
        slope = np.sqrt(dx*dx+dz*dz)
        limit = np.minimum(1.,np.tan(np.deg2rad(35.))/np.maximum(slope,1.e-8))
        dx *= limit
        dz *= limit
    normal = np.stack((-dx,-dz,np.ones_like(dx)),axis=-1)
    normal /= np.linalg.norm(normal,axis=-1)[...,None]
    normal8 = quantize(normal*.5+.5)
    files = {'albedo':'T_NordicFace_BaseColor.tga',
             'roughness':'T_NordicFace_Roughness.tga',
             'normal':'T_NordicFace_NormalGL.tga'}
    for role, data in [('albedo',color),('roughness',rough),('normal',normal8)]:
        tga(out/files[role],data)
    preview_step = max(1, int(np.ceil(w/1536)))
    png(out/'context_preview.png',color[::-1][::preview_step,::preview_step])
    png(out/'context_normal_preview.png',normal8[::-1][::preview_step,::preview_step])
    pilot_x0 = int((0.-UV_MIN[0])/du)
    pilot_x1 = int((1.-UV_MIN[0])/du)
    pilot_y0 = int((0.-UV_MIN[1])/dv)
    pilot_y1 = int((1.-UV_MIN[1])/dv)
    png(out/'context_pilot_crop.png',color[pilot_y0:pilot_y1,pilot_x0:pilot_x1][::-1])
    png(out/'context_pilot_normal_crop.png',normal8[pilot_y0:pilot_y1,pilot_x0:pilot_x1][::-1])
    # Verify exact full-ring seam and show that a 500m shift is not repeated.
    check_u,check_z = np.meshgrid(np.linspace(-3000.,6500.,97),np.linspace(-180.,55.,29))
    check = noise(check_u,check_z,380.,180.,3)
    seam_error = float(np.max(np.abs(check-noise(check_u+CIRCUMFERENCE,check_z,380.,180.,3))))
    shift_difference = float(np.sqrt(np.mean((check-noise(check_u+ARC_METRES,check_z,380.,180.,3))**2)))
    if not seam_error < 1e-10 or not shift_difference > .1:
        raise RuntimeError('Ring continuity or unique500m context audit failed')
    manifest = dict(schema='directorscut.authored_cliff_context.v1',resolution=[w,h],
        context_uv_min=UV_MIN.tolist(),context_uv_max=UV_MAX.tolist(),
        context_sample_expression='(UV0-context_uv_min)/(context_uv_max-context_uv_min); CLAMP',
        physical_extent_m=[float((UV_MAX[0]-UV_MIN[0])*ARC_METRES),float((UV_MAX[1]-UV_MIN[1])*280.)],
        original_uv_metres=[ARC_METRES,280.],z_formula='Z=280*UV0.y-200',
        palette_linear_base=.170 if args.fractured else .222,palette_rgb_multipliers=[.981,1.008,1.027],
        material_pass='fractured_v2' if args.fractured else 'subtle_v1',
        palette_linear_min_max=[luminance_min,luminance_max],roughness_min_max=[float(rough.min()/255),float(rough.max()/255)],
        normal_format='OpenGL tangent-space +V up; import green flip once',
        max_normal_angle_degrees=float(np.degrees(np.arccos(np.clip(normal[:,:,2].min(),-1,1)))),
        normal_angle_percentiles_degrees=np.percentile(np.degrees(np.arccos(np.clip(normal[:,:,2],-1,1))),[50,90,99]).tolist(),
        source_pixels_used=False,geometry_changed=False,lighting_changed=False,
        authored_not_simulated=True,limitation='Low-contrast authored context strata and mineral masks; no actual surface slope, physical erosion, or imported scan normals. Coarse context map is not close-up rock detail.',
        uniqueness='One full10km circumference only; no500m repeat. Full angular seam is continuous.',
        audit=dict(full_ring_seam_max_error=seam_error,shift497m_noise_rms_difference=shift_difference),
        texture_files=files,files={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in files.values()},
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),seconds=time.time()-started)
    (out/'tile_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(output=str(out),**manifest)),flush=True)


if __name__ == '__main__':
    main()
