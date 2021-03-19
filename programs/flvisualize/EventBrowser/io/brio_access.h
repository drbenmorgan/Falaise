// -*- mode: c++ ; -*-
/* brio_access.h
 * Author (s) :     Xavier Garrido <<garrido@lal.in2p3.fr>>
 * Creation date: 2010-12-09
 * Last modified: 2014-07-16
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
 *   Access to BRIO files
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_VISUALIZATION_IO_BRIO_ACCESS_H
#define FALAISE_SNEMO_VISUALIZATION_IO_BRIO_ACCESS_H 1

// This project
#include <EventBrowser/io/i_data_access.h>

// Standard library:
#include <map>
#include <string>
#include <vector>

namespace brio {
class reader;
}

namespace snemo {

namespace visualization {

namespace io {

/// \brief A data access class to read BRIO file
class brio_access : public i_data_access {
 public:
  /// Default constructor
  brio_access();

  /// Destructor
  virtual ~brio_access();

  /// Return the total number of entry
  virtual size_t get_number_of_entries() const;

  /// Return a file type as string
  virtual std::string get_file_type_as_string() const;

  /// Return current filename
  virtual const std::string& get_current_filename() const;

  /// Open data stream
  virtual bool open(const std::vector<std::string>& filenames_);

  /// Check file validity
  virtual bool is_valid(const std::vector<std::string>& filenames_) const;

  /// Check if current file can be read
  virtual bool is_readable();

  /// Check if data stream is opened
  virtual bool is_opened() const;

  /// Rewind data stream
  virtual bool rewind();

  /// Reset
  virtual bool reset();

  /// Close data stream
  virtual bool close();

  /// Build list of event
  virtual bool build_list();

  /// Retrieve event record from a given event number
  virtual bool retrieve_event(event_record& event_, const size_t event_number_);

 private:
  size_t _number_of_entries_;          //!< Total number of entries
  unsigned int _current_file_number_;  //!< Current file index

  std::vector<std::string> _file_list_;                       //!< List of data file
  std::map<size_t, std::pair<size_t, size_t> > _entry_list_;  //!< File limits

  std::string _mode_;      //!< Bank mode
  brio::reader* _reader_;  //!< Brio reader
};

}  // end of namespace io

}  // end of namespace visualization

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_VISUALIZATION_IO_BRIO_ACCESS_H

// end of brio_access.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
