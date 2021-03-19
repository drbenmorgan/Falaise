/* tube_volume.cc
 *
 * Copyright (C) 2011 Xavier Garrido <garrido@lal.in2p3.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <EventBrowser/detector/tube_volume.h>

#include <geomtools/geom_info.h>
#include <geomtools/tube.h>

#include <TGeoManager.h>

namespace snemo {

namespace visualization {

namespace detector {

// ctor:
tube_volume::tube_volume(const std::string &name_, const std::string &category_)
    : i_root_volume(name_, category_) {
  _type = "tube";
  _composite = false;

  _inner_radius_ = 0.0;
  _outer_radius_ = 0.0;
  _height_ = 0.0;
}

// dtor:
tube_volume::~tube_volume() = default;

void tube_volume::_construct(const geomtools::i_shape_3d &shape_3d_) {
  const auto &mtube = dynamic_cast<const geomtools::tube &>(shape_3d_);

  _inner_radius_ = mtube.get_inner_r();
  _outer_radius_ = mtube.get_outer_r();
  _height_ = mtube.get_z();

  auto *material = new TGeoMaterial("Dummy");
  auto *medium = new TGeoMedium("Dummy", 1, material);

  _geo_volume =
      gGeoManager->MakeTube(_name.c_str(), medium, _inner_radius_, _outer_radius_, _height_ / 2.);
}

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

// end of tube_volume.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
