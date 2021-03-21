// -*- mode: c++ ; -*-
/* event_server.h
 * Author (s) :   Xavier Garrido <<garrido@lal.in2p3.fr>>
 * Creation date: 2010-06-12
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
 *   Server to manage event data
 *
 * History:
 *
 * 2012-12-19 : Add methods for selection purposes
 * 2014-07-16 : Doxygenation
 */

#ifndef FALAISE_SNEMO_VISUALIZATION_IO_EVENT_SERVER_H
#define FALAISE_SNEMO_VISUALIZATION_IO_EVENT_SERVER_H 1

// Standard library:
#include <set>
#include <string>

// Third party:
// - Boost:
#include <boost/cstdint.hpp>
// - Bayeux/datatools:
#include <datatools/bit_mask.h>
#include <datatools/i_tree_dump.h>

// This project:
#include <EventBrowser/io/data_model.h>

namespace snemo {

namespace visualization {

namespace io {

class i_data_access;

/// \brief The event server to give access to data bank through a unique
/// interface.
/// This class allows the access to data through an interface
/// ```i_data_access``` that can read data from BRIO files as well as
/// Boost archive.
class event_server {
 public:
  /// File type enumeration
  enum class file_type {
    NONE = 0,
    UNKNOWN = 1,
    CORRUPT = 2,
    BOOST_SERIAL = 3,
    BRIO = 4,
    RAW = 5,
    DUMMY = 6
    //          eDAQ   // to get data directly from DAQ server
  };

  /// Default constructor
  event_server() = default;

  /// Destructor
  virtual ~event_server();

  /// Connect to the server
  bool connect(const std::vector<std::string>& filenames_);

  /// Return true if connect
  bool is_connected() const;

  /// Set initialization flag
  void set_connected(const bool initialized_);

  /// Close any existing connection
  bool close();

  /// Rewind data to the very first event
  bool rewind();

  /// Check if data has random access (BRIO)
  bool has_random_data() const;

  /// Check if data has sequential access (Boost)
  bool has_sequential_data() const;

  /// Check if data comes from external source
  bool has_external_data() const;

  // Needed because of event_browser_module
  // TODO: Better design to allow server to be constructed without
  // asumption of files as input. Likely needs an interface, then concrete impls
  /// Set external origin of data
  void set_external(const bool external_);

  /// Return the filename from the current event
  std::string get_current_filename() const;

  /// Return file type status as string
  std::string get_file_type_as_string() const;

  /// Return the number of events in the data stream
  size_t get_number_of_events() const;

  /// Return current event number
  int get_current_event_number() const;

  /// Set current event number
  void set_current_event_number(const int current_event_);

  /// Read ith event
  bool read_event(const unsigned int event_number_);

  /// Go to next event in data stream
  bool next_event();

  /// Setting event from an external source
  void set_external_event(event_record& external_event_);

  /// Getting a non-mutable reference to current event
  const event_record& get_event() const;

  /// Getting a mutable reference to current event
  event_record& get_event();

  /// Write current event to file
  bool store_event(const std::string& filename_ = "") const;

  /// Smart dumping of current event
  void dump_event(std::ostream& out_ = std::clog, const std::string& title_ = "",
                  const std::string& indent_ = "") const;


  /// Return true if the server holds a non-empty selection list
  bool has_event_selection() const;

  /// Clear event selection list
  void clear_selection();

  /// Add ith event to the selection
  void select_event(const int event_number_);

  /// Remove ith event from selection
  void remove_event(const int event_number_);

  /// Fill event selection list with all events in current stream
  void fill_selection();

  /// Get first event number in selection list
  int get_first_selected_event() const;

  /// Get last event number within selection list
  int get_last_selected_event() const;

private:
  /// Check if event server has opened data stream
  bool is_opened() const;

  /// Set sequential access to data
  void set_sequential(const bool sequential_);

  /// Set file type status
  void set_file_type(const file_type file_type_);

  /// Return file type status
  file_type get_file_type() const;

  /// Main opening function
  bool _at_open_(const std::vector<std::string>& filenames_);

 private:
   /// Event server status
  enum status_type {
    UNDEFINED = datatools::bit_mask::bit00,
    INITIALIZED = datatools::bit_mask::bit01,
    SEQUENTIAL = datatools::bit_mask::bit02,
    EXTERNAL = datatools::bit_mask::bit03
  };

 uint32_t _status_ = UNDEFINED;                //!< Event server status
  file_type _file_type_ = file_type::NONE;      //!< File type status
  i_data_access* _data_access_ = nullptr;       //!< Abstract object to data flow
  int _current_event_number_ = -1;              //!< Current event number
  event_record* _event_ = nullptr;              //!< Event record
  using event_selection_list_type = std::set<int32_t>;
  event_selection_list_type _event_selection_;  //!< Event selection list
};

}  // end of namespace io

}  // end of namespace visualization

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_VISUALIZATION_IO_EVENT_SERVER_H

// end of event_server.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
