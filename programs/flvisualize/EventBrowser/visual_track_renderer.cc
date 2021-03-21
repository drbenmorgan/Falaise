/* visual_track_renderer.cc
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

#include <EventBrowser/view/browser_tracks.h>
#include <EventBrowser/view/options_manager.h>
#include <EventBrowser/view/style_manager.h>
#include <EventBrowser/view/visual_track_renderer.h>

#include <EventBrowser/detector/detector_manager.h>
#include <EventBrowser/detector/i_root_volume.h>

#include <EventBrowser/io/event_server.h>

#include <EventBrowser/utils/root_utilities.h>

#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>
#include <falaise/snemo/datamodels/line_trajectory_pattern.h>

#include <mctools/utils.h>

#include <TLatex.h>
#include <TObjArray.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>

namespace snemo {

namespace visualization {

namespace view {

void visual_track_renderer::push_mc_tracks() {
  const io::event_record &event = this->get_event();
  const auto &sim_data = event.get<mctools::simulated_data>(io::SD_LABEL);

  // Get hit categories related to visual track
  std::vector<std::string> visual_categories;
  sim_data.get_step_hits_categories(
      visual_categories, mctools::simulated_data::HIT_CATEGORY_TYPE_PREFIX, "__visu.tracks");
  if (visual_categories.empty()) {
    return;
  }

  // Enable/disable track hits
  // Highlight track from primary particles
  const mctools::simulated_data::primary_event_type &pevent = sim_data.get_primary_event();
  const genbb::primary_event::particles_col_type &particles = pevent.get_particles();

  std::set<int> enable_tracks;

  for (const auto &a_primary : particles) {
    if (!a_primary.has_generation_id()) {
      continue;
    }
    const int track_id = a_primary.get_generation_id() + 1;
    if (is_checked(a_primary)) {
      enable_tracks.insert(track_id);
    }
  }

  for (const auto &visual_categorie : visual_categories) {
    for (const auto &a_hit : sim_data.get_step_hits(visual_categorie)) {
      std::string particle_name = a_hit->get_particle_name();

      const bool is_delta_ray_from_alpha =
          a_hit->get_auxiliaries().has_flag(mctools::track_utils::DELTA_RAY_FROM_ALPHA_FLAG);

      if (is_delta_ray_from_alpha) {
        particle_name = "delta_ray_from_alpha";
      } else if (particle_name == "e+") {
        particle_name = "positron";
      } else if (particle_name == "e-") {
        particle_name = "electron";
      } else if (particle_name == "mu+") {
        particle_name = "muon_plus";
      } else if (particle_name == "mu-") {
        particle_name = "muon_minus";
      }

      style_manager &style_mgr = style_manager::get_instance();
      if (!style_mgr.has_particle_properties(particle_name)) {
        if (!style_mgr.add_particle_properties(particle_name)) {
          DT_LOG_INFORMATION(options_manager::get_instance().get_logging_priority(),
                             "Particle '" << particle_name << "' has no properties set !");
        }
      }

      if (!style_mgr.get_particle_visibility(particle_name)) {
        continue;
      }

      size_t line_color = style_mgr.get_particle_color(particle_name);
      size_t line_width = style_mgr.get_mc_line_width();
      size_t line_style = style_mgr.get_mc_line_style();

      const int track_id = a_hit->get_track_id();

      if (is_checked(*a_hit)) {
        enable_tracks.insert(track_id);
      }
      if (enable_tracks.count(track_id) != 0u) {
        continue;
      }

      if (is_highlighted(*a_hit)) {
        line_width += 3;

        TPolyMarker3D *mark1 = base_renderer::make_polymarker(a_hit->get_position_start());
        this->push_graphical_object(mark1);

        mark1->SetMarkerColor(kRed);
        mark1->SetMarkerStyle(kPlus);

        TPolyMarker3D *mark2 = base_renderer::make_polymarker(a_hit->get_position_stop());
        this->push_graphical_object(mark2);

        mark2->SetMarkerColor(kRed);
        mark2->SetMarkerStyle(kCircle);
      }
      geomtools::polyline_type points;
      points.push_back(a_hit->get_position_start());
      points.push_back(a_hit->get_position_stop());

      TPolyLine3D *mc_path = base_renderer::make_polyline(points);
      this->push_graphical_object(mc_path);
      mc_path->SetLineColor(line_color);
      mc_path->SetLineWidth(line_width);
      mc_path->SetLineStyle(line_style);
    }
  }  // end of category list
}

void visual_track_renderer::push_mc_legend() {
  double x = 1.00;
  double y = 0.97;
  const double dx = 0.05;
  const style_manager &style_mgr = style_manager::get_instance();

  for (const auto &particle : style_mgr.get_particles_properties()) {
    const std::string particle_name = particle.first;
    const style_manager::particle_properties particle_properties = particle.second;

    if (!particle_properties._visibility_) {
      continue;
    }

    auto *legend = new TLatex;
    this->push_graphical_object(legend);

    legend->SetNDC();
    legend->SetTextAlign(31);
    legend->SetTextSize(0.04);
    legend->SetTextFont(42);
    legend->SetTextColor(particle_properties._color_);

    const std::string latex = particle_properties._latex_name_;
    legend->SetText(x -= dx, y, latex.c_str());
  }

  // Add a latest legend text for particles not in the previous list
  auto *legend = new TLatex;
  this->push_graphical_object(legend);

  legend->SetNDC();
  legend->SetTextAlign(31);
  legend->SetTextSize(0.04);
  legend->SetTextFont(42);
  legend->SetTextColor(style_mgr.get_particle_color("others"));
  legend->SetText(x -= dx, y, "others");
}

void visual_track_renderer::push_reconstructed_tracks() {
  const io::event_record &event = this->get_event();
  const auto &pt_data = event.get<snemo::datamodel::particle_track_data>(io::PTD_LABEL);

  // Show non-associated calorimeters
  for (const auto &a_calo : pt_data.isolatedCalorimeters()) {
    const geomtools::geom_id &a_calo_gid = a_calo->get_geom_id();
    this->highlight_geom_id(a_calo_gid, style_manager::get_instance().get_calibrated_data_color());
    if (!options_manager::get_instance().get_option_flag(SHOW_CALIBRATED_INFO)) {
      const double energy = a_calo->get_energy() / CLHEP::MeV;
      const double sigma_e = a_calo->get_sigma_energy() / CLHEP::MeV;
      const double time = a_calo->get_time() / CLHEP::ns;
      const double sigma_t = a_calo->get_sigma_time() / CLHEP::ns;

      // Save z position inside text and then parse it
      std::ostringstream text_to_parse;
      text_to_parse.precision(2);
      text_to_parse << std::fixed << "#splitline"
                    << "{E = " << energy << " #pm " << sigma_e << " MeV}"
                    << "{t  = " << time << " #pm " << sigma_t << " ns}";
      this->highlight_geom_id(a_calo_gid, style_manager::get_instance().get_calibrated_data_color(),
                              text_to_parse.str());
    }
  }  // end of calorimeter list

  for (const auto &a_particle : pt_data.particles()) {
    if (!is_checked(*a_particle)) {
      continue;
    }

    // Get color from charge
    size_t color = 0;
    if (a_particle->get_charge() == snemo::datamodel::particle_track::NEUTRAL) {
      color = style_manager::get_instance().get_particle_color("gamma");
    } else if (a_particle->get_charge() == snemo::datamodel::particle_track::NEGATIVE) {
      color = style_manager::get_instance().get_particle_color("electron");
    } else if (a_particle->get_charge() == snemo::datamodel::particle_track::POSITIVE) {
      color = style_manager::get_instance().get_particle_color("positron");
    } else if (a_particle->get_charge() == snemo::datamodel::particle_track::UNDEFINED) {
      color = style_manager::get_instance().get_particle_color("alpha");
    }

    // Show reconstructed vertices
    const auto &vertices = a_particle->get_vertices();

    for (const auto &a_vertex : vertices) {
      const geomtools::vector_3d &a_position = a_vertex->get_position();
      {
        TPolyMarker3D *mark = base_renderer::make_polymarker(a_position);
        this->push_graphical_object(mark);
        mark->SetMarkerColor(color);
        mark->SetMarkerStyle(kPlus);
      }
      if (is_highlighted(*a_vertex)) {
        TPolyMarker3D *mark = base_renderer::make_polymarker(a_position);
        this->push_graphical_object(mark);
        mark->SetMarkerColor(color);
        mark->SetMarkerStyle(kCircle);
      }
    }  // end of vertex list

    // Gamma tracks
    if (a_particle->get_charge() == snemo::datamodel::particle_track::NEUTRAL) {
      geomtools::polyline_type vtces;
      for (const auto &ivtx : vertices) {
        vtces.push_back(ivtx->get_position());
      }
      TPolyLine3D *track = base_renderer::make_polyline(vtces);
      this->push_graphical_object(track);
      track->SetLineColor(color);
      if (a_particle->get_auxiliaries().has_flag("__gamma_from_annihilation")) {
        track->SetLineStyle(kDashDotted);
      } else {
        track->SetLineStyle(kDashed);
      }
    }

    // Show associated calorimeters
    for (const auto &a_calo : a_particle->get_associated_calorimeter_hits()) {
      const geomtools::geom_id &a_calo_gid = a_calo->get_geom_id();
      this->highlight_geom_id(a_calo_gid, color);
      const double energy = a_calo->get_energy();
      const double sigma_e = a_calo->get_sigma_energy();
      const double time = a_calo->get_time();
      const double sigma_t = a_calo->get_sigma_time();

      // Save z position inside text and then parse it
      std::ostringstream oss;
      oss.precision(2);
      oss << std::fixed << "#splitline{E = ";
      utils::root_utilities::get_prettified_energy(oss, energy, sigma_e, true);
      oss << "}{t  = ";
      utils::root_utilities::get_prettified_time(oss, time, sigma_t, true);
      oss << "}";
      this->highlight_geom_id(a_calo_gid, color, oss.str());
    }  // end of calorimeter list

    // Show attached fit, if there is one
    if (a_particle->has_trajectory()) {
      const snemo::datamodel::tracker_trajectory &a_trajectory = a_particle->get_trajectory();
      const snemo::datamodel::base_trajectory_pattern &a_pattern = a_trajectory.get_pattern();
      const auto &iw3dr =
          dynamic_cast<const geomtools::i_wires_3d_rendering &>(a_pattern.get_shape());
      TPolyLine3D *track = base_renderer::make_track(iw3dr);
      this->push_graphical_object(track);
      track->SetLineColor(color);
    }
  }
}
}  // end of namespace view

}  // end of namespace visualization

}  // end of namespace snemo

// end of visual_track_renderer.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
