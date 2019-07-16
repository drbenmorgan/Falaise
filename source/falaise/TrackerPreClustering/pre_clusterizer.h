/// \file falaise/TrackerPreClustering/pre_clusterizer.h
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
 *  Tracker Pre-Clustering algorithm.
 *
 * History:
 *
 *   This set of classes have been ported from the Channel metapackage.
 */

#ifndef FALAISE_TRACKERPRECLUSTERING_PRE_CLUSTERIZER_H
#define FALAISE_TRACKERPRECLUSTERING_PRE_CLUSTERIZER_H 1

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/logger.h>

// This project:
#include "falaise/TrackerPreClustering/interface.h"

namespace TrackerPreClustering {

/// \brief A pre-clusterizer of Geiger hits for the SuperNEMO detector
/** This algorithm aims to group the Geiger hits in a given SuperNEMO event record
 *  using some simple clustering criteria:
 *  - prompt hits are grouped within a prompt pre-cluster if they lie on the same side of
 *    the source foil,
 *  - delayed hits are grouped within a delayed pre-cluster if they lie on the same side of
 *    the source foil and are in a 10 us time coincidence window.
 *  Only two prompt clusters can exists (maximum: one per side of the source foil).
 *  There is no limitation on the number of delayed clusters.
 *
 *  The TrackerPreClustering::pre_clusterizer object is configured through a
 * TrackerPreClustering::setup_data object.
 *
 *  The input of the algorithm is implemented through the template
 * TrackerPreClustering::input_data<> class which is basically a collection of Geiger hits.
 *
 *  The output of the algorithm is implemented through the template
 * TrackerPreClustering::output_data<> class which stores collections of prompt and delayed
 * clusters. By default the tracking chamber is considered as splitted in two parts.
 *
 */
class pre_clusterizer {
 public:
  static const int OK;
  static const int ERROR;

  /// Default constructor
  pre_clusterizer();

  /// Check the lock flag
  bool is_locked() const;

  /// Configure and initialize the algorithm
  int initialize(const setup_data& setup_);

  /// Reset
  void reset();

  /// Process the list of hits
  /// A 'Hit' class must be provided with the proper interface (see the TrackerPreClustering::gg_hit
  /// mock data model)
  template <typename Hit>
  int process(const input_data<Hit>& input_, output_data<Hit>& output_);

  /// Return the cell size
  double get_cell_size() const;

  /// Set the cell size
  void set_cell_size(double cell_size_);

  /// Set the delayed hit cluster time
  void set_delayed_hit_cluster_time(double);

  /// Return the delayed hit cluster time
  double get_delayed_hit_cluster_time() const;

  /// Check if prompt hits are processed
  bool is_processing_prompt_hits() const;

  /// Set the flag for prompt hits processing
  void set_processing_prompt_hits(bool);

  /// Check if delayed hits are processed
  bool is_processing_delayed_hits() const;

  /// Set the flag for delayed hits processing
  void set_processing_delayed_hits(bool);

  /// Check the flag to split the tracking chamber
  bool is_split_chamber() const;

  /// Set the flag to split the tracking chamber
  void set_split_chamber(bool);

 protected:
  /// Set defualt attribute values
  void _set_defaults();

 private:
  bool _locked_;                          /// Lock flag
  double _cell_size_;                     /// Size of a cell (embedded length units)
  double _delayed_hit_cluster_time_;      /// Delayed hit cluster time window (embedded time units)
  bool _processing_prompt_hits_;          /// Activation of the processing of prompt hits
  bool _processing_delayed_hits_;         /// Activation of the processing of delayed hits
  bool _split_chamber_;  /// Split the chamber in two half-chambers to classify the hits and
                         /// time-clusters
};

/// A functor for handle on tracker hits that perform a comparison by delayed time
template <class Hit>
struct compare_tracker_hit_ptr_by_delayed_time {
 public:
  /// Main comparison method(less than) : require non null handles and non-Nan delayed times
  bool operator()(const Hit* hit_ptr_i_, const Hit* hit_ptr_j_) const {
    if (!hit_ptr_i_->has_delayed_time()) return false;
    if (!hit_ptr_j_->has_delayed_time()) return false;
    return hit_ptr_i_->get_delayed_time() < hit_ptr_j_->get_delayed_time();
  }
};

template <typename Hit>
int pre_clusterizer::process(const input_data<Hit> &input_data_, output_data<Hit> &output_data_) {
  typedef Hit hit_type;
  typedef std::vector<const hit_type *> hit_collection_type;

  // Collection of hit per half-chamber :
  static hit_collection_type prompt_hits[2];
  prompt_hits[0].clear();
  prompt_hits[1].clear();
  static hit_collection_type delayed_hits[2];
  delayed_hits[0].clear();
  delayed_hits[1].clear();

  for (const hit_type *hitref : input_data_.hits) {
    if (hitref->is_sterile()) {
      output_data_.ignored_hits.push_back(hitref);
      continue;
    }
    if (hitref->is_noisy()) {
      output_data_.ignored_hits.push_back(hitref);
      continue;
    }
    if (!hitref->has_xy()) {
      return ERROR;
    }
    if (!hitref->has_geom_id()) {
      return ERROR;
    }

    // int module = hitref->get_module();
    int side = hitref->get_side();
    // int layer  = hitref->get_layer();
    // int row    = hitref->get_row();

    if (hitref->is_prompt()) {
      if (is_processing_prompt_hits()) {
        if (side == 0 || side == 1) {
          int effective_side = side;
          if (!is_split_chamber()) {
            effective_side = 0;
          }
          prompt_hits[effective_side].push_back(hitref);
        } else {
          output_data_.ignored_hits.push_back(hitref);
        }
      } else {
        output_data_.ignored_hits.push_back(hitref);
      }
      continue;
    }

    if (hitref->is_delayed()) {
      if (is_processing_delayed_hits()) {
        if (side == 0 || side == 1) {
          int effective_side = side;
          if (!is_split_chamber()) {
            effective_side = 0;
          }
          delayed_hits[effective_side].push_back(hitref);
        } else {
          output_data_.ignored_hits.push_back(hitref);
        }
      } else {
        output_data_.ignored_hits.push_back(hitref);
      }
      continue;
    }
  }

  if (is_processing_prompt_hits()) {
    unsigned int max_side = 2;
    if (!is_split_chamber()) {
      max_side = 1;
    }
    // For each side of the tracking chamber, we collect one unique candidate time-cluster of prompt
    // hits.
    for (unsigned int side = 0; side < max_side; side++) {
      hit_collection_type *new_prompt_cluster = 0;
      if (prompt_hits[side].size() == 1) {
        output_data_.ignored_hits.push_back(prompt_hits[side][0]);
        continue;
      }
      for (unsigned int i = 0; i < prompt_hits[side].size(); i++) {
        // Traverse the prompt hits in this side :
        const hit_type *hit_ref = prompt_hits[side].at(i);
        if (new_prompt_cluster == 0) {
          hit_collection_type tmp;
          output_data_.prompt_clusters.push_back(tmp);
          new_prompt_cluster = &output_data_.prompt_clusters.back();
        }
        new_prompt_cluster->push_back(hit_ref);
      }
    }
  }

  if (is_processing_delayed_hits()) {
    // For each side of the tracking chamber, we try to collect some candidate time-clusters of
    // delayed hits. The aggregation criterion uses a time-interval of width
    // '_delayed_hit_cluster_time_' (~10usec).
    unsigned int max_side = 2;
    if (!is_split_chamber()) {
      max_side = 1;
    }
    for (unsigned int side = 0; side < max_side; side++) {
      // Sort the collection of delayed hits by delayed time :
      compare_tracker_hit_ptr_by_delayed_time<hit_type> cthpbdt;
      std::sort(delayed_hits[side].begin(), delayed_hits[side].end(), cthpbdt);
      if (delayed_hits[side].size() < 2) {
        if (delayed_hits[side].size() == 1) {
          output_data_.ignored_hits.push_back(delayed_hits[side][0]);
        }
        continue;
      }
      // Pick up the first time-orderer delayed hit on this side of the source foil as
      // the start of a forseen cluster :
      const hit_type *hit_ref_1 = delayed_hits[side].at(0);
      hit_collection_type *new_cluster = 0;
      for (unsigned int i = 1; i < delayed_hits[side].size(); i++) {
        // Traverse the other delayed hits in this side :
        const hit_type *hit_ref_2 = delayed_hits[side].at(i);
        // Check if the delayed time falls in a time window of given width and starting from
        // the time reference given by the 'first' hit :
        if (hit_ref_2->get_delayed_time() >
            (hit_ref_1->get_delayed_time() + _delayed_hit_cluster_time_)) {
          // If the delay between both hits is too large:
          // If no current cluster was initiated, drop the current starting cluster hit :
          if (new_cluster == 0) {
            output_data_.ignored_hits.push_back(hit_ref_1);
          }
          // Make the current hit the new start for a cluster :
          hit_ref_1 = hit_ref_2;
          hit_ref_2 = 0;
          new_cluster = 0;
          continue;
        }
        // If the delay between both hits make them good candidate for time clustering,
        // push them all in the current collecting cluster if any, and create it if it has
        // not been done
        if (new_cluster == 0) {
          // New time cluster :
          hit_collection_type hct;
          output_data_.delayed_clusters.push_back(hct);
          new_cluster = &(output_data_.delayed_clusters.back());
          // Record the first hit of this new time-cluster :
          new_cluster->push_back(hit_ref_1);
        }
        // Record the current hit in the current time-cluster :
        new_cluster->push_back(hit_ref_2);
      }
    }
  }

  return pre_clusterizer::OK;
}

}  // end of namespace TrackerPreClustering

#endif  // FALAISE_TRACKERPRECLUSTERING_PRE_CLUSTERIZER_H

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
