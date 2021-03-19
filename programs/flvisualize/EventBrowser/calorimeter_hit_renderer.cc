/* calorimeter_hit_renderer.cc
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
#include <EventBrowser/view/calorimeter_hit_renderer.h>
#include <EventBrowser/view/options_manager.h>
#include <EventBrowser/view/style_manager.h>

#include <EventBrowser/detector/detector_manager.h>
#include <EventBrowser/detector/i_root_volume.h>

#include <EventBrowser/io/event_server.h>

#include <EventBrowser/utils/root_utilities.h>

#include <geomtools/id_mgr.h>

#include <TColor.h>
#include <TMarker3DBox.h>
#include <TObjArray.h>
#include <TPolyMarker3D.h>

namespace snemo {

namespace visualization {

namespace view {

void calorimeter_hit_renderer::push_simulated_hits(const std::string& hit_category_) {
  const io::event_record& event = _server->get_event();
  const auto& sim_data = event.get<mctools::simulated_data>(io::SD_LABEL);

  if (!sim_data.has_step_hits(hit_category_)) {
    return;
  }

  for (const auto& a_hit : sim_data.get_step_hits(hit_category_)) {
    const geomtools::vector_3d& pstart = a_hit->get_position_start();
    const geomtools::vector_3d& pstop = a_hit->get_position_stop();

    const double dx = std::abs(0.5 * (pstart.x() - pstop.x()));
    const double dy = std::abs(0.5 * (pstart.y() - pstop.y()));
    const double dz = std::abs(0.5 * (pstart.z() - pstop.z()));

    const geomtools::vector_3d pos = 0.5 * (pstart + pstop);

    auto* step_3d = new TMarker3DBox;
    _objects->Add(step_3d);
    step_3d->SetPosition(pos.x(), pos.y(), pos.z());
    step_3d->SetSize(dx, dy, dz);
    step_3d->SetLineColor(kRed);

    // Retrieve line width from properties if 'hit' is highlighted:
    size_t line_width = style_manager::get_instance().get_mc_line_width();
    if (a_hit->get_auxiliaries().has_flag(browser_tracks::HIGHLIGHT_FLAG)) {
      line_width = 3;
      auto* mark1 = new TPolyMarker3D;
      _objects->Add(mark1);
      mark1->SetMarkerColor(kRed);
      mark1->SetMarkerStyle(kCircle);
      mark1->SetPoint(0, pstart.x(), pstart.y(), pstart.z());
      auto* mark2 = new TPolyMarker3D;
      _objects->Add(mark2);
      mark2->SetMarkerColor(kRed);
      mark2->SetMarkerStyle(kCircle);
      mark2->SetPoint(0, pstop.x(), pstop.y(), pstop.z());
    }
    step_3d->SetLineWidth(line_width);

    this->highlight_geom_id(a_hit->get_geom_id(), kRed);
  }  // end of step collection
}

void calorimeter_hit_renderer::push_calibrated_hits() {
  const io::event_record& event = _server->get_event();
  const auto& calib_data = event.get<snemo::datamodel::calibrated_data>(io::CD_LABEL);

  for (const auto& a_hit : calib_data.calorimeter_hits()) {
    this->highlight_geom_id(a_hit->get_geom_id(),
                            style_manager::get_instance().get_calibrated_data_color());

    if (options_manager::get_instance().get_option_flag(SHOW_CALIBRATED_INFO)) {
      const double energy = a_hit->get_energy();
      const double sigma_e = a_hit->get_sigma_energy();
      const double time = a_hit->get_time();
      const double sigma_t = a_hit->get_sigma_time();

      // Save z position inside text and then parse it
      std::ostringstream oss;
      oss.precision(2);
      oss << std::fixed << "#splitline{E = ";
      utils::root_utilities::get_prettified_energy(oss, energy, sigma_e, true);
      oss << "}{t  = ";
      utils::root_utilities::get_prettified_time(oss, time, sigma_t, true);
      oss << "}";
      this->highlight_geom_id(a_hit->get_geom_id(),
                              style_manager::get_instance().get_calibrated_data_color(), oss.str());
    }
  }
}

}  // end of namespace view

}  // end of namespace visualization

}  // end of namespace snemo

// end of calorimeter_hit_renderer.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
