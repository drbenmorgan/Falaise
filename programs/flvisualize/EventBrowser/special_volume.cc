/* special_volume.cc
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

#include <EventBrowser/detector/special_volume.h>
#include <EventBrowser/utils/root_utilities.h>

#include <geomtools/geom_info.h>

#include <TObjArray.h>
#include <TPolyLine3D.h>

namespace snemo {

namespace visualization {

namespace detector {

bool special_volume::has_objects() const { return _objects_ != nullptr; }

// ctor:
special_volume::special_volume(const std::string &name_, const std::string &category_, const geomtools::geom_info& ginfo_)
    : i_root_volume(name_, category_, ginfo_) {
  _type = "special";
  _composite = false;
}

// dtor:
special_volume::~special_volume() {
  _objects_->Delete();
  _objects_ = nullptr;
}

void special_volume::_construct(const geomtools::i_shape_3d &shape_3d_) {
  _objects_ = utils::root_utilities::wires_to_root_draw(get_placement().get_translation(),
                                                        get_placement().get_rotation(), shape_3d_);
}

void special_volume::clear() {
  if (!has_objects()) {
    return;
  }

  TObjArrayIter iter(_objects_);
  while (iter.Next() != nullptr) {
    TObject *a_object = *iter;

    if (a_object->IsA() == TPolyLine3D::Class()) {
      auto *pl3d = dynamic_cast<TPolyLine3D *>(a_object);
      pl3d->SetLineWidth(1);
      pl3d->SetLineColor(_color);
    }
  }
}


void special_volume::_highlight() {
  _color = _highlight_color;
  this->clear();
}

void special_volume::draw() const {
  if (has_objects()) {
    _objects_->Draw();
  }
}

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

// end of special_volume.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
