
import itertools
import numpy as np

# ----------------------------------------------------------------------

from pytriqs.operators import n, c, c_dag, Operator, dagger

# ----------------------------------------------------------------------

from pyed.OperatorUtils import quartic_tensor_from_operator
from pyed.OperatorUtils import operator_from_quartic_tensor

from pyed.OperatorUtils import symmetrize_quartic_tensor

from pyed.OperatorUtils import quartic_permutation_symmetrize
from pyed.OperatorUtils import quartic_conjugation_symmetrize
from pyed.OperatorUtils import quartic_pauli_symmetrize

# ----------------------------------------------------------------------
def fundamental_operators_from_gf_struct(gf_struct):

    fundamental_operators = []
    for block, idxs in gf_struct:
        for idx in idxs:
            fundamental_operators.append( c(block, idx) )

    return fundamental_operators

# ----------------------------------------------------------------------
def get_rpa_tensor(H_int, fundamental_operators):

    """ Takes a TRIQS operator object and extracts the quartic terms
    and returns the corresponding antisymmetrized quartic tensor in
    vertex index order, i.e., cc+cc+. """
    
    U_abcd = quartic_tensor_from_operator(H_int, fundamental_operators)
    U_abcd = 4 * quartic_permutation_symmetrize(U_abcd)

    # -- Group in Gamma order cc^+cc^+ ( from c^+c^+cc )
    Gamma_RPA_abcd = np.ascontiguousarray(np.transpose(U_abcd, (2, 0, 3, 1)))
        
    return Gamma_RPA_abcd

# ----------------------------------------------------------------------
def get_gamma_rpa(chi0_wnn, U_abcd):

    # -- Build constant gamma

    gamma_wnn = chi0_wnn.copy()
    
    # Nb! In the three frequency form $\Gamma \propto U/\beta^2$

    beta = chi0_wnn.mesh.components[0].beta
    gamma_wnn.data[:] = U_abcd[None, None, None, ...] / beta**2
    
    return gamma_wnn

# ----------------------------------------------------------------------    
def kanamori_charge_and_spin_quartic_interaction_tensors(norb, U, Up, J, Jp):

    """ Following Eliashberg notes. """

    shape = [norb]*4
    U_c, U_s = np.zeros(shape, dtype=np.complex), np.zeros(shape, dtype=np.complex)
    
    for a, abar, b, bbar in itertools.product(range(norb), repeat=4):

        scalar_c, scalar_s = 0, 0

        if (a == abar) and (a == b) and (b == bbar):
            scalar_c = U
            scalar_s = U
            
        if (a == bbar) and  (a != b) and (abar == b):
            scalar_c = -Up + 2*J
            scalar_s = Up

        if (a == abar) and  (a != b) and (b == bbar):
            scalar_c = 2*Up - J
            scalar_s = J

        if (a == b) and  (a != abar) and (abar == bbar):
            scalar_c = Jp
            scalar_s = Jp
            
        #U_c[a, b, bbar, abar] = scalar_c
        #U_s[a, b, bbar, abar] = scalar_s
        
        U_c[a, abar, b, bbar] = scalar_c
        U_s[a, abar, b, bbar] = scalar_s

    return U_c, U_s

# ----------------------------------------------------------------------    
def split_quartic_tensor_in_charge_and_spin(U_4):

    """Assuming spin is the slow index and orbital is the fast index 

    Using a rank 4 U_abcd tensor with composite (spin, orbital) indices
    as input, assuming c^+c^+ c c structure of the tensor

    Returns:

    U_c : Charge channel rank 4 interaction tensor
    U_s : Spin channel rank 4 interaction tensor """

    shape_4 = np.array(U_4.shape)
    shape_8 = np.vstack(([2]*4, shape_4 / 2)).T.flatten()

    U_8 = U_4.reshape(shape_8)

    U_8 = np.transpose(U_8, (0, 2, 4, 6, 1, 3, 5, 7)) # spin first

    # -- Check spin-conservation

    zeros = np.zeros_like(U_8[0, 0, 0, 0])
    
    np.testing.assert_array_almost_equal(U_8[0, 0, 0, 1], zeros)
    np.testing.assert_array_almost_equal(U_8[0, 0, 1, 0], zeros)
    np.testing.assert_array_almost_equal(U_8[0, 1, 0, 0], zeros)
    np.testing.assert_array_almost_equal(U_8[1, 0, 0, 0], zeros)

    np.testing.assert_array_almost_equal(U_8[1, 1, 1, 0], zeros)
    np.testing.assert_array_almost_equal(U_8[1, 1, 0, 1], zeros)
    np.testing.assert_array_almost_equal(U_8[1, 0, 1, 1], zeros)
    np.testing.assert_array_almost_equal(U_8[0, 1, 1, 1], zeros)

    np.testing.assert_array_almost_equal(U_8[1, 0, 1, 0], zeros)
    np.testing.assert_array_almost_equal(U_8[0, 1, 0, 1], zeros)

    # -- split in charge and spin
    
    # c+ c c+ c form of the charge, spin diagonalization
    U_c = - U_8[0, 0, 0, 0] - U_8[0, 0, 1, 1]
    U_s = U_8[0, 0, 0, 0] - U_8[0, 0, 1, 1]

    # c+ c+ c c  form of the charge, spin diagonalization
    #U_c = U_8[0, 0, 0, 0] + U_8[0, 1, 1, 0]
    #U_s = -U_8[0, 0, 0, 0] + U_8[0, 1, 1, 0]

    #U_c *= 4
    #U_s *= 4
    
    return U_c, U_s

# ----------------------------------------------------------------------    
def quartic_tensor_from_charge_and_spin(U_c, U_s):

    shape_4 = U_c.shape
    shape_8 = [2]*4 + list(shape_4)

    U_8 = np.zeros(shape_8, dtype=U_c.dtype)

    U_uu = -0.5 * (U_c - U_s)
    U_ud = +0.5 * (U_c + U_s)
    
    for s1, s2 in itertools.product(range(2), repeat=2):
        if s1 == s2:
            U_8[s1, s1, s1, s1] = U_uu
        else:
            U_8[s1, s1, s2, s2] = -U_ud
            U_8[s1, s2, s2, s1] = U_s

    U_8 = np.transpose(U_8, (0, 4, 1, 5, 2, 6, 3, 7))
    U_4 = U_8.reshape(2*np.array(shape_4))
    
    return U_4
