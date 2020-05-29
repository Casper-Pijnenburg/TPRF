# ----------------------------------------------------------------------

import os
import glob
import numpy as np

# ----------------------------------------------------------------------
from triqs.gf import *
from h5 import HDFArchive

# ----------------------------------------------------------------------    

from triqs.operators import n
from triqs.operators.util.op_struct import set_operator_structure

from triqs.plot.mpl_interface import oplot, oplotr, oploti, plt     

# ----------------------------------------------------------------------

from brew_dmft.ParameterCollection import ParameterCollection

# ----------------------------------------------------------------------
def calc_field(plot=True):

    filenames = glob.glob('data_pyed_h_field*.h5')

    out = ParameterCollection()
    d = ParameterCollection(data=[])
    h_vec, m_vec, m_ref_vec = [], [], []
    
    for filename in filenames:

        print('--> Loading:', filename)
        
        with HDFArchive(filename, 'r') as s:
            p = s['p']            
            d.data.append(p)
            h_vec.append(p.h_field)

            m = 0.5*(- p.G_tau['up'](p.beta) + p.G_tau['dn'](p.beta))
            m_vec.append(np.squeeze(m))

            m_ref_vec.append(p.magnetization)

            # Susceptibilit from quadratic expectation value
            if np.abs(p.h_field) < 1e-9:
                out.chi_exp = p.magnetization2 * 2 * p.beta

    h_vec, m_vec, m_ref_vec = np.array(h_vec), np.array(m_vec), np.array(m_ref_vec)
    sidx = np.argsort(h_vec)
    d.h_vec, d.m_vec, d.m_ref_vec = h_vec[sidx], m_vec[sidx], m_ref_vec[sidx]

    from scipy.interpolate import InterpolatedUnivariateSpline as IUS

    spl = IUS(d.h_vec, d.m_ref_vec)
    out.chi = -spl(0, nu=1) # Linear response
    out.beta = p.beta

    print('beta, chi, chi_exp =', out.beta, out.chi, out.chi_exp)

    filename_out = 'data_pyed_extrap_h_field_beta%6.6f.h5' % out.beta
    with HDFArchive(filename_out, 'w') as s:
        s['field'] = out

    for key, value in list(out.dict().items()):
        setattr(d, key, value)

    if plot: plot_field(d)
        
# ----------------------------------------------------------------------
def plot_field(out):

    plt.figure(figsize=(3.25*2, 8))
    for p in out.data:

        subp = [2, 1, 1]

        ax = plt.subplot(*subp); subp[-1] += 1
        oplotr(p.G_tau['up'], 'b', alpha=0.25)
        oplotr(p.G_tau['dn'], 'g', alpha=0.25)
        ax.legend().set_visible(False)

    subp = [2, 1, 2]
    plt.subplot(*subp); subp[-1] += 1
    plt.title(r'$\chi \approx %2.2f$' % out.chi)
    plt.plot(out.h_vec, out.m_vec, '-og', alpha=0.5)
    plt.plot(out.h_vec, out.m_ref_vec, 'xb', alpha=0.5)
    plt.plot(out.h_vec, -out.chi * out.h_vec, '-r', alpha=0.5)

    plt.tight_layout()
    plt.savefig('figure_static_field.pdf')

# ----------------------------------------------------------------------
if __name__ == '__main__':

    paths = glob.glob('pyed_*')

    for path in paths:
        print('--> path:', path)
        os.chdir(path)
        calc_field()
        os.chdir('../')

    plt.show()
    
