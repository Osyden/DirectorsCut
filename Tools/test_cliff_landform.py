"""Constraint/provenance regression tests for the offline clean landform seed."""
import json
import sys
import unittest

sys.dont_write_bytecode = True

import numpy as np

from generate_cliff_landform import Settings, generate
from generate_eroded_cliff_sector import validate_seed


def fixture():
    rows, cols, cell = 201, 65, 2.0
    yy, xx = np.indices((rows, cols))
    xy = np.stack([(xx-32)*cell, 1470+yy*cell], axis=-1)
    angles = np.linspace(1.49, 1.66, 51)
    v = np.linspace(0, 1, 161)
    radius = np.interp(v, [0, .2, .84, 1], [1500, 1540, 1720, 1910])
    height = np.interp(v, [0, .2, .84, 1], [-105, -90, 20, 23])
    raw = np.stack([np.cos(angles[:, None])*radius,
                    np.sin(angles[:, None])*radius,
                    np.broadcast_to(height, (51, 161))], axis=-1)
    frame = {"source_angle_rad": float(np.pi/2), "arena_radius_m": 1600}
    meta = {"frame": frame, "coordinate_frame": "test XY metres",
            "array_layout": "radial rows, tangential cols",
            "provenance": {"geometry_reference": "synthetic_constraint_fixture"}}
    return {"world_xy_m": xy, "cell_size_m": np.array(cell),
            "raw_profile_xyz_world_m": raw, "raw_profile_angles_rad": angles,
            "raw_profile_v": v, "shore_clearance_m": 1508-np.linalg.norm(xy, axis=-1),
            "frame_origin_world_m": np.array([-64, 1470, 0]),
            "basis_u_world": np.array([1, 0, 0]), "basis_v_world": np.array([0, 1, 0]),
            "basis_up_world": np.array([0, 0, 1]),
            "u_m": np.arange(cols)*cell, "v_m": np.arange(rows)*cell,
            "metadata_json": np.array(json.dumps(meta)),
            # Deliberate nonsense: the old wall heightfield must never be read.
            "height_m": np.full((rows, cols), 99999, np.float32)}


class CleanLandformTests(unittest.TestCase):
    def test_ignores_old_interior_and_heightfield(self):
        data = fixture()
        a = generate(data)
        data["height_m"][:] = -999999
        interior = (data["raw_profile_v"] > .2) & (data["raw_profile_v"] < .84)
        data["raw_profile_xyz_world_m"][:, interior] += 50000
        b = generate(data)
        for key in ("height_m", "protected_mask", "bedrock_m", "outflow_mask"):
            np.testing.assert_array_equal(a[key], b[key])

    def test_monotone_no_upper_overshoot_or_terminal_step(self):
        result = generate(fixture())
        z = result["base_profile_height_m"]
        self.assertTrue(np.all(np.diff(z, axis=1) > 0))
        np.testing.assert_array_equal(z[:, 0], result["constraint_lower_height_m"])
        np.testing.assert_array_equal(z[:, -1], result["constraint_upper_height_m"])
        self.assertTrue(np.all(z <= result["constraint_upper_height_m"][:, None]))
        self.assertLess(float(np.max(np.diff(z, axis=1)[:, -1])), .05)

    def test_valid_outflow_and_kernel_schema(self):
        result = generate(fixture())
        validate_seed(result)
        out = result["outflow_mask"] != 0
        fixed = result["protected_mask"] == 1
        valid = result["heightfield_valid_mask"] != 0
        self.assertGreater(int(out.sum()), 0)
        self.assertTrue(np.all(fixed[out] & valid[out]))
        neighbours_invalid = np.zeros(valid.shape, bool)
        neighbours_invalid[1:] |= ~valid[:-1]
        neighbours_invalid[:-1] |= ~valid[1:]
        neighbours_invalid[:, 1:] |= ~valid[:, :-1]
        neighbours_invalid[:, :-1] |= ~valid[:, 1:]
        self.assertTrue(np.all(neighbours_invalid[out]))
        self.assertTrue(np.all(result["erodibility"][fixed] == 0))
        self.assertTrue(np.all(result["loose_m"][fixed] == 0))

    def test_seed_changes_interior_but_not_fixed_constraint_samples(self):
        data = fixture()
        a, b = generate(data), generate(data, Settings(seed=512))
        radius = np.linalg.norm(data["world_xy_m"], axis=-1)
        # Exclude lateral fixed tile seams: they freeze the new proposal, not
        # unrelated old terrain. Shore and upper plateau are actual constraints.
        boundary = (radius <= np.min(a["constraint_lower_radius_m"])) | (radius >= np.max(a["constraint_upper_radius_m"]))
        np.testing.assert_array_equal(a["height_m"][boundary], b["height_m"][boundary])
        self.assertGreater(float(np.max(np.abs(a["height_m"]-b["height_m"]))), 1)
        c = generate(data)
        np.testing.assert_array_equal(a["height_m"], c["height_m"])

    def test_does_not_export_rejected_wall_as_raw_control(self):
        result = generate(fixture())
        self.assertFalse(any(k.startswith("raw_profile") for k in result))
        self.assertNotIn("discarded_vertical_span_m", result)
        self.assertEqual(json.loads(str(result["metadata_json"]))["source_kind"], "clean_macro_landform")

    def test_preview_catchments_connect_and_preserve_shore(self):
        data = fixture()
        base = generate(data)
        result = generate(data, Settings(preview_catchments=True))
        radius = np.linalg.norm(data["world_xy_m"], axis=-1)
        shore = radius <= float(np.min(result["constraint_lower_radius_m"]))
        np.testing.assert_array_equal(base["height_m"][shore], result["height_m"][shore])
        self.assertGreater(float(np.max(result["drainage_cut_m"])), 1)
        self.assertFalse(json.loads(str(result["metadata_json"]))["metrics"]["temple_footing_compatibility_verified"])
        paths = json.loads(str(result["drainage_paths_json"]))
        for network in paths:
            trunk, first, second = network["paths_arc_rimdistance_halfwidth_depth_m"]
            self.assertEqual(first[0], trunk[3])
            self.assertEqual(second[0], trunk[4])
            self.assertGreater(trunk[-1][1], 20)
            self.assertLessEqual(trunk[-1][1], 40)
            self.assertEqual(trunk[0][3], 0)
        validate_seed(result)


if __name__ == "__main__":
    unittest.main()
