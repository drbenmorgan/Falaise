/* polycone_volume.cc
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

#include <EventBrowser/detector/polycone_volume.h>

#include <EventBrowser/utils/root_utilities.h>

#include <geomtools/geom_info.h>
#include <geomtools/polycone.h>

#include <TGeoManager.h>
#include <TGeoPcon.h>

namespace snemo {

namespace visualization {

namespace detector {

// ctor:
polycone_volume::polycone_volume(const std::string& name_, const std::string& category_)
    : i_root_volume(name_, category_) {
  _type = "polycone";
  _composite = false;

  _nbr_z_section_ = 0;
}

// dtor:
polycone_volume::~polycone_volume() = default;

void polycone_volume::_construct(const geomtools::i_shape_3d& shape_3d_) {
  const auto& mpolycone = dynamic_cast<const geomtools::polycone&>(shape_3d_);

  _nbr_z_section_ = mpolycone.points().size();

  TGeoShape* geo_shape = utils::root_utilities::get_geo_shape(mpolycone);
  geo_shape->SetName(_name.c_str());

  auto* material = new TGeoMaterial("Dummy");
  auto* medium = new TGeoMedium("Dummy", 1, material);

  _geo_volume = new TGeoVolume(_name.c_str(), geo_shape, medium);
}

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

// end of polycone_volume.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
