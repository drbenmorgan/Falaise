/// \file falaise/TrackerPreClustering/interface.h
/* Author(s) :    Francois Mauger <mauger@lpccaen.in2p3.fr>
 * Creation date: 2012-03-30
 * Last modified: 2014-02-07
 *
 * Copyright 2012-2014 F. Mauger
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
 * You should have received a copy of the GNU General Public  License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Description:
 *
 *  Description of the setup/input/output interface data for the
 *  Tracker Pre-Clustering templatized algorithm.
 *
 */

#ifndef FALAISE_TRACKERPRECLUSTERING_INTERFACE_H
#define FALAISE_TRACKERPRECLUSTERING_INTERFACE_H 1

// Standard library:
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

// Third party:
// - Boost:
#include <boost/cstdint.hpp>
// - Bayeux/datatools:
#include <bayeux/datatools/logger.h>

namespace TrackerPreClustering {

/// \brief Setup data for the TrackerPreClustering algorithm
struct setup_data {
  /// Return the last error message
  const std::string &get_last_error_message() const;

  /// Set the last error message
  void set_last_error_message(const std::string &message_);

  /// Default constructor
  setup_data();

  /// Reset
  void reset();

  /// Check the setup data
  bool check() const;

  // Attributes:
  datatools::logger::priority logging;  //!< Logging flag
  double cell_size;                     //!< The dimension of a cell
  double delayed_hit_cluster_time;      //!< Delayed hit cluster time
  bool processing_prompt_hits;          //!< Flag for processing of prompt hits
  bool processing_delayed_hits;         //!< Flag for processing of delayed hits
  bool split_chamber;                   //!< Flag to split the chamber

 protected:
  std::string _last_error_message;  /// The last error message
};

/// \brief Input data structure
template <class Hit>
struct input_data {
  // Typedefs:
  typedef Hit hit_type;
  typedef std::vector<const hit_type *> hit_collection_type;

  /// Return the last error message
  const std::string &get_last_error_message() const;

  /// Set the last error message
  void set_last_error_message(const std::string &message_);

  /// Default constructor
  input_data();

  /// Reset
  void reset();

  /// Check
  bool check() const;

  // Attributes:
  hit_collection_type hits;  //!< Collection of Geiger hits

 protected:
  std::string _last_error_message;  //!< The last error message at check
};

/// \brief Output data structure
template <class Hit>
struct output_data {
  // Typedefs:
  typedef Hit hit_type;
  typedef std::vector<const hit_type *> hit_collection_type;
  typedef std::vector<hit_collection_type> cluster_collection_type;

  /// Default constructor
  output_data();

  /// Reset
  void reset();

  /// Print
  void dump(std::ostream &out_) const;

  // Attributes:
  hit_collection_type ignored_hits;          //!< Collection of ignored hits
  cluster_collection_type prompt_clusters;   //!< Collection of prompt clusters
  cluster_collection_type delayed_clusters;  //!< Collection of delayed clusters
};
template <class Hit>
const std::string& input_data<Hit>::get_last_error_message() const {
  return _last_error_message;
}

template <class Hit>
void input_data<Hit>::set_last_error_message(const std::string& message_) {
  _last_error_message = message_;
  return;
}

template <class Hit>
input_data<Hit>::input_data() {
  return;
}

template <class Hit>
void input_data<Hit>::reset() {
  hits.clear();
  _last_error_message.clear();
  return;
}

template <class Hit>
bool input_data<Hit>::check() const {
  input_data* mutable_this = const_cast<input_data*>(this);
  hit_collection_type tags;
  tags.reserve(hits.size());
  for (unsigned int i = 0; i < hits.size(); i++) {
    const hit_type* a_hit = hits.at(i);
    if (a_hit == 0) {
      std::ostringstream message;
      message << "TrackerPreClustering::input_data<>::check: "
              << "Null hit !";
      mutable_this->set_last_error_message(message.str());
      return false;
    }
    if (std::find(tags.begin(), tags.end(), a_hit) != tags.end()) {
      std::ostringstream message;
      message << "TrackerPreClustering::input_data<>::check: "
              << "Double referenced hit !";
      mutable_this->set_last_error_message(message.str());
      return false;
    }
    tags.push_back(a_hit);
    if (!a_hit->has_geom_id()) {
      std::ostringstream message;
      message << "TrackerPreClustering::input_data<>::check: "
              << "Missing GID !";
      mutable_this->set_last_error_message(message.str());
      return false;
    }
    if (!a_hit->has_xy()) {
      std::ostringstream message;
      message << "TrackerPreClustering::input_data<>::check: "
              << "Missing XY position !";
      mutable_this->set_last_error_message(message.str());
      return false;
    }
    if (a_hit->is_delayed() && !a_hit->has_delayed_time()) {
      std::ostringstream message;
      message << "TrackerPreClustering::input_data<>::check: "
              << "Missing delayed time !";
      mutable_this->set_last_error_message(message.str());
      return false;
    }
  }
  return true;
}

template <class Hit>
output_data<Hit>::output_data() {
  return;
}

template <class Hit>
void output_data<Hit>::reset() {
  ignored_hits.clear();
  prompt_clusters.clear();
  delayed_clusters.clear();
  return;
}

template <class Hit>
void output_data<Hit>::dump(std::ostream& out_) const {
  out_ << "TrackerPreClustering::output_data: " << std::endl;
  out_ << "|-- Ignored hits : " << ignored_hits.size() << std::endl;
  out_ << "|-- Prompt clusters: " << prompt_clusters.size() << std::endl;
  for (unsigned int i = 0; i < prompt_clusters.size(); i++) {
    if (i < prompt_clusters.size() - 1) {
      out_ << "|   |-- ";
    } else {
      out_ << "|   `-- ";
    }
    out_ << "Prompt cluster #" << i << "  size : " << prompt_clusters.at(i).size() << std::endl;
  }
  out_ << "`-- Delayed clusters: " << delayed_clusters.size() << std::endl;
  for (unsigned int i = 0; i < delayed_clusters.size(); i++) {
    if (i < delayed_clusters.size() - 1) {
      out_ << "    |-- ";
    } else {
      out_ << "    `-- ";
    }
    out_ << "Delayed cluster #" << i << "  size : " << delayed_clusters.at(i).size() << std::endl;
  }

  return;
}

}  // end of namespace TrackerPreClustering

#endif  // FALAISE_TRACKERPRECLUSTERING_INTERFACE_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
