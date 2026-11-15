"""Regression checks that the audit catches guard, provenance and mask drift."""
from pathlib import Path
import tempfile
import unittest
import numpy as np
from audit_weathered_cliff_mesh import audit,ray_hit


class AuditChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.folder = Path(self.temp.name)
        h = np.arange(25,dtype=np.float32).reshape(5,5)*.2
        kind = np.zeros((5,5),np.uint8)
        kind[[0,-1],:] = 2
        kind[:,[0,-1]] = 2
        self.source = dict(height_m=h,cell_size_m=np.array(2.),heightfield_valid_mask=np.ones_like(kind),
                           constraint_type=kind,frame_origin_world_m=np.array([-1350.,1200.,0.]),
                           basis_u_world=np.array([1.,0,0]),basis_v_world=np.array([0,1.,0]),
                           flow_accumulation=h+1,deposited_m=h*.1,eroded_m=h*.2)
        p = np.array([[-1350+2*c,1200+2*r,float(h[r,c])] for r in range(5) for c in range(5)])
        t = np.array([(a,a+1,a+6) for a in range(25) if a//5<4 and a%5<4]+
                     [(a,a+6,a+5) for a in range(25) if a//5<4 and a%5<4],np.int64)
        ids,weights = [],[]
        for i in range(len(p)):
            matching = np.flatnonzero(np.any(t==i,axis=1))
            # Prefer an unconstrained source triangle where one exists.
            j = min(matching,key=lambda j: bool(np.any(kind.ravel()[t[j]])))
            ids.append(j)
            weights.append((t[j]==i).astype(float))
        ids,weights = np.array(ids),np.array(weights)
        gkind = np.bitwise_or.reduce(kind.ravel()[t[ids]],axis=1)
        colors = []
        for key,log in (('flow_accumulation',True),('deposited_m',False),('eroded_m',False)):
            f = self.source[key].astype(float)
            f = np.log1p(f) if log else f
            colors.append((f/f.max()).ravel())
        self.mesh = dict(vertices_m=p.copy(),baseline_vertices_m=p,triangles=t,source_grid_triangles=t,
                         source_triangle_index=ids,source_barycentric=weights,
                         guard_kind=gkind,guard_weight=(gkind==0).astype(float),
                         vertex_rgba=np.column_stack((*colors,np.ones(25))))

    def tearDown(self):
        self.temp.cleanup()

    def run_audit(self):
        source,mesh = self.folder/'source.npz',self.folder/'mesh.npz'
        np.savez(source,**self.source)
        np.savez(mesh,**self.mesh)
        return audit(mesh,source)

    def test_valid_original_mesh(self):
        self.assertTrue(self.run_audit()['passed'])

    def test_fixed_guard_motion_detected(self):
        self.mesh['vertices_m'][0,2] += .03
        self.assertIn('fixed guard vertices changed',self.run_audit()['failures'])

    def test_baseline_provenance_drift_detected(self):
        self.mesh['baseline_vertices_m'][12,0] += .1
        self.assertIn('baseline no longer reconstructs original source triangles',self.run_audit()['failures'])

    def test_mask_permutation_detected(self):
        self.mesh['vertex_rgba'] = np.roll(self.mesh['vertex_rgba'],1,axis=0)
        self.assertIn('material masks disagree with original triangle barycentric sampling',self.run_audit()['failures'])

    def test_guard_relaxation_detected(self):
        self.mesh['guard_weight'][0] = .2
        self.assertIn('original constrained triangle may move',self.run_audit()['failures'])

    def test_reversed_face_detected(self):
        self.mesh['triangles'] = self.mesh['triangles'].copy()
        self.mesh['triangles'][4] = self.mesh['triangles'][4,::-1]
        self.assertIn('inconsistent shared-edge winding',self.run_audit()['failures'])

    def test_original_vertical_face_ray(self):
        faces = np.array([[[3.,-2,-2],[3,2,-2],[3,0,2]]])
        hit = ray_hit(faces,(faces.min(axis=1),faces.max(axis=1)),np.array([8.,0,0]),[-2,0,0],6.)
        self.assertIsNotNone(hit)
        self.assertAlmostEqual(hit['distance_m'],5.)
        np.testing.assert_allclose(hit['normal'],[1.,0,0])

    def test_original_stacked_surfaces_nearest_underside(self):
        faces = np.array([[[-2.,-2,1],[2,-2,1],[0,2,1]],[[-2.,-2,4],[2,-2,4],[0,2,4]]])
        hit = ray_hit(faces,(faces.min(axis=1),faces.max(axis=1)),np.array([0.,0,2]),[0,0,1],6.)
        self.assertIsNotNone(hit)
        self.assertAlmostEqual(hit['position'][2],4.)
        np.testing.assert_allclose(hit['normal'],[0.,0,1])


if __name__ == '__main__':
    unittest.main()
