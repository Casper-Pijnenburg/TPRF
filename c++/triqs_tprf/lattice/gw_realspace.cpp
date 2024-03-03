#include <iostream>
#include <typeinfo>
#include <nda/nda.hpp>
#include <nda/linalg/eigenelements.hpp>
#include <triqs/gfs.hpp>
#include <triqs/mesh.hpp>
#include <iomanip>

#include "common.hpp"
#include "lattice_utility.hpp"
#include "../fourier/fourier.hpp"
#include "../mpi.hpp"
#include <omp.h>

namespace triqs_tprf {

    b_g_Dt_t iw_to_tau(b_g_Dw_cvt g_w) {
        auto dlr = make_gf_dlr(g_w);
        return make_gf_dlr_imtime(dlr);
    }

    b_g_Dw_t tau_to_iw(b_g_Dt_cvt g_w) {
        auto dlr = make_gf_dlr(g_w);
        return make_gf_dlr_imfreq(dlr);
    }

    b_g_Dt_t iw_to_tau_p(b_g_Dw_cvt g_w, int num_cores) {
        omp_set_num_threads(num_cores);
        auto iw_mesh = g_w[0].mesh();
        auto tau_mesh = make_adjoint_mesh(iw_mesh);
        int iw_size = iw_mesh.size();
        int tau_size = tau_mesh.size();

        int size = g_w[0].target().shape()[0];

        auto gf_temp = gf(tau_mesh, g_w[0].target().shape());
        auto g_tau = make_block_gf<dlr_imtime>(g_w.block_names(), {gf_temp, gf_temp});


        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            auto g_up = gf(iw_mesh, {1, size});
            auto g_dn = gf(iw_mesh, {1, size});

            for (int w = 0; w < iw_size; ++w){
                g_up[w](0, range::all) = g_w[0][w](i, range::all);
                g_dn[w](0, range::all) = g_w[1][w](i, range::all);
            }

            auto g_up_dlr = make_gf_dlr(g_up);
            auto g_dn_dlr = make_gf_dlr(g_dn);
            auto g_up_t = make_gf_dlr_imtime(g_up_dlr);
            auto g_dn_t = make_gf_dlr_imtime(g_dn_dlr);

            for (int t = 0; t < tau_size; ++t){
                g_tau[0][t](i, range::all) = g_up_t[t](0, range::all);
                g_tau[1][t](i, range::all) = g_dn_t[t](0, range::all);
            }
        }

        return g_tau;
    }


    b_g_Dw_t tau_to_iw_p(b_g_Dt_cvt g_t, int num_cores) {
        omp_set_num_threads(num_cores);
        auto tau_mesh = g_t[0].mesh();
        auto iw_mesh = make_adjoint_mesh(tau_mesh);
        int tau_size = tau_mesh.size();
        int iw_size = iw_mesh.size();
        int size = g_t[0].target().shape()[0];

        auto gf_temp = gf(iw_mesh, g_t[0].target().shape());
        auto g_w = make_block_gf<dlr_imfreq>(g_t.block_names(), {gf_temp, gf_temp});


        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            auto g_up = gf(tau_mesh, {1, size});
            auto g_dn = gf(tau_mesh, {1, size});

            for (int t = 0; t < tau_size; ++t){
                g_up[t](0, range::all) = g_t[0][t](i, range::all);
                g_dn[t](0, range::all) = g_t[1][t](i, range::all);
            }

            auto g_up_dlr = make_gf_dlr(g_up);
            auto g_dn_dlr = make_gf_dlr(g_dn);
            auto g_up_t = make_gf_dlr_imfreq(g_up_dlr);
            auto g_dn_t = make_gf_dlr_imfreq(g_dn_dlr);

            for (int w = 0; w < iw_size; ++w){
                g_w[0][w](i, range::all) = g_up_t[w](0, range::all);
                g_w[1][w](i, range::all) = g_dn_t[w](0, range::all);
            }
        }

        return g_w;
    }

    b_g_Dw_t dyson_mu(b_g_Dw_t g_w, double mu, int num_cores) {
        omp_set_num_threads(num_cores);
        auto iw_mesh = g_w[0].mesh();
        int size = g_w[0].target().shape()[0];

        matrix<double> Mu(size, size);
        Mu() = mu;

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            g_w[0][i] = inverse(inverse(g_w[0][i]) - Mu);
            g_w[1][i] = inverse(inverse(g_w[1][i]) - Mu);
        }

        return g_w;
    }

    b_g_Dw_t dyson_mu_sigma(b_g_Dw_t g_w, double mu, b_g_Dw_t sigma_w, int num_cores) {
        omp_set_num_threads(num_cores);
        auto iw_mesh = g_w[0].mesh();
        int size = g_w[0].target().shape()[0];

        matrix<double> Mu(size, size);
        Mu() = mu;

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            g_w[0][i] = inverse(inverse(g_w[0][i]) - Mu - sigma_w[0][i]);
            g_w[1][i] = inverse(inverse(g_w[1][i]) - Mu - sigma_w[1][i]);
        }
        
        return g_w;
    }

    double total_density(b_g_Dw_t g_w, int num_cores) {
        omp_set_num_threads(num_cores);

        auto iw_mesh = g_w[0].mesh();
        int iw_size = iw_mesh.size();
        int size = g_w[0].target().shape()[0];

        double tot = 0.0;

        for (int i = 0; i < size; ++i) {
            auto g_up = gf<dlr_imfreq, scalar_valued>{iw_mesh};
            auto g_dn = gf<dlr_imfreq, scalar_valued>{iw_mesh};

            for (int w = 0; w < iw_size; ++w){
                g_up[w] = g_w[0][w](i, i);
                g_dn[w] = g_w[1][w](i, i);
            }

            auto g_up_dlr = make_gf_dlr(g_up);
            auto g_dn_dlr = make_gf_dlr(g_dn);
            tot += density(g_up_dlr).real() + density(g_dn_dlr).real();
        }

        return tot;
    }

    g_Dw_t inv(g_Dw_t g_w, int num_cores) {
        omp_set_num_threads(num_cores);
        // auto g_w_inv = gf(g_w);
        auto iw_mesh = g_w.mesh();

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            g_w[i] = inverse(g_w[i]);
        }

        return g_w;
    }


    b_g_Dw_t polarization(b_g_Dw_cvt g_w, dlr_imfreq iw_mesh_b, int num_cores) {
        omp_set_num_threads(num_cores);

        auto tau_mesh_b = make_adjoint_mesh(iw_mesh_b);
        int tau_mesh_size = tau_mesh_b.size();

        int orbitals = g_w[0].target().shape()[0];
        auto g_t = iw_to_tau_p(g_w, num_cores);

        auto gf_temp = gf(tau_mesh_b, g_w[0].target().shape());
        auto P_t = make_block_gf<dlr_imtime>(g_w.block_names(), {gf_temp, gf_temp});
        
        
        #pragma omp parallel for
        for (int a = 0; a < orbitals; ++a) {
            for (int b = 0; b < orbitals; ++b) {
                for (int j = 0; j < 2; ++j) {
                    for (int i = 0; i < tau_mesh_size; ++i) {
                        P_t[j][i](a, b) = -1.0 * g_t[j][i](a, b) * g_t[j][tau_mesh_size - i - 1](b, a);
                    }
                }
            }
        }

        
        return tau_to_iw_p(P_t, num_cores);
    }

    b_g_Dw_t screened_potential(b_g_Dw_cvt P, matrix<double> V, bool self_interactions, int num_cores) {
        omp_set_num_threads(num_cores);
         
        auto iw_mesh = P[0].mesh();
        int size = P[0].target().shape()[0];
        auto I = nda::eye<double>(size);

        auto V_t = V;

        if (!self_interactions) {
            #pragma omp parallel for
            for (int i = 0; i < size; ++i) {
                V_t(i, i) = 0;
            }
        }

        auto W_up = gf(iw_mesh, P[0].target().shape());
        auto W_dn = gf(iw_mesh, P[0].target().shape());

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            
            auto A = I - V_t * P[0][i];
            auto B =   -   V * P[1][i];
            auto C =   -   V * P[0][i];
            auto D = I - V_t * P[1][i];

            auto Ainv = inverse(A);

            auto S = inverse(D - C * Ainv * B);

            W_up[i] = (Ainv + Ainv * B * S * C * Ainv) * V_t - Ainv * B * S * V;
            W_dn[i] = -S * C * Ainv * V + S * V_t;
        }
        
        auto W = make_block_gf<dlr_imfreq>(P.block_names(), {W_up, W_dn});
        return W;
    }


    b_g_Dw_t dyn_self_energy(b_g_Dw_cvt G, b_g_Dw_cvt W, matrix<double> V, bool self_interactions, int num_cores) {
        omp_set_num_threads(num_cores);
         
        auto iw_mesh_f = G[0].mesh();
        auto iw_mesh_b = W[0].mesh();
        int size = G[0].target().shape()[0];

        auto V_t = V;

        if (!self_interactions) {
            #pragma omp parallel for
            for (int i = 0; i < size; ++i) {
                V_t(i, i) = 0;
            }
        }

        auto W_dyn = block_gf(W);
        

        
        #pragma omp parallel for
        for (int j = 0; j < iw_mesh_b.size(); ++j) {
            for (int i = 0; i < 2; ++i) {
                    W_dyn[i][j] = W[i][j] - V_t;
            }    
        }

        auto W_dyn_t = iw_to_tau_p(W_dyn, num_cores);
        auto G_t = iw_to_tau_p(G, num_cores);
        auto sigma_t = block_gf(G_t);

        
        #pragma omp parallel for
        for (int a = 0; a < size; ++a) {
            for (int b = 0; b < size; ++b) {
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < iw_mesh_f.size(); ++j) {
                        sigma_t[i][j](a, b) = -W_dyn_t[i][j](a, b) * G_t[i][j](a, b);
                    }
                }
            }
        }
    
        return tau_to_iw_p(sigma_t, num_cores);
    }

    std::vector<matrix<std::complex<double>>> density(b_g_Dw_cvt G, int num_cores) {
        omp_set_num_threads(num_cores);

        auto iw_mesh = G[0].mesh();
        int iw_size = iw_mesh.size();
        int size = G[0].target().shape()[0];

        matrix<std::complex<double>> rho_up(size, size);
        matrix<std::complex<double>> rho_dn(size, size);

        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            auto g_up = gf(iw_mesh, {1, size});
            auto g_dn = gf(iw_mesh, {1, size});

            for (int w = 0; w < iw_size; ++w){
                g_up[w](0, range::all) = G[0][w](i, range::all);
                g_dn[w](0, range::all) = G[1][w](i, range::all);
            }

            auto g_up_dlr = make_gf_dlr(g_up);
            auto g_dn_dlr = make_gf_dlr(g_dn);


            rho_up(i, range::all) = density(g_up_dlr)(0, range::all);
            rho_dn(i, range::all) = density(g_dn_dlr)(0, range::all);
        }

        std::vector<matrix<std::complex<double>>> rho = {rho_up, rho_dn};
        return rho;
    }


    b_g_Dw_t hartree_self_energy(b_g_Dw_cvt G, matrix<double> V, bool self_interactions, int num_cores) {
        omp_set_num_threads(num_cores);

        auto iw_mesh = G[0].mesh();
        int size = G[0].target().shape()[0];

        auto V_t = V;

        if (!self_interactions) {
            #pragma omp parallel for
            for (int i = 0; i < size; ++i) {
                V_t(i, i) = 0;
            }
        }

        std::vector<matrix<std::complex<double>>> rho = density(G, num_cores);


        matrix<double> hartree_up(size, size);
        matrix<double> hartree_dn(size, size);
        hartree_up() = 0.0;
        hartree_dn() = 0.0;

        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                hartree_up(i, i) += V_t(i, j) * rho[0](j, j).real() + V(i, j) * rho[1](j, j).real();
                hartree_dn(i, i) += V(i, j) * rho[0](j, j).real() + V_t(i, j) * rho[1](j, j).real();
            }
        }

        auto g = gf(iw_mesh, G[0].target().shape());
        auto hartree = make_block_gf<dlr_imfreq>(G.block_names(), {g, g});

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            hartree[0][i] = hartree_up;
            hartree[1][i] = hartree_dn;
        } 

        return hartree;
    }

    b_g_Dw_t fock_self_energy(b_g_Dw_cvt G, matrix<double> V, bool self_interactions, int num_cores) {
        omp_set_num_threads(num_cores);

        auto iw_mesh = G[0].mesh();
        int size = G[0].target().shape()[0];

        auto V_t = V;

        if (!self_interactions) {
            #pragma omp parallel for
            for (int i = 0; i < size; ++i) {
                V_t(i, i) = 0;
            }
        }


        std::vector<matrix<std::complex<double>>> rho = density(G, num_cores);

        matrix<double> fock_up(size, size);
        matrix<double> fock_dn(size, size);
        fock_up() = 0.0;
        fock_dn() = 0.0;

        #pragma omp parallel for
        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                fock_up(i, j) = -V_t(i, j) * rho[0](i, j).real();
                fock_dn(i, j) = -V_t(i, j) * rho[1](i, j).real();
            }
        }

        auto g = gf(iw_mesh, G[0].target().shape());
        auto fock = make_block_gf<dlr_imfreq>(G.block_names(), {g, g});

        #pragma omp parallel for
        for (int i = 0; i < iw_mesh.size(); ++i) {
            fock[0][i] = fock_up;
            fock[1][i] = fock_dn;
        } 

        return fock;
    }


}
