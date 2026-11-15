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

    def test_empty_outflow_mask_preserves_closed_default(self):
        seed = demo_seed(24)
        cfg = Settings(steps=32)
        closed, metrics = erode(seed, cfg)
        seed["outflow_mask"] = np.zeros(seed["height_m"].shape, np.uint8)
        explicit, _ = erode(seed, cfg)
        for name in ("height_m", "water_m", "suspended_m", "eroded_m", "deposited_m", "flow_accumulation"):
            np.testing.assert_array_equal(closed[name], explicit[name], err_msg=name)
        self.assertEqual(metrics["boundary"], "closed D8")
        self.assertEqual(metrics["outflow_water_m3"], 0)
        self.assertEqual(metrics["outflow_sediment_m3"], 0)

    def test_open_outflow_conserves_exported_water_and_sediment(self):
        seed = demo_seed(40)
        seed["outflow_mask"] = np.zeros(seed["height_m"].shape, np.uint8)
        seed["outflow_mask"][0, :] = 1
        initial = {key: value.copy() for key, value in seed.items()}
        output, metrics = erode(seed, Settings(steps=128, rainfall_jitter=0, thermal_rate=0))
        outlets = seed["outflow_mask"] == 1
        fixed = seed["protected_mask"] == 1
        self.assertGreater(metrics["outflow_water_m3"], 1.)
        self.assertGreater(metrics["outflow_sediment_m3"], .01)
        self.assertLess(abs(metrics["solver_solid_mass_error_m3"]), 1.e-6)
        self.assertLess(abs(metrics["water_budget_error_m3"]), 1.e-6)
        self.assertEqual(float(output["water_m"][outlets].sum()), 0)
        self.assertEqual(float(output["suspended_m"][outlets].sum()), 0)
        self.assertGreater(float(output["suspended_m"][~outlets].sum()), 0)  # no terminal dump
        self.assertEqual(float(output["outflow_water_m3"][~outlets].sum()), 0)
        self.assertEqual(float(output["outflow_sediment_m3"][~outlets].sum()), 0)
        self.assertAlmostEqual(float(output["outflow_water_m3"].sum()), metrics["outflow_water_m3"], places=8)
        self.assertAlmostEqual(float(output["outflow_sediment_m3"].sum()), metrics["outflow_sediment_m3"], places=8)
        np.testing.assert_array_equal(output["height_m"][fixed].view(np.uint32), seed["height_m"][fixed].view(np.uint32))
        area = float(seed["cell_size_m"])**2
        solid_change = float(np.sum(output["height_m"].astype(np.float64)-seed["height_m"])) * area
        remaining = float(np.sum(output["suspended_m"], dtype=np.float64))*area
        error = solid_change+remaining+metrics["outflow_sediment_m3"]
        self.assertLessEqual(abs(error), metrics["float32_quantization_bound_m3"]+1.e-6)
        for name, value in initial.items():
            np.testing.assert_array_equal(seed[name], value, err_msg=name)

    def test_outflow_at_invalid_padding_boundary_is_not_padding_flux(self):
        seed = demo_seed(28)
        seed["heightfield_valid_mask"] = np.ones(seed["height_m"].shape, np.uint8)
        seed["heightfield_valid_mask"][:, :5] = 0
        seed["protected_mask"][:, :6] = 1
        seed["outflow_mask"] = np.zeros(seed["height_m"].shape, np.uint8)
        seed["outflow_mask"][:, 5] = 1
        cfg = Settings(steps=40, rainfall_jitter=0)
        a, metrics = erode(seed, cfg)
        seed["height_m"][:, :5] += 1000
        seed["bedrock_m"][:, :5] += 1000
        b, _ = erode(seed, cfg)
        valid = seed["heightfield_valid_mask"] == 1
        for name in ("height_m", "water_m", "suspended_m", "outflow_water_m3", "outflow_sediment_m3"):
            np.testing.assert_array_equal(a[name][valid], b[name][valid], err_msg=name)
        for name in ("water_m", "suspended_m", "outflow_water_m3", "outflow_sediment_m3"):
            self.assertEqual(float(a[name][~valid].sum()), 0, name)
        self.assertGreater(metrics["outflow_water_m3"], 0)

    def test_outflow_only_removes_available_boundary_reservoirs(self):
        seed = demo_seed(16)
        seed["outflow_mask"] = np.zeros(seed["height_m"].shape, np.uint8)
        seed["outflow_mask"][0, :] = 1
        cfg = Settings(steps=20, rainfall_jitter=0, flow_rate=0, thermal_rate=0, evaporation_rate=0)
        output, metrics = erode(seed, cfg)
        expected = 16*cfg.steps*cfg.rainfall_m*float(seed["cell_size_m"])**2
        self.assertAlmostEqual(metrics["outflow_water_m3"], expected, places=9)
        self.assertEqual(metrics["outflow_sediment_m3"], 0)
        np.testing.assert_array_equal(output["height_m"], seed["height_m"])
        dry, metrics = erode(seed, replace(cfg, rainfall_m=0))
        self.assertEqual(metrics["outflow_water_m3"], 0)
        self.assertEqual(float(dry["water_m"].sum()), 0)

    def test_outflow_validation_rejects_interior_and_invalid_sinks(self):
        for corruption in ("interior", "movable", "padding", "nonbinary", "shape"):
            seed = demo_seed(16)
            mask = np.zeros(seed["height_m"].shape, np.float32)
            mask[0, :] = 1
            if corruption == "interior":
                mask[8, 8] = 1
                seed["protected_mask"][8, 8] = 1
            if corruption == "movable": seed["protected_mask"][0, 8] = 0
            if corruption == "padding":
                seed["heightfield_valid_mask"] = np.ones(mask.shape, np.uint8)
                seed["heightfield_valid_mask"][0, 8] = 0
            if corruption == "nonbinary": mask[0, 8] = .5
            if corruption == "shape": mask = mask[:-1]
            seed["outflow_mask"] = mask
            with self.assertRaises(ValueError, msg=corruption):
                erode(seed, Settings(steps=1))


if __name__ == "__main__":
    unittest.main()
