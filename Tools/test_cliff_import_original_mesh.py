"""Regression: original-3D import references preserve stacked surfaces and masks."""
from pathlib import Path
import tempfile
import unittest

import numpy as np

from audit_cliff_preview_import import original_mesh_reference, audit_geometry


class OriginalMeshImportTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.path = Path(self.directory.name) / "original.npz"
        # Two vertices share XY but occupy different strata. An XY-based
        # reference or identity lookup would collapse this valid tetrahedron.
        self.local = np.array([[0., 0., 0.], [0., 0., 13.], [7., 0., 1.], [1., 9., 2.]])
        self.faces = np.array([[0, 2, 1], [0, 1, 3], [0, 3, 2], [1, 2, 3]])
        self.colors = np.array([[.1, .2, .3, 1], [.9, .8, .7, 1], [.4, .6, .2, 1], [.3, .7, .8, 1]], dtype=np.float32)
        angle = .71
        self.basis = np.array([[np.cos(angle), -np.sin(angle), 0], [np.sin(angle), np.cos(angle), 0], [0, 0, 1]])
        self.origin = np.array([-1219., 879., -72.])
        self.meta = dict(mesh_path=str(self.path), local_origin_arena_m=self.origin.tolist(),
                         basis_u_arena=self.basis[:, 0].tolist(), basis_v_arena=self.basis[:, 1].tolist(),
                         basis_z_arena=self.basis[:, 2].tolist())
        self.write(self.colors)

    def tearDown(self):
        self.directory.cleanup()

    def write(self, colors):
        np.savez(self.path, vertices_m=self.origin + self.local @ self.basis.T,
                 triangles=self.faces, vertex_rgba=colors)

    def test_stacked_vertices_survive_frame_export_and_reordered_import(self):
        vertices, faces, rgba = original_mesh_reference(self.meta)
        np.testing.assert_allclose(vertices, self.local, atol=1.e-10)
        np.testing.assert_array_equal(faces, self.faces)
        np.testing.assert_array_equal(rgba, self.colors)
        order = np.array([3, 1, 0, 2])
        inverse = np.argsort(order)
        imported = (vertices * [100, -100, 100])[order]
        source = dict(positions_m=vertices.tolist(), triangles=faces.tolist(), meta=self.meta)
        mapping, report = audit_geometry(source, imported.tolist(), inverse[faces[::-1]].tolist())
        self.assertEqual(mapping, order.tolist())
        self.assertEqual(report["source_vertices"], 4)
        self.assertLess(report["position_max_error_cm"], 1.e-8)

    def test_rejects_nonrigid_export_frame(self):
        self.meta["basis_z_arena"] = [0, 0, 2]
        with self.assertRaisesRegex(RuntimeError, "orthonormal"):
            original_mesh_reference(self.meta)

    def test_submillimetre_edges_keep_distinct_identity_in_same_search_bucket(self):
        self.local[1] = [.00004, .00003, .00002]
        self.write(self.colors)
        vertices, faces, _ = original_mesh_reference(self.meta)
        order = np.array([2, 0, 3, 1])
        inverse = np.argsort(order)
        imported = (vertices * [100, -100, 100])[order].astype(np.float32)
        source = dict(positions_m=vertices.tolist(), triangles=faces.tolist(), meta=self.meta)
        mapping, report = audit_geometry(source, imported.tolist(), inverse[faces].tolist())
        self.assertEqual(mapping, order.tolist())
        self.assertEqual(report['source_vertices'], 4)

    def test_duplicate_original_xyz_still_rejects_unprovable_identity(self):
        self.local[1] = self.local[0]
        self.write(self.colors)
        vertices, faces, _ = original_mesh_reference(self.meta)
        source = dict(positions_m=vertices.tolist(), triangles=faces.tolist(), meta=self.meta)
        with self.assertRaisesRegex(RuntimeError, 'Ambiguous nearest'):
            audit_geometry(source, (vertices*[100, -100, 100]).tolist(), faces.tolist())

    def test_rejects_missing_or_nonfinite_numeric_masks(self):
        self.write(self.colors[:3])
        with self.assertRaisesRegex(RuntimeError, "vertex masks"):
            original_mesh_reference(self.meta)
        changed = self.colors.copy()
        changed[1, 0] = np.nan
        self.write(changed)
        with self.assertRaisesRegex(RuntimeError, "vertex masks"):
            original_mesh_reference(self.meta)


if __name__ == "__main__":
    unittest.main()
