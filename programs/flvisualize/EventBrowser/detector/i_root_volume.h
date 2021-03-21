// -*- mode: c++ ; -*-
/* i_root_volume.h
 * Author (s) :   Xavier Garrido <<garrido@lal.in2p3.fr>>
 * Creation date: 2011-02-24
 * Last modified: 2014-07-11
 *
 * Copyright (C) 2011-2014 Xavier Garrido <garrido@lal.in2p3.fr>
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
 *
 * Description:
 *   Interface to ROOT volume
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_VISUALIZATION_DETECTOR_I_ROOT_VOLUME_H
#define FALAISE_SNEMO_VISUALIZATION_DETECTOR_I_ROOT_VOLUME_H 1

#include <EventBrowser/detector/i_volume.h>

class TGeoVolume;

namespace geomtools {
class i_shape_3d;
}

namespace snemo {

namespace visualization {

namespace detector {

/// \brief A dedicated ROOT volume object
class i_root_volume : public i_volume {
 public:
  /// Default constructor
  i_root_volume(const std::string& name_, const std::string& category_,
                const geomtools::geom_info& ginfo_);

  /// Destructor
  virtual ~i_root_volume();

  /// Get a mutable pointer to volume (TGeoVolume cast)
  virtual void* get_volume();

  /// Get a non-mutable pointer to volume (TGeoVolume cast)
  virtual const void* get_volume() const;

  /// Update method to refresh volume properties
  virtual void update();

  /// Clear volume properties
  virtual void clear();

  /// Highlight volume
  virtual void highlight(const size_t color_ = 0);

 protected:
  /// Implement dedicated highlight rendering
  virtual void _highlight();

  /// Implement dedicated construct method
  virtual void _construct(const geomtools::i_shape_3d& shape_3d_) = 0;

  bool _initialized = false;          //<! Initialization flag
  TGeoVolume* _geo_volume = nullptr;  //<! ROOT geometry volume

 private:
  void _build(const geomtools::geom_info& ginfo_);
};

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_VISUALIZATION_DETECTOR_I_ROOT_VOLUME_H

// end of i_root_volume.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
