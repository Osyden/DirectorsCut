"""CPU-only semantic constraint/solver regressions; no Blender or UE required."""
import unittest
import numpy as np

from generate_massif_cliff_seed import constraints, polygon_distance, screened_fit, validate_boundary_source


def fixture():
    y,x = np.indices((41,41))
    height = (x*.01+y*.02).astype(np.float32)
    valid = np.ones_like(height,np.uint8)
    valid[:2] = 0
    outflow = np.zeros_like(valid)
    outflow[2] = 1
    return dict(height_m=height,cell_size_m=np.asarray(2.),world_xy_m=np.stack((2*x,2*y),axis=-1).astype(float),
                frame_origin_world_m=np.asarray([0.,0.,0.]),basis_u_world=np.asarray([1.,0.,0.]),
                basis_v_world=np.asarray([0.,1.,0.]),basis_up_world=np.asarray([0.,0.,1.]),
                protected_mask=np.ones_like(height),heightfield_valid_mask=valid,outflow_mask=outflow)


def anchors(height=4.,polygon=None):
    return dict(schema="directorscut.cliff_support_anchors.v1",coordinate_frame="arena_local_metres",
                anchors=[dict(id="test",support_height_m=height,margin_m=0.,blend_m=30.,
                              footprint_xy_m=polygon or [[36,36],[44,36],[44,44],[36,44]])])


class Constraints(unittest.TestCase):
    def test_old_blanket_lock_is_released(self):
        d=fixture(); kinds,target,distance,reports,_=constraints(d)
        self.assertEqual(kinds[20,20],0)
        self.assertEqual(int(kinds[5,20]&4),4) # Exactly six metres from outlet.
        self.assertEqual(kinds[6,20]&4,0)
        self.assertEqual(float(distance[5,20]),6.)
        np.testing.assert_array_equal(target,d['height_m'])

    def test_semantic_pad_and_triangle_margin(self):
        d=fixture(); kinds,target,_,reports,_=constraints(d,anchors())
        self.assertEqual(reports[0]['coverage'],'full')
        self.assertFalse(reports[0]['full_footing_support_verified'])
        self.assertTrue(np.all(target[(kinds&8)>0]==4.))
        self.assertEqual(int(kinds[17,18]&8),8) # 2m outside exact polygon.

    def test_seam_never_moves_even_for_tiny_difference(self):
        d=fixture(); d['height_m'][:]=1.
        kinds,target,_,reports,_=constraints(d,anchors(1.01,[[0,36],[4,36],[4,44],[0,44]]))
        np.testing.assert_array_equal(target[(kinds&2)>0],np.ones(np.count_nonzero((kinds&2)>0)))
        self.assertFalse(np.any(((kinds&2)>0)&((kinds&8)>0)))
        self.assertEqual(reports[0]['coverage'],'partial')

    def test_measured_source_is_exact_and_frame_mismatch_rejected(self):
        d=fixture(); b={k:v.copy() for k,v in d.items()}; b['height_m']+=17
        bh=validate_boundary_source(d,b)
        kinds,target,*_=constraints(d,boundary_height=bh)
        np.testing.assert_array_equal(target[kinds>0],b['height_m'][kinds>0])
        b['world_xy_m'][10,10,0]+=.1
        with self.assertRaises(ValueError):validate_boundary_source(d,b)

    def test_bad_polygon_and_anchor_conflict_rejected(self):
        d=fixture()
        with self.assertRaises(ValueError):polygon_distance(d['world_xy_m'],[[0,0],[2,0],[1,.1],[0,2]])
        a=anchors(); a['anchors']+=anchors(6.)['anchors']
        with self.assertRaises(ValueError):constraints(d,a)

    def test_screened_fit_fixed_samples_and_discrete_equation(self):
        valid=np.ones((31,31),bool); fixed=np.zeros_like(valid); fixed[:,0]=True
        raw=np.zeros(valid.shape); target=np.zeros_like(raw); target[:,0]=10
        result,correction,metrics=screened_fit(raw,target,valid,fixed,2.,fit_length_m=20.)
        np.testing.assert_array_equal(result[:,0],np.full(31,10.))
        self.assertTrue(np.all(result>=0)); self.assertTrue(np.all(result<=10.))
        self.assertGreater(float(result[15,5]),float(result[15,25]))
        residual=(4+.01)*correction[1:-1,1:-1]-correction[:-2,1:-1]-correction[2:,1:-1]-correction[1:-1,:-2]-correction[1:-1,2:]
        self.assertLess(float(np.max(np.abs(residual))),.001)

    def test_nonphysical_padding_does_not_pull_valid_height(self):
        valid=np.ones((19,19),bool); valid[:3]=False; fixed=~valid
        raw=np.full(valid.shape,5.); target=np.full(valid.shape,5.); target[~valid]=-1.e6
        result,_,_=screened_fit(raw,target,valid,fixed,2.)
        np.testing.assert_array_equal(result[valid],np.full(valid.sum(),5.))
        np.testing.assert_array_equal(result[fixed],target[fixed].astype(np.float32))


if __name__ == '__main__':
    unittest.main()
