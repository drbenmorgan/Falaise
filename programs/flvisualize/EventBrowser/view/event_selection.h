// -*- mode: c++ ; -*-
/* event_selection.h
 * Author (s) :   Xavier Garrido <<garrido@lal.in2p3.fr>>
 * Creation date: 2010-06-12
 * Last modified: 2012-12-19
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
 *
 * Description:
 *   definition of cut selection
 *
 * History:
 *
 * 2016-04-02 : Adding base_widget class to create widget
 * 2012-12-19 : Refactoring event selection
 */

#ifndef FALAISE_SNEMO_VISUALIZATION_VIEW_EVENT_SELECTION_H
#define FALAISE_SNEMO_VISUALIZATION_VIEW_EVENT_SELECTION_H 1

// Standard library:
#include <map>

// - ROOT:
#include <Rtypes.h>

// - This project:
#include <EventBrowser/view/event_browser_signals.h>

// Forward declaration
class TGCompositeFrame;
class TGTextButton;

namespace cuts {
class cut_manager;
}

namespace snemo {

namespace visualization {

namespace io {
class event_server;
}

namespace view {

class event_browser;
class status_bar;
class base_widget;

/// \brief A class hosting interactive event selection cut
class event_selection {
 public:
  // explicit Default constructor needed for root
  event_selection() = default;

  /// Constructor
  event_selection(TGCompositeFrame* main_, io::event_server* server_, view::status_bar* status_);

  /// Destructor
  virtual ~event_selection();

  /// Return a non-mutable reference to event server
  const io::event_server& get_event_server() const;

  /// Return a non-mutable reference to cut manager
  const cuts::cut_manager& get_cut_manager() const;

  /// Return a mutable reference to cut manager
  cuts::cut_manager& get_cut_manager();

  /// Event selection
  void select_events(const button_signals_type signal_ = UNDEFINED, const int event_selected_ = -1);

  /// Main process method
  void process();

 private:
  /// Check if selection is enabled
  bool is_selection_enable() const;

  /// Initialization
  void initialize(TGCompositeFrame* main_);

  /// Reset
  void reset();

  /// Build GUI widgets
  void _build_();

  /// Install cut manager
  void _install_cut_manager_();

  /// Build cuts
  void _build_cuts_();

  /// Check cuts
  bool _check_cuts_();

 private:
  /// Collection of widgets
  typedef std::map<int, base_widget*> widget_collection_type;

  bool _initialized_ = false;
  bool _selection_enable_ = false;

  int _initial_event_id_ = 0;

  TGCompositeFrame* _main_ = nullptr;
  io::event_server* _server_ = nullptr;
  event_browser* _browser_ = nullptr;
  status_bar* _status_ = nullptr;

  cuts::cut_manager* _cut_manager_ = nullptr;  //!< Cut manager pointer
  std::string _current_cut_name_;

  TGTextButton* _load_button_ = nullptr;
  TGTextButton* _save_button_ = nullptr;
  TGTextButton* _reset_button_ = nullptr;
  TGTextButton* _update_button_ = nullptr;
  widget_collection_type _widgets_;

  // No I/O so ClassDefVersionID = 0
  ClassDef(event_selection, 0);
};

}  // end of namespace view

}  // end of namespace visualization

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_VISUALIZATION_VIEW_EVENT_SELECTION_H

// end of event_selection.h
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
