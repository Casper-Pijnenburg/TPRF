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
#pragma once

#include <nda/nda.hpp>
#include <triqs/gfs.hpp>
#include <triqs/mesh.hpp>

using namespace triqs::gfs;
using namespace triqs::mesh;
using namespace nda;

#include "types.hpp"

namespace triqs_tprf {

  // Forward declaration
  template <Channel_t C> std::array<int, 6> channel_stride_order;

  // ----------------------------------------------------
  // Index grouping definitions
  // ----------------------------------------------------

  // The standard layout is:
  //
  // {nu_1, nu_2, a, b, c, d} <=> {0, 1, 2, 3, 4, 5}
  //
  // where nu_1 and nu_2 (nu_1=0, nu_2=1) are fermionic Matsubara frequencies
  // and a, b, c, d (a=2, b=3, c=4, d=5) are target-space indices

  // ----------------------------------------------------
  // Channel_t::PH

  // in the particle-hole channel (Channel_t::PH) the indices are grouped as
  // {nu_1, a, b}, {nu_2, d, c} <=> {0, 2, 3}, {1, 5, 4}

  template <> inline constexpr std::array<int, 6> channel_stride_order<Channel_t::PH> = {0, 2, 3, 1, 5, 4};

  // ----------------------------------------------------
  // Channel_t::PH_bar

  // in the particle-hole-bar channel (Channel_t::PH_bar) the indices are grouped
  // as
  // {nu_1, a, d}, {nu_2, c, b} <=> {0, 2, 5}, {1, 4, 3}

  template <> inline constexpr std::array<int, 6> channel_stride_order<Channel_t::PH_bar> = {0, 2, 5, 1, 4, 3};

  // ----------------------------------------------------
  // Channel_t::PP

  // in the particle-particle channel (Channel_t::PP) the indices are grouped as
  // {nu_1, a, c}, {nu_2, b, d} <=> {0, 2, 4}, {1, 3, 5}

  template <> inline constexpr std::array<int, 6> channel_stride_order<Channel_t::PP> = {0, 2, 4, 1, 3, 5};

  // ----------------------------------------------------
  // channel_matrix_view
  //
  // Takes a contiguous 6-dimensional array_view with the layout
  // of a given channel and returns the associated matrix_view

  template <Channel_t C> using channel_memory_layout = contiguous_layout_with_stride_order<encode(channel_stride_order<C>)>;

  template <Channel_t C>
  inline nda::matrix_view<g2_iw_t::scalar_t> channel_matrix_view(array_contiguous_view<g2_iw_t::scalar_t, 6, channel_memory_layout<C>> arr) {
    constexpr std::array<int, 6> str_ord = channel_stride_order<C>;
    return group_indices_view(arr, idx_group<str_ord[0], str_ord[1], str_ord[2]>, idx_group<str_ord[3], str_ord[4], str_ord[5]>);
  }
} // namespace triqs_tprf
