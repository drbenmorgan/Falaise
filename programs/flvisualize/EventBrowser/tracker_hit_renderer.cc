/* tracker_hit_renderer.cc
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

// Third party:
// - ROOT:
#include <TColor.h>
#include <TMarker3DBox.h>
#include <TMath.h>
#include <TObjArray.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>
#include <TRotation.h>

// - Bayeux/geomtools:
#include <bayeux/geomtools/manager.h>

// - Falaise:
#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>
#include <falaise/snemo/datamodels/line_trajectory_pattern.h>
#include <falaise/snemo/processing/geiger_regime.h>

// This project:
#include <EventBrowser/io/data_model.h>
#include <EventBrowser/view/browser_tracks.h>
#include <EventBrowser/view/options_manager.h>
#include <EventBrowser/view/style_manager.h>
#include <EventBrowser/view/tracker_hit_renderer.h>

#include <EventBrowser/detector/detector_manager.h>
#include <EventBrowser/detector/i_root_volume.h>

#include <EventBrowser/io/event_server.h>

#include <EventBrowser/utils/root_utilities.h>

namespace snemo {

namespace visualization {

namespace view {

void tracker_hit_renderer::push_simulated_hits(const std::string &hit_category_) {
  const io::event_record &event = this->get_event();
  const auto &sim_data = event.get<mctools::simulated_data>(io::SD_LABEL);

  if (!sim_data.has_step_hits(hit_category_)) {
    return;
  }

  const mctools::simulated_data::hit_handle_collection_type &hit_collection =
      sim_data.get_step_hits(hit_category_);

  if (hit_collection.empty()) {
    return;
  }

  // time gradient color
  double hit_start_time = hit_collection.front().get().get_time_start();
  double hit_stop_time = hit_collection.front().get().get_time_start();

  const bool geiger_with_gradient =
      options_manager::get_instance().get_option_flag(SHOW_GG_TIME_GRADIENT);
  if (geiger_with_gradient) {
    for (const auto &it_hit : hit_collection) {
      hit_start_time = std::min(it_hit->get_time_start(), hit_start_time);
      hit_stop_time = std::max(it_hit->get_time_start(), hit_stop_time);
    }
  }

  for (const auto &a_step : hit_collection) {
    const auto &startPos = a_step->get_position_start();
    const auto &stopPos = a_step->get_position_stop();
    const double startTime = a_step->get_time_start();

    // draw the Geiger avalanche path:
    auto *gg_path = new TPolyLine3D;
    this->push_graphical_object(gg_path);
    gg_path->SetPoint(0, startPos.x(), startPos.y(), startPos.z());
    gg_path->SetPoint(1, stopPos.x(), stopPos.y(), stopPos.z());

    const auto time_percent =
        (size_t)((TColor::GetNumberOfColors() - 1) * (startTime - hit_start_time) /
                 (hit_stop_time - hit_start_time));
    const size_t color = geiger_with_gradient ? TColor::GetColorPalette(time_percent) : kSpring;
    gg_path->SetLineColor(color);

    // Store this value into cluster properties:
    auto *mutable_hit = const_cast<mctools::base_step_hit *>(&(*a_step));

    set_color(*mutable_hit, utils::root_utilities::get_hex_color(color));

    // Retrieve line width from properties if 'hit' is highlighted:
    size_t line_width = style_manager::get_instance().get_mc_line_width();
    if (is_highlighted(*mutable_hit)) {
      line_width = 3;
    }
    gg_path->SetLineWidth(line_width);

    // draw circle tangential to the track:
    if (options_manager::get_instance().get_option_flag(SHOW_GG_CIRCLE)) {
      const size_t n_point = 100;
      TRotation dr;
      int cell_axis = 'z';

      const detector::detector_manager &detector_mgr = detector::detector_manager::get_instance();
      const std::string &setup_label = detector_mgr.get_setup_name();
      if (setup_label == "snemo::tracker_commissioning") {
        cell_axis = 'x';
      }

      if (cell_axis == 'z') {
        dr.RotateZ(2 * TMath::Pi() / (double)n_point);
      } else if (cell_axis == 'x') {
        dr.RotateX(2 * TMath::Pi() / (double)n_point);
      } else {
        DT_THROW_IF(true, std::logic_error, "Unsupported cell axis !");
      }

      geomtools::polyline_type points;
      if (cell_axis == 'z') {
        TVector3 current_pos((startPos - stopPos).x(), (startPos - stopPos).y(), stopPos.z());
        for (size_t i_point = 0; i_point <= n_point; ++i_point) {
          current_pos *= dr;
          points.push_back(geomtools::vector_3d(current_pos.x() + stopPos.x(),
                                                current_pos.y() + stopPos.y(), current_pos.z()));
        }
      } else if (cell_axis == 'x') {
        TVector3 current_pos(stopPos.x(), (startPos - stopPos).y(), (startPos - stopPos).z());

        for (size_t i_point = 0; i_point <= n_point; ++i_point) {
          current_pos *= dr;
          points.push_back(geomtools::vector_3d(current_pos.x(), current_pos.y() + stopPos.y(),
                                                current_pos.z() + stopPos.z()));
        }
      }

      TPolyLine3D *gg_drift = base_renderer::make_polyline(points);
      this->push_graphical_object(gg_drift);
      gg_drift->SetLineColor(color);
      gg_drift->SetLineWidth(line_width);
    }  // end of "show geiger drift circle" condition
  }    // end of step collection
}

void tracker_hit_renderer::push_calibrated_hits() {
  const io::event_record &event = this->get_event();
  const auto &calib_data = event.get<snemo::datamodel::calibrated_data>(io::CD_LABEL);

  const snemo::datamodel::TrackerHitHdlCollection &ct_collection = calib_data.tracker_hits();

  if (ct_collection.empty()) {
    return;
  }

  for (const auto &a_hit : ct_collection) {
    tracker_hit_renderer::_make_calibrated_geiger_hit(*a_hit, false);
  }
}

void tracker_hit_renderer::push_clustered_hits() {
  const io::event_record &event = this->get_event();
  const auto &tracker_clustered_data =
      event.get<snemo::datamodel::tracker_clustering_data>(io::TCD_LABEL);

  for (const auto &a_solution : tracker_clustered_data.solutions()) {
    // Check solution properties:
    if (!is_checked(*a_solution)) {
      continue;
    }

    // Get clusters stored in the current tracker solution:
    size_t clusterIndex = 0;
    for (const auto &a_cluster : a_solution->get_clusters()) {
      // Check cluster properties:
      if (!is_checked(*a_cluster)) {
        continue;
      }

      // Determine cluster color
      const size_t cluster_color = style_manager::get_instance().get_color(clusterIndex);
      ++clusterIndex;

      // Store this value into cluster properties:
      auto *mutable_cluster = const_cast<snemo::datamodel::tracker_cluster *>(&(*a_cluster));
      set_color(*mutable_cluster, utils::root_utilities::get_hex_color(cluster_color));

      // Get tracker hits stored in the current tracker cluster:
      // Make a gradient color starting from color_solution:
      for (const auto &a_gg_hit : a_cluster->hits()) {
        // Retrieve a mutable reference to calibrated_tracker_hit:
        auto *mutable_hit = const_cast<snemo::datamodel::calibrated_tracker_hit *>(&(*a_gg_hit));

        // Store current color to be used by calibrated_tracker_hit renderer:
        set_color(*mutable_hit, utils::root_utilities::get_hex_color(cluster_color));

        const options_manager &options_mgr = options_manager::get_instance();
        if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_BOX)) {
          const double x = a_gg_hit->get_x();
          const double y = a_gg_hit->get_y();
          const double z = a_gg_hit->get_z();
          const double dz = a_gg_hit->get_sigma_z();
          const double r = 22.0 / CLHEP::mm;

          auto *hit_3d = new TMarker3DBox;
          this->push_graphical_object(hit_3d);
          hit_3d->SetPosition(x, y, z);
          hit_3d->SetSize(r, r, dz);
          hit_3d->SetLineColor(cluster_color);
          // Retrieve line width from properties if 'hit' is highlighted:
          size_t line_width = 1;
          if (is_highlighted(*mutable_hit)) {
            line_width = 3;
          }
          hit_3d->SetLineWidth(line_width);
        } else if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_CIRCLE)) {
          tracker_hit_renderer::_make_calibrated_geiger_hit(*a_gg_hit, true);
        }
      }  // end of gg hits
    }    // end of cluster loop
  }      // end of solution loop
}

void tracker_hit_renderer::push_fitted_tracks() {
  const io::event_record &event = this->get_event();
  const auto &tracker_trajectory_data =
      event.get<snemo::datamodel::tracker_trajectory_data>(io::TTD_LABEL);

  for (const auto &a_solution : tracker_trajectory_data.get_solutions()) {
// DONT couple data model to view!
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // Check solution properties:
    if (!a_solution->get_auxiliaries().has_flag(browser_tracks::CHECKED_FLAG)) {
      continue;
    }
#pragma GCC diagnostic pop

    // Get trajectories stored in the current tracker trajectory solution:
    for (const auto &a_trajectory : a_solution->get_trajectories()) {
      // Get trajectory properties:
      const datatools::properties &traj_properties = a_trajectory->get_auxiliaries();

      if (!is_checked(*a_trajectory)) {
        continue;
      } else {
        if (!traj_properties.has_flag("default")) {
          continue;
        }
      }

      // Retrieve trajectory visual rendering:
      const snemo::datamodel::base_trajectory_pattern &a_pattern = a_trajectory->get_pattern();
      const auto &iw3dr =
          dynamic_cast<const geomtools::i_wires_3d_rendering &>(a_pattern.get_shape());
      TPolyLine3D *track = base_renderer::make_track(iw3dr);
      this->push_graphical_object(track);

      // Determine trajectory color by getting cluster color:
      int trajectory_color = 0;
      if (a_trajectory->has_cluster()) {
        const snemo::datamodel::tracker_cluster &a_cluster = a_trajectory->get_cluster();
        const std::string hex_str = get_color(a_cluster);
        trajectory_color = TColor::GetColor(hex_str.c_str());
      }
      track->SetLineColor(trajectory_color);

      // Retrieve line width from properties if 'track' is highlighted:
      size_t line_width = 1;
      if (is_highlighted(*a_trajectory)) {
        line_width = 3;
      }
      track->SetLineWidth(line_width);

      // For delayed tracks such as alpha track, show the recalibrated
      // tracker hit
      if (traj_properties.has_key("t0") &&
          options_manager::get_instance().get_option_flag(SHOW_RECALIBRATED_TRACKER_HITS)) {
        const double t0 = traj_properties.fetch_real("t0");
        if (a_trajectory->has_cluster()) {
          const snemo::datamodel::tracker_cluster &a_cluster = a_trajectory->get_cluster();

          snemo::processing::geiger_regime a_gg_regime;

          // Copy and recalibrate tracker hits stored in the current tracker cluster:
          for (auto a_gg_hit_copy : a_cluster.hits()) {
            // Recalibrate drift radius given fitted t0
            double new_r = datatools::invalid_real();
            double new_sigma_r = datatools::invalid_real();
            double new_anode_time = datatools::invalid_real();
            if (a_gg_hit_copy->is_delayed()) {
              new_anode_time = a_gg_hit_copy->get_delayed_time() - t0;
            } else {
              new_anode_time = a_gg_hit_copy->get_anode_time() - t0;
            }
            if (new_anode_time < 0.0 * CLHEP::ns) {
              new_anode_time = 0.0 * CLHEP::ns;
            }
            a_gg_regime.calibrateRadiusFromTime(new_anode_time, new_r, new_sigma_r);
            a_gg_hit_copy->set_r(new_r);
            a_gg_hit_copy->set_sigma_r(new_sigma_r);
            a_gg_hit_copy->set_delayed(false);
            tracker_hit_renderer::_make_calibrated_geiger_hit(*a_gg_hit_copy, true);
          }
        }
      }

    }  // end of trajectory loop
  }    // end of solution loop
}

void tracker_hit_renderer::_make_calibrated_geiger_hit(
    const snemo::datamodel::calibrated_tracker_hit &hit_, const bool show_cluster) {
  // Compute the position of the anode impact in the drift cell coordinates reference frame:
  const detector::detector_manager &detector_mgr = detector::detector_manager::get_instance();

  geomtools::vector_3d cell_module_pos(hit_.get_x(), hit_.get_y(), hit_.get_z());
  geomtools::vector_3d cell_world_pos;
  detector_mgr.compute_world_coordinates(cell_module_pos, cell_world_pos);

  // Get (x, y) position of triggered cell
  const double x = cell_world_pos.x();
  const double y = cell_world_pos.y();

  // Add error in z coordinate
  const double z = cell_world_pos.z();
  const double sigma_z = hit_.get_sigma_z();

  // Retrieve line width from properties if 'hit' is highlighted:
  size_t line_width = style_manager::get_instance().get_mc_line_width();
  if (is_highlighted(hit_)) {
    line_width = 3;
  }

  int color = style_manager::get_instance().get_calibrated_data_color();
  const std::string hex_str = get_color(hit_);

  if (show_cluster && !hex_str.empty()) {
    const options_manager &options_mgr = options_manager::get_instance();
    if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_HITS) &&
        options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_CIRCLE)) {
      color = TColor::GetColor(hex_str.c_str());
    }
  }
  auto *gg_dz = new TPolyLine3D;
  this->push_graphical_object(gg_dz);
  gg_dz->SetLineColor(color);
  gg_dz->SetLineWidth(line_width);

  int cell_axis = 'z';
  const std::string &setup_label = detector_mgr.get_setup_name();
  if (setup_label == "snemo::tracker_commissioning") {
    cell_axis = 'x';
  }
  DT_THROW_IF(cell_axis != 'z' && cell_axis != 'x', std::logic_error, "Unsupported cell axis !");

  if (cell_axis == 'z') {
    gg_dz->SetPoint(0, x, y, z - sigma_z);
    gg_dz->SetPoint(1, x, y, z + sigma_z);
  } else if (cell_axis == 'x') {
    gg_dz->SetPoint(0, x - sigma_z, y, z);
    gg_dz->SetPoint(1, x + sigma_z, y, z);
  }

  if (hit_.is_delayed()) {
    const double r = 22.0 / CLHEP::mm;  // hit_.get_r();
    geomtools::polyline_type points;
    points.push_back(geomtools::vector_3d(x + r, y + r, z));
    points.push_back(geomtools::vector_3d(x + r, y - r, z));
    points.push_back(geomtools::vector_3d(x - r, y - r, z));
    points.push_back(geomtools::vector_3d(x - r, y + r, z));
    points.push_back(geomtools::vector_3d(x + r, y + r, z));

    TPolyLine3D *gg_drift_square = base_renderer::make_polyline(points);
    this->push_graphical_object(gg_drift_square);
    gg_drift_square->SetLineColor(color);
    gg_drift_square->SetLineWidth(line_width);

  } else {
    // add calibrated drift value:  r-dr; r+dr
    const size_t n_point = 100;
    TRotation dr;
    if (cell_axis == 'z') {
      dr.RotateZ(2 * TMath::Pi() / (double)n_point);
    } else if (cell_axis == 'x') {
      dr.RotateX(2 * TMath::Pi() / (double)n_point);
    }

    // get calibrated info
    const double r = hit_.get_r();
    const double sigma_r = hit_.get_sigma_r();

    geomtools::polyline_type rmins;
    geomtools::polyline_type rmaxs;
    if (cell_axis == 'z') {
      TVector3 r_min(r - sigma_r, 0, z);
      TVector3 r_max(r + sigma_r, 0, z);

      for (size_t i_point = 0; i_point <= n_point; ++i_point) {
        r_min *= dr;
        r_max *= dr;
        rmins.push_back(geomtools::vector_3d(r_min.x() + x, r_min.y() + y, r_min.z()));
        rmaxs.push_back(geomtools::vector_3d(r_max.x() + x, r_max.y() + y, r_max.z()));
      }
    } else if (cell_axis == 'x') {
      TVector3 r_min(x, r - sigma_r, 0);
      TVector3 r_max(x, r + sigma_r, 0);

      for (size_t i_point = 0; i_point <= n_point; ++i_point) {
        r_min *= dr;
        r_max *= dr;
        rmins.push_back(geomtools::vector_3d(r_min.x(), r_min.y() + y, r_min.z() + z));
        rmaxs.push_back(geomtools::vector_3d(r_max.x(), r_max.y() + y, r_max.z() + z));
      }
    }
    TPolyLine3D *gg_drift_min = base_renderer::make_polyline(rmins);
    this->push_graphical_object(gg_drift_min);
    gg_drift_min->SetLineColor(color);
    gg_drift_min->SetLineWidth(line_width);

    TPolyLine3D *gg_drift_max = base_renderer::make_polyline(rmaxs);
    this->push_graphical_object(gg_drift_max);
    gg_drift_max->SetLineColor(color);
    gg_drift_max->SetLineWidth(line_width);
  }
}

}  // end of namespace view

}  // end of namespace visualization

}  // end of namespace snemo

// end of tracker_hit_renderer.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
