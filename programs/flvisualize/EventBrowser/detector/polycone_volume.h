// -*- mode: c++ ; -*-
/* polycone_volume.h
 * Author (s) :     Xavier Garrido <<garrido@lal.in2p3.fr>>
 * Creation date: 2011-02-28
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
 *   Polycone volume for ROOT
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_VISUALIZATION_DETECTOR_POLYCONE_VOLUME_H
#define FALAISE_SNEMO_VISUALIZATION_DETECTOR_POLYCONE_VOLUME_H 1

#include <EventBrowser/detector/i_root_volume.h>

namespace snemo {

namespace visualization {

namespace detector {

// \brief A polycone volume
class polycone_volume : public i_root_volume {
 public:
  /// Default constructor
  polycone_volume(const std::string& name_ = "", const std::string& category_ = "");

  /// Destructor
  virtual ~polycone_volume();

 protected:
  /// Construct the polycone volume
  virtual void _construct(const geomtools::i_shape_3d& shape_3d_);

 private:
  size_t _nbr_z_section_;  //<! Number of z-section to describe the polycone
};

}  // end of namespace detector

}  // end of namespace visualization

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_VISUALIZATION_DETECTOR_POLYCONE_VOLUME_H

// end of polycone_volume.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
