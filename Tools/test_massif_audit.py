"""Focused geometric tests for the independent massif-footprint audit."""
import sys
import unittest

import numpy as np

sys.dont_write_bytecode = True
from audit_massif_cliff_seed import footprint_report, convex_ccw, audit


def fixture():
    angle = -.71
    u = np.array([np.cos(angle), np.sin(angle), 0.])
    v = np.array([-u[1], u[0], 0.])
    return {"height_m": np.full((5, 5), 10., np.float32),
            "heightfield_valid_mask": np.ones((5, 5), np.uint8),
            "cell_size_m": np.array(2.), "frame_origin_world_m": np.array([-1200., 950., 0.]),
            "basis_u_world": u, "basis_v_world": v}


def anchor(data, points):
    p = np.asarray(points)
    world = data["frame_origin_world_m"][:2]+p[:, 0, None]*data["basis_u_world"][:2]+p[:, 1, None]*data["basis_v_world"][:2]
    return {"id": "fixture", "footprint_xy_m": world.tolist(), "support_height_m": 10.}


def constrained_fixture():
    data = fixture()
    shape = (9, 9)
    row, col = np.indices(shape)
    xy = data["frame_origin_world_m"][:2]+col[..., None]*2*data["basis_u_world"][:2]+row[..., None]*2*data["basis_v_world"][:2]
    kinds = np.zeros(shape, np.uint8)
    kinds[row <= 3] |= 4
    kinds[(row == 0) | (col == 0) | (row == 8) | (col == 8)] |= 2
    fixed = kinds != 0
    data.update(height_m=np.full(shape, 10., np.float32), heightfield_valid_mask=np.ones(shape, np.uint8),
                world_xy_m=xy, outflow_mask=(row == 0).astype(np.uint8),
                protected_mask=fixed.astype(np.float32), constraint_type=kinds,
                constraint_target_height_m=np.full(shape, 10., np.float32),
                coast_distance_m=(row*2).astype(np.float32),
                bedrock_m=np.where(fixed, 10., 8.).astype(np.float32),
                erodibility=np.where(fixed, 0., .5).astype(np.float32),
                loose_m=np.where(fixed, 0., .1).astype(np.float32))
    return data


class FootprintAuditTests(unittest.TestCase):
    def test_rotated_negative_frame_and_exact_area(self):
        data = fixture()
        result = footprint_report(data, anchor(data, [[1, 1], [7, 1], [7, 7], [1, 7]]))
        self.assertEqual(result["status"], "covered_at_target")
        self.assertAlmostEqual(result["covered_area_m2"], 36., places=8)
        self.assertLess(abs(result["area_accounting_error_m2"]), 1.e-8)

    def test_interior_hole_cannot_hide_behind_valid_corners(self):
        data = fixture()
        data["heightfield_valid_mask"][2, 2] = 0
        result = footprint_report(data, anchor(data, [[1, 1], [7, 1], [7, 7], [1, 7]]))
        self.assertEqual(result["status"], "hole_in_footprint")
        self.assertAlmostEqual(result["missing_triangle_area_m2"], 12., places=8)
        self.assertAlmostEqual(result["covered_area_m2"], 24., places=8)

    def test_partial_coverage_is_not_full_support(self):
        data = fixture()
        result = footprint_report(data, anchor(data, [[-2, 2], [2, 2], [2, 6], [-2, 6]]))
        self.assertEqual(result["status"], "partial_tile_coverage_requires_other_provider")
        self.assertAlmostEqual(result["outside_tile_area_m2"], 8., places=8)
        self.assertAlmostEqual(result["covered_area_m2"], 8., places=8)

    def test_uses_true_triangle_extrema_not_only_footprint_corners(self):
        data = fixture()
        data["height_m"][2, 2] = 14.
        result = footprint_report(data, anchor(data, [[1, 1], [7, 1], [7, 7], [1, 7]]))
        self.assertEqual(result["status"], "support_height_mismatch")
        self.assertAlmostEqual(result["height_min_m"], 10.)
        self.assertAlmostEqual(result["height_max_m"], 14.)

    def test_reverse_winding_accepted_but_concavity_rejected(self):
        p = np.array([[0, 0], [2, 0], [2, 2], [0, 2]])
        np.testing.assert_array_equal(convex_ccw(p[::-1]), p)
        with self.assertRaises(ValueError):
            convex_ccw([[0, 0], [2, 0], [1, .5], [2, 2], [0, 2]])

    def test_sub_five_cm_temple_override_cannot_move_immutable_seam(self):
        source = constrained_fixture()
        candidate = {k: v.copy() for k, v in source.items()}
        self.assertTrue(audit(source, candidate)["passed"])
        candidate["height_m"][6, 0] += .02
        candidate["constraint_target_height_m"][6, 0] = candidate["height_m"][6, 0]
        candidate["constraint_type"][6, 0] |= 8
        result = audit(source, candidate)
        self.assertIn("seam source heights moved", result["failures"])

    def test_exact_six_metre_coast_boundary_must_remain_fixed(self):
        source = constrained_fixture()
        candidate = {k: v.copy() for k, v in source.items()}
        candidate["constraint_type"][3, 4] = 0
        candidate["protected_mask"][3, 4] = 0
        result = audit(source, candidate)
        self.assertIn("a physical sample within6m of receiving outlets is not fixed", result["failures"])


if __name__ == "__main__":
    unittest.main()
