import sys

import numpy as np

from triqs.gf import Gf, MeshImFreq, Idx

from triqs_tprf.tight_binding import TBLattice
from triqs_tprf.lattice import lattice_dyson_g0_wk

norb, nk, nw = [int(ele) for ele in sys.argv[1:]]

t = - 2.0 * np.eye(norb)
t_r = TBLattice(
    units = [(1, 0, 0)],
    hopping = {
        (+1,): t,
        (-1,): t,
        },
    orbital_positions = [(0,0,0)]*norb,
    orbital_names = [str(ele) for ele in range(norb)],
    )

kmesh = t_r.get_kmesh((nk, 1, 1))
e_k = t_r.fourier(kmesh)
wmesh = MeshImFreq(beta=1.0, S='Fermion', n_max=nw)

g0_wk = lattice_dyson_g0_wk(mu=0.0, e_k=e_k, mesh=wmesh)

print(g0_wk.data.shape)
