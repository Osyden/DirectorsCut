"""CPU conservation, immutability and transport regressions for erosion."""
from dataclasses import replace
import sys
import unittest

sys.dont_write_bytecode = True
import numpy as np
from generate_eroded_cliff_sector import Settings, demo_seed, erode


class ErosionTests(unittest.TestCase):
    def test_128_transport_mass_and_protected_cells(self):
        seed = demo_seed(128)
        original = {key: value.copy() for key, value in seed.items()}
        seed["frame_origin_world_m"] = np.array([17., -9., 0.])
        output, metrics = erode(seed, Settings(steps=128))
        self.assertGreater(metrics["gross_eroded_m3"], 1.)
        self.assertGreater(metrics["gross_deposited_m3"], 1.)
        self.assertLess(abs(metrics["solver_solid_mass_error_m3"]), 1.e-6)
        self.assertLess(abs(metrics["water_budget_error_m3"]), 1.e-6)
        self.assertLessEqual(abs(metrics["serialized_solid_mass_error_m3"]), metrics["float32_quantization_bound_m3"]+1.e-6)
        fixed = seed["protected_mask"] == 1
        np.testing.assert_array_equal(output["height_m"][fixed].view(np.uint32), seed["height_m"][fixed].view(np.uint32))
        np.testing.assert_array_equal(output["bedrock_m"], seed["bedrock_m"])
        np.testing.assert_array_equal(output["frame_origin_world_m"], seed["frame_origin_world_m"])
        self.assertTrue(np.all(output["height_m"] >= output["bedrock_m"]))
        for name in ("water_m", "suspended_m", "loose_m", "eroded_m", "deposited_m", "flow_accumulation"):
            self.assertTrue(np.isfinite(output[name]).all(), name)
            self.assertGreaterEqual(float(output[name].min()), -1.e-8, name)
        for name in ("slope", "flow_mask"):
            self.assertTrue(np.all((output[name] >= 0) & (output[name] <= 1)), name)
        self.assertTrue(np.all(np.abs(output["curvature"]) <= 1))
        for name, value in original.items():
            np.testing.assert_array_equal(seed[name], value, err_msg=f"Mutated seed {name}")

    def test_seed_determinism(self):
        seed = demo_seed(48)
        cfg = Settings(steps=48)
        a, _ = erode(seed, cfg)
        b, _ = erode(seed, cfg)
        c, _ = erode(seed, replace(cfg, seed=cfg.seed+1))
        for name in ("height_m", "eroded_m", "deposited_m", "flow_accumulation", "water_m", "suspended_m"):
            np.testing.assert_array_equal(a[name], b[name], err_msg=name)
        self.assertFalse(np.array_equal(a["height_m"], c["height_m"]))

    def test_fixed_pads_include_signed_zero(self):
        seed = demo_seed(32)
        seed["height_m"][0, 0] = np.float32(-0.)
        seed["bedrock_m"][0, 0] = -10.
        seed["protected_mask"][:] = 1.
        output, _ = erode(seed, Settings(steps=32))
        np.testing.assert_array_equal(output["height_m"].view(np.uint32), seed["height_m"].view(np.uint32))
        self.assertEqual(np.count_nonzero(output["eroded_m"]), 0)
        self.assertEqual(np.count_nonzero(output["deposited_m"]), 0)

    def test_hard_rock_without_loose_cover_does_not_erode(self):
        seed = demo_seed(32)
        seed["erodibility"][:] = 0
        output, _ = erode(seed, Settings(steps=32))
        np.testing.assert_array_equal(output["height_m"], seed["height_m"])

    def test_thermal_moves_only_loose_material_and_conserves_mass(self):
        seed = demo_seed(17)
        seed["height_m"][:] = 0
        seed["height_m"][8, 8] = 6
        seed["bedrock_m"] = seed["height_m"]-2
        seed["protected_mask"][:] = 0
        seed["loose_m"] = np.zeros_like(seed["height_m"])
        seed["loose_m"][8, 8] = .4
        cfg = Settings(steps=20, rainfall_m=0, thermal_rate=.4)
        output, metrics = erode(seed, cfg)
        self.assertGreater(float(output["deposited_m"].sum()), 0)
        self.assertGreaterEqual(float(output["height_m"][8, 8]), 5.6-1.e-6)
        self.assertLess(float(output["height_m"][8, 8]), 6)
        self.assertLess(abs(metrics["gross_eroded_m3"]-metrics["gross_deposited_m3"]), 1.e-9)
        self.assertEqual(float(output["suspended_m"].sum()), 0)
        seed["loose_m"][:] = 0
        still, _ = erode(seed, cfg)
        np.testing.assert_array_equal(still["height_m"], seed["height_m"])

    def test_bedrock_limits_accelerated_erosion(self):
        seed = demo_seed(48)
        seed["bedrock_m"] = seed["height_m"]-.01
        output, _ = erode(seed, Settings(steps=96, rainfall_m=.05, initial_loose_m=0))
        self.assertTrue(np.all(output["height_m"] >= seed["bedrock_m"]))

    def test_invalid_seed_rejected(self):
        for corruption in ("dtype", "bedrock", "mask", "nan", "shape", "cell"):
            seed = demo_seed(8)
            if corruption == "dtype": seed["height_m"] = seed["height_m"].astype(np.float64)
            if corruption == "bedrock": seed["bedrock_m"][3, 3] = seed["height_m"][3, 3]+1
            if corruption == "mask": seed["protected_mask"][3, 3] = 1.1
            if corruption == "nan": seed["height_m"][3, 3] = np.nan
            if corruption == "shape": seed["erodibility"] = seed["erodibility"][:-1]
            if corruption == "cell": seed["cell_size_m"] = np.array(0.)
            with self.assertRaises(ValueError, msg=corruption):
                erode(seed, Settings(steps=1))

    def test_nonphysical_padding_cannot_feed_erosion(self):
        seed = demo_seed(24)
        domain = np.ones(seed["height_m"].shape, np.uint8)
        domain[:, :5] = 0
        seed["heightfield_valid_mask"] = domain
        seed["protected_mask"][:, :5] = 1
        cfg = Settings(steps=40, rainfall_jitter=0)
        a, metrics = erode(seed, cfg)
        seed["height_m"][:, :5] += 1000
        seed["bedrock_m"][:, :5] += 1000
        b, _ = erode(seed, cfg)
        valid = domain == 1
        for name in ("height_m", "eroded_m", "deposited_m", "water_m", "flow_accumulation"):
            np.testing.assert_array_equal(a[name][valid], b[name][valid], err_msg=name)
        self.assertEqual(float(a["water_m"][~valid].sum()), 0)
        expected_rain = cfg.steps*cfg.rainfall_m*float(seed["cell_size_m"])**2*np.count_nonzero(valid)
        self.assertAlmostEqual(metrics["rainfall_m3"], expected_rain, places=8)


if __name__ == "__main__":
    unittest.main()
