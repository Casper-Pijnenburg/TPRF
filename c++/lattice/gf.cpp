/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2017, H. U.R. Strand
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <triqs/arrays/linalg/det_and_inverse.hpp>
using triqs::arrays::inverse;

#include "common.hpp"
#include "gf.hpp"

namespace tprf {
  
// ----------------------------------------------------
// g

#ifdef TPRF_OMP

gk_iw_t lattice_dyson_g0_wk(double mu, ek_vt e_k, g_iw_t::mesh_t mesh) {

  auto I = make_unit_matrix<ek_vt::scalar_t>(e_k.target_shape()[0]);
  gk_iw_t g0_wk = make_gf<gk_iw_t::mesh_t::var_t>({mesh, e_k.mesh()}, e_k.target());
  
#pragma omp parallel for 
  for (int idx = 0; idx < ek.mesh().size(); idx++) {
    auto iter = e_k.mesh().begin(); iter += idx; auto k = *iter;
    
    for (auto const &w : mesh) g0_wk[w, k] = inverse((w + mu)*I - e_k(k));
  }

  return g0_wk;
}

#else
  
gk_iw_t lattice_dyson_g0_wk(double mu, ek_vt e_k, g_iw_t::mesh_t mesh) {

  auto I = make_unit_matrix<ek_vt::scalar_t>(e_k.target_shape()[0]);
  gk_iw_t g0_wk = make_gf<gk_iw_t::mesh_t::var_t>({mesh, e_k.mesh()}, e_k.target());
  
  for (auto const &k : e_k.mesh())
    for (auto const &w : mesh) g0_wk[w, k] = inverse((w + mu)*I - e_k(k));

  return g0_wk;
}

#endif

// ----------------------------------------------------
  
#ifdef TPRF_OMP

gk_iw_t lattice_dyson_g_wk(double mu, ek_vt e_k, g_iw_vt sigma_w) {

  auto mesh = sigma_w.mesh();
  auto I = make_unit_matrix<ek_vt::scalar_t>(e_k.target_shape()[0]);
  gk_iw_t g_wk =
      make_gf<gk_iw_t::mesh_t::var_t>({mesh, e_k.mesh()}, e_k.target());

#pragma omp parallel for 
  for (int idx = 0; idx < ek.mesh().size(); idx++) {
    auto iter = e_k.mesh().begin(); iter += idx; auto k = *iter;
    
    for (auto const &w : mesh) g_wk[w, k] = inverse((w + mu)*I - e_k(k) - sigma_w[w]);
  }

  return g_wk;
}

#else
  
gk_iw_t lattice_dyson_g_wk(double mu, ek_vt e_k, g_iw_vt sigma_w) {

  auto mesh = sigma_w.mesh();
  auto I = make_unit_matrix<ek_vt::scalar_t>(e_k.target_shape()[0]);
  gk_iw_t g_wk =
      make_gf<gk_iw_t::mesh_t::var_t>({mesh, e_k.mesh()}, e_k.target());

  for (auto const &k : e_k.mesh())
    for (auto const &w : mesh) g_wk[w, k] = inverse((w + mu)*I - e_k(k) - sigma_w[w]);

  return g_wk;
}

#endif

// ----------------------------------------------------
  
#ifdef TPRF_OMP

gr_iw_t fourier_wk_to_wr(gk_iw_vt g_wk) {

  auto _ = all_t{};
  auto target = g_wk.target();

  //const auto & [ wmesh, kmesh ] = g_wk.mesh();
  auto wmesh = std::get<0>(g_wk.mesh());
  auto kmesh = std::get<1>(g_wk.mesh());
  auto rmesh = make_adjoint_mesh(kmesh);

  gr_iw_t g_wr = make_gf<gr_iw_t::mesh_t::var_t>({wmesh, rmesh}, target);

  auto w0 = *wmesh.begin();
  auto p = _fourier_plan<0>(gf_const_view(g_wk[w0, _]), gf_view(g_wr[w0, _]));

#pragma omp parallel for 
  for (int idx = 0; idx < wmesh.size(); idx++) {
    auto iter = wmesh.begin(); iter += idx; auto w = *iter;

    auto g_r = make_gf<cyclic_lattice>(rmesh, target);
    auto g_k = make_gf<brillouin_zone>(kmesh, target);

#pragma omp critical
    g_k = g_wk[w, _];

    _fourier_with_plan<0>(gf_const_view(g_k), gf_view(g_r), p);

#pragma omp critical
    g_wr[w, _] = g_r;

  }

  return g_wr;
}

#else
  
gr_iw_t fourier_wk_to_wr(gk_iw_vt g_wk) {

  auto [wmesh, kmesh] = g_wk.mesh();
  auto rmesh = make_adjoint_mesh(kmesh);

  gr_iw_t g_wr = make_gf<gr_iw_t::mesh_t::var_t>({wmesh, rmesh}, g_wk.target());

  auto _ = all_t{};
  for (auto const &w : wmesh) g_wr[w, _]() = fourier(g_wk[w, _]);

  return g_wr;
}

#endif

// ----------------------------------------------------

#ifdef TPRF_OMP
  
gk_iw_t fourier_wr_to_wk(gr_iw_vt g_wr) {

  auto _ = all_t{};
  auto target = g_wr.target();

  //auto [wmesh, rmesh] = g_wr.mesh();
  auto wmesh = std::get<0>(g_wr.mesh());
  auto rmesh = std::get<1>(g_wr.mesh());
  auto kmesh = make_adjoint_mesh(rmesh);
  
  gk_iw_t g_wk = make_gf<gk_iw_t::mesh_t::var_t>({wmesh, kmesh}, target);

  auto w0 = *wmesh.begin();
  auto p = _fourier_plan<0>(gf_const_view(g_wr[w0, _]), gf_view(g_wk[w0, _]));

#pragma omp parallel for 
  for (int idx = 0; idx < wmesh.size(); idx++) {
    auto iter = wmesh.begin(); iter += idx; auto w = *iter;

    auto g_r = make_gf<cyclic_lattice>(rmesh, target);
    auto g_k = make_gf<brillouin_zone>(kmesh, target);

#pragma omp critical
    g_r = g_wr[w, _];

    _fourier_with_plan<0>(gf_const_view(g_r), gf_view(g_k), p);

#pragma omp critical
    g_wk[w, _] = g_k;

  }
  
  return g_wk;
}

#else
  
gk_iw_t fourier_wr_to_wk(gr_iw_vt g_wr) {

  auto [wmesh, rmesh] = g_wr.mesh();
  auto kmesh = make_adjoint_mesh(rmesh);
  
  gk_iw_t g_wk = make_gf<gk_iw_t::mesh_t::var_t>({wmesh, kmesh}, g_wr.target());

  auto _ = all_t{};
  for (auto const &w : wmesh) g_wk[w, _]() = fourier(g_wr[w, _]);
  
  return g_wk;
}

#endif
  
// ----------------------------------------------------
// Transformations: Matsubara frequency <-> imaginary time

#ifdef TPRF_OMP
  
gr_tau_t fourier_wr_to_tr(gr_iw_vt g_wr, int nt) {

  auto wmesh = std::get<0>(g_wr.mesh());
  auto rmesh = std::get<1>(g_wr.mesh());

  double beta = wmesh.domain().beta;
  auto S = wmesh.domain().statistic;

  int nw = wmesh.last_index() + 1;
  if( nt <= 0 ) nt = 4 * nw;

  gr_tau_t g_tr = make_gf<gr_tau_t::mesh_t::var_t>(
      {{beta, S, nt}, rmesh}, g_wr.target());

  auto tmesh = std::get<0>(g_rt.mesh());

  auto _ = all_t{};

  auto r0 = *rmesh.begin();
  auto p = _fourier_plan<0>(gf_const_view(g_wr[_, r0]), gf_view(g_tr[_, r0]));

#pragma omp parallel for 
  for (int idx = 0; idx < rmesh.size(); idx++) {
    auto iter = rmesh.begin(); iter += idx; auto r = *iter;

    auto g_w = make_gf<imfreq>(wmesh, g_wr.target());
    auto g_t = make_gf<imtime>(tmesh, g_wr.target());

#pragma omp critical
    g_w = g_wr[_, r];

    _fourier_with_plan<0>(gf_const_view(g_w), gf_view(g_t), p);

#pragma omp critical
    g_tr[_, r] = g_t;

  }

  return g_tr;
}

#else
  
gr_tau_t fourier_wr_to_tr(gr_iw_vt g_wr, int nt) {

  auto wmesh = std::get<0>(g_wr.mesh());
  auto rmesh = std::get<1>(g_wr.mesh());

  double beta = wmesh.domain().beta;

  int nw = wmesh.last_index() + 1;
  if( nt <= 0 ) nt = 4 * nw;

  gr_tau_t g_tr = make_gf<gr_tau_t::mesh_t::var_t>(
    {{beta, wmesh.domain().statistic, nt}, rmesh}, g_wr.target());

  auto _ = all_t{};
  for (auto const &r : rmesh) g_tr[_, r]() = fourier<0>(g_wr[_, r]);

  return g_tr;
}

#endif
  
} // namespace tprf