/* i_root_volume.cc
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

#include <stdexcept>

#include <EventBrowser/detector/i_root_volume.h>
#include <EventBrowser/view/style_manager.h>

#include <geomtools/geom_info.h>

#include <TGeoMatrix.h>
#include <TGeoVolume.h>
#include <TMath.h>

namespace snemo {

namespace visualization {

namespace detector {

// ctor:
i_root_volume::i_root_volume(const std::string& name_, const std::string& category_) {
  _name = name_;
  _category = category_;
  _geo_volume = nullptr;
  _initialized = false;
}

// dtor:
i_root_volume::~i_root_volume() { this->reset(); }

bool i_root_volume::is_initialized() const { return _initialized; }

void* i_root_volume::get_volume() {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Not initialized !");
  return _geo_volume;
}

const void* i_root_volume::get_volume() const {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Not initialized !");
  return _geo_volume;
}

void i_root_volume::initialize(const geomtools::geom_info& ginfo_) {
  _placement = ginfo_.get_world_placement();
  _construct(ginfo_.get_logical().get_shape());
  _initialized = true;
}

void i_root_volume::update() {
  const view::style_manager& style_mgr = view::style_manager::get_instance();

  _color = style_mgr.get_volume_color(_category);
  _transparency = style_mgr.get_volume_transparency(_category);
  _visibility = style_mgr.get_volume_visibility(_category) == VISIBLE;

  if (is_initialized()) {
    this->clear();
  }
}

void i_root_volume::clear() {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Volume '" << _name << "' is not initialized!");
  _geo_volume->SetLineColor(_color);
  _geo_volume->SetFillColor(_color);
  _geo_volume->SetLineWidth(1);
  _geo_volume->SetTransparency(_transparency);
  _geo_volume->SetVisibility(_visibility);
}

void i_root_volume::reset() {
  delete _geo_volume;
  _geo_volume = nullptr;
  _initialized = false;
}

void i_root_volume::highlight(const size_t color_) {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Volume '" << get_name() << "' is not initialized!");
  _highlight_color = color_;
  _highlight();
}

void i_root_volume::_highlight() {
  _geo_volume->SetLineColor(_highlight_color);
  _geo_volume->SetLineWidth(3);
  // 2015/09/05 XG: Set the visibility of highlighted volume always to
  // true. To remove them from the view then disable the corresponding
  // data bank.
  // _geo_volume->SetVisibility(_visibility);
  _geo_volume->SetVisibility(true);
}

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

// end of i_root_volume.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
