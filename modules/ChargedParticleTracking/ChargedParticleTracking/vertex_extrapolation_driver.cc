/// \file falaise/snemo/reconstruction/vertex_extrapolation_driver.cc

// Ourselves:
#include <ChargedParticleTracking/vertex_extrapolation_driver.h>

// Standard library:
#include <sstream>

// Third party:
// - Bayeux/datatools:
#include <datatools/properties.h>
// - Bayeux/geomtools:
#include <geomtools/manager.h>

// This project (Falaise):
#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>
#include <falaise/snemo/datamodels/line_trajectory_pattern.h>
#include <falaise/snemo/datamodels/tracker_trajectory.h>

#include <falaise/snemo/geometry/calo_locator.h>
#include <falaise/snemo/geometry/gg_locator.h>
#include <falaise/snemo/geometry/gveto_locator.h>
#include <falaise/snemo/geometry/locator_plugin.h>
#include <falaise/snemo/geometry/xcalo_locator.h>

namespace snemo {

namespace reconstruction {

const std::string &vertex_extrapolation_driver::get_id() {
  static const std::string s("VED");
  return s;
}

void vertex_extrapolation_driver::set_initialized(const bool initialized_) {
  _initialized_ = initialized_;
}

bool vertex_extrapolation_driver::is_initialized() const { return _initialized_; }

void vertex_extrapolation_driver::set_logging_priority(
    const datatools::logger::priority priority_) {
  _logging_priority_ = priority_;
}

datatools::logger::priority vertex_extrapolation_driver::get_logging_priority() const {
  return _logging_priority_;
}

void vertex_extrapolation_driver::set_geometry_manager(const geomtools::manager &gmgr_) {
  DT_THROW_IF(is_initialized(), std::logic_error, "Driver is already initialized !");
  _geometry_manager_ = &gmgr_;
}

const geomtools::manager &vertex_extrapolation_driver::get_geometry_manager() const {
  DT_THROW_IF(!has_geometry_manager(), std::logic_error, "No geometry manager is setup !");
  return *_geometry_manager_;
}

bool vertex_extrapolation_driver::has_geometry_manager() const { return _geometry_manager_ != nullptr; }


/// Initialize the driver through configuration properties
void vertex_extrapolation_driver::initialize(const datatools::properties &setup_) {
  DT_THROW_IF(is_initialized(), std::logic_error, "Driver is already initialized !");

  DT_THROW_IF(!has_geometry_manager(), std::logic_error, "Missing geometry manager !");
  DT_THROW_IF(!get_geometry_manager().is_initialized(), std::logic_error,
              "Geometry manager is not initialized !");

  // Logging priority
  datatools::logger::priority lp = datatools::logger::extract_logging_configuration(setup_);
  DT_THROW_IF(lp == datatools::logger::PRIO_UNDEFINED, std::logic_error,
              "Invalid logging priority level for geometry manager !");
  set_logging_priority(lp);

  // Get geometry locator plugin
  const geomtools::manager &geo_mgr = get_geometry_manager();
  std::string locator_plugin_name;
  if (setup_.has_key("locator_plugin_name")) {
    locator_plugin_name = setup_.fetch_string("locator_plugin_name");
  } else {
    // If no locator plugin name is set, then search for the first one
    for (const auto& ip : geo_mgr.get_plugins()) {
      const std::string &plugin_name = ip.first;
      if (geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(plugin_name)) {
        locator_plugin_name = plugin_name;
        break;
      }
    }
  }
  // Access to a given plugin by name and type :
  DT_THROW_IF(!geo_mgr.has_plugin(locator_plugin_name) ||
                  !geo_mgr.is_plugin_a<snemo::geometry::locator_plugin>(locator_plugin_name),
              std::logic_error, "Found no locator plugin named '" << locator_plugin_name << "'");
  _locator_plugin_ = &geo_mgr.get_plugin<snemo::geometry::locator_plugin>(locator_plugin_name);

  set_initialized(true);
}

/// Reset the driver
void vertex_extrapolation_driver::reset() {
  _initialized_ = false;
  _logging_priority_ = datatools::logger::PRIO_WARNING;
  _geometry_manager_ = nullptr;
  _locator_plugin_ = nullptr;
}

void vertex_extrapolation_driver::process(const snemo::datamodel::tracker_trajectory &trajectory_,
                                          snemo::datamodel::particle_track &particle_) {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Driver is not initialized !");
  this->_measure_vertices_(trajectory_, particle_.get_vertices());
}

void vertex_extrapolation_driver::_measure_vertices_(
    const snemo::datamodel::tracker_trajectory &trajectory_,
    snemo::datamodel::particle_track::vertex_collection_type &vertices_) {
  // Extract the side from the geom_id of the tracker_trajectory object:
  if (!trajectory_.has_geom_id()) {
    DT_LOG_ERROR(get_logging_priority(), "Tracker trajectory has no geom_id! Abort!");
    return;
  }
  const geomtools::geom_id &gid = trajectory_.get_geom_id();
  const geomtools::id_mgr &id_mgr = get_geometry_manager().get_id_mgr();
  if (!id_mgr.has(gid, "module") || !id_mgr.has(gid, "side")) {
    DT_LOG_ERROR(get_logging_priority(),
                 "Trajectory geom_id " << gid << " has no 'module' or 'side' address!");
    return;
  }
  const int side = id_mgr.get(gid, "side");

  // Set the calorimeter locators :
  const snemo::geometry::calo_locator &calo_locator = _locator_plugin_->get_calo_locator();
  const snemo::geometry::xcalo_locator &xcalo_locator = _locator_plugin_->get_xcalo_locator();
  const snemo::geometry::gveto_locator &gveto_locator = _locator_plugin_->get_gveto_locator();
  // TODO: Add source strip locator...

  const double xcalo_bd[2] = {calo_locator.get_wall_window_x(snemo::geometry::utils::SIDE_BACK),
                              calo_locator.get_wall_window_x(snemo::geometry::utils::SIDE_FRONT)};
  const double ycalo_bd[2] = {
      xcalo_locator.get_wall_window_y(side, snemo::geometry::xcalo_locator::WALL_LEFT),
      xcalo_locator.get_wall_window_y(side, snemo::geometry::xcalo_locator::WALL_RIGHT)};
  const double zcalo_bd[2] = {
      gveto_locator.get_wall_window_z(side, snemo::geometry::gveto_locator::WALL_BOTTOM),
      gveto_locator.get_wall_window_z(side, snemo::geometry::gveto_locator::WALL_TOP)};

  // Check Geiger cell location wrt to vertex extrapolation
  this->_check_vertices_(trajectory_);

  // Look first if trajectory pattern is an helix or not:
  const snemo::datamodel::base_trajectory_pattern &a_track_pattern = trajectory_.get_pattern();
  const std::string &a_pattern_id = a_track_pattern.get_pattern_id();

  // Extrapolated vertices:
  using vertex_dict_type = std::vector<std::pair<std::string, geomtools::vector_3d> >;
  vertex_dict_type vertices;

  // Add a property into 'blur_spot' auxiliaries to refer to the hit calo:
  std::string calo_category_flag;

  // ----- Start of line pattern handling
  if (a_pattern_id == snemo::datamodel::line_trajectory_pattern::pattern_id()) {
    const auto& ltp = dynamic_cast<const snemo::datamodel::line_trajectory_pattern &>(a_track_pattern);
    const geomtools::line_3d &a_line = ltp.get_segment();
    const geomtools::vector_3d &first = a_line.get_first();
    const geomtools::vector_3d &last = a_line.get_last();
    const geomtools::vector_3d direction = first - last;

    // Calculate intersection on each geometric object
    typedef std::map<geomtools::vector_3d, std::string> vertex_list_type;
    vertex_list_type vtxlist;
    // Source foil:
    {
      const double x = 0.0 * CLHEP::mm;
      const double y = direction.y() / direction.x() * (x - first.x()) + first.y();
      const double z = direction.z() / direction.y() * (y - first.y()) + first.z();

      // Extrapolated vertex:
      const geomtools::vector_3d a_vertex(x, y, z);
      vtxlist.insert(std::make_pair(
          a_vertex, snemo::datamodel::particle_track::vertex_on_source_foil_label()));
    }

    // Calorimeter walls:
    {
      for (size_t iside = 0; iside < snemo::geometry::utils::NSIDES; ++iside) {
        const double x = xcalo_bd[iside];
        const double y = direction.y() / direction.x() * (x - first.x()) + first.y();
        const double z = direction.z() / direction.y() * (y - first.y()) + first.z();

        // Extrapolated vertex
        const geomtools::vector_3d a_vertex(x, y, z);
        vtxlist.insert(std::make_pair(
            a_vertex, snemo::datamodel::particle_track::vertex_on_main_calorimeter_label()));
      }
    }

    // Calorimeter on xwalls
    {
      for (size_t iwall = 0; iwall < snemo::geometry::xcalo_locator::NWALLS_PER_SIDE; ++iwall) {
        const double y = ycalo_bd[iwall];
        const double z = direction.z() / direction.y() * (y - first.y()) + first.z();
        const double x = direction.x() / direction.y() * (y - first.y()) + first.x();

        // Extrapolate vertex
        const geomtools::vector_3d a_vertex(x, y, z);
        vtxlist.insert(std::make_pair(
            a_vertex, snemo::datamodel::particle_track::vertex_on_x_calorimeter_label()));
      }
    }

    // Calorimeter on gveto
    {
      for (size_t iwall = 0; iwall < snemo::geometry::gveto_locator::NWALLS_PER_SIDE; ++iwall) {
        const double z = zcalo_bd[iwall];
        const double y = direction.y() / direction.z() * (z - first.z()) + first.y();
        const double x = direction.x() / direction.y() * (y - first.y()) + first.x();

        // Extrapolate vertex
        const geomtools::vector_3d a_vertex(x, y, z);
        vtxlist.insert(std::make_pair(
            a_vertex, snemo::datamodel::particle_track::vertex_on_gamma_veto_label()));
      }
    }

    // This *looks* like it finds the two vertices closest to the end points of
    // the line pattern
    std::pair<double, double> min_distances;
    datatools::infinity(min_distances.first);
    datatools::infinity(min_distances.second);
    auto jt1 = vtxlist.begin();
    auto jt2 = vtxlist.begin();
    for (auto it = vtxlist.begin(); it != vtxlist.end(); ++it) {
      const double l1 = (first - it->first).mag();
      const double l2 = (last - it->first).mag();

      if (l1 < l2) {
        if (l1 > min_distances.first) {
          continue;
        }
        jt1 = it;
        min_distances.first = l1;
      } else {
        if (l2 > min_distances.second) {
          continue;
        }
        jt2 = it;
        min_distances.second = l2;
      }
    }

    // Create a mutable line object to set the new position
    geomtools::line_3d *a_mutable_line = const_cast<geomtools::line_3d *>(&a_line);
    if (_use_vertices_[jt1->second]) {
      a_mutable_line->set_first(jt1->first);
      vertices.push_back(std::make_pair(jt1->second, jt1->first));
    } else {
      vertices.push_back(std::make_pair(snemo::datamodel::particle_track::vertex_on_wire_label(),
                                        a_line.get_first()));
    }
    if (_use_vertices_[jt2->second]) {
      a_mutable_line->set_last(jt2->first);
      vertices.push_back(std::make_pair(jt2->second, jt2->first));
    } else {
      vertices.push_back(std::make_pair(snemo::datamodel::particle_track::vertex_on_wire_label(),
                                        a_line.get_last()));
    }
  }  // ----- end of line pattern handling
  // ---- start of helix pattern handling
  else if (a_pattern_id == snemo::datamodel::helix_trajectory_pattern::pattern_id()) {
    const auto& htp = dynamic_cast<const snemo::datamodel::helix_trajectory_pattern &>(a_track_pattern);
    const geomtools::helix_3d &a_helix = htp.get_helix();

    // Extract helix parameters
    const geomtools::vector_3d &hcenter = a_helix.get_center();
    const double hradius = a_helix.get_radius();

    // Store all the computed t parameter values
    std::map<double, std::string> tparams;

    // Source foil
    {
      const double xfoil = 0.0 * CLHEP::mm;
      const double xcenter = hcenter.x();
      const double cangle = (xfoil - xcenter) / hradius;

      if (std::fabs(cangle) < 1.0) {
        const double angle = std::acos(cangle);
        tparams.insert(
            std::make_pair(geomtools::helix_3d::angle_to_t(+angle),
                           snemo::datamodel::particle_track::vertex_on_source_foil_label()));
        tparams.insert(
            std::make_pair(geomtools::helix_3d::angle_to_t(-angle),
                           snemo::datamodel::particle_track::vertex_on_source_foil_label()));
      }
    }

    // Calorimeter walls
    {
      for (size_t iside = 0; iside < snemo::geometry::utils::NSIDES; ++iside) {
        const double xcenter = hcenter.x();
        const double cangle = (xcalo_bd[iside] - xcenter) / hradius;

        if (std::fabs(cangle) < 1.0) {
          const double angle = std::acos(cangle);
          tparams.insert(
              std::make_pair(geomtools::helix_3d::angle_to_t(+angle),
                             snemo::datamodel::particle_track::vertex_on_main_calorimeter_label()));
          tparams.insert(
              std::make_pair(geomtools::helix_3d::angle_to_t(-angle),
                             snemo::datamodel::particle_track::vertex_on_main_calorimeter_label()));
        }
      }
    }

    // X-walls
    {
      for (size_t iwall = 0; iwall < snemo::geometry::xcalo_locator::NWALLS_PER_SIDE; ++iwall) {
        const double ycenter = hcenter.y();
        const double cangle = (ycalo_bd[iwall] - ycenter) / hradius;

        if (std::fabs(cangle) < 1.0) {
          double angle = std::asin(cangle);
          tparams.insert(
              std::make_pair(geomtools::helix_3d::angle_to_t(angle),
                             snemo::datamodel::particle_track::vertex_on_x_calorimeter_label()));
          const double mean_angle = (a_helix.get_angle1() + a_helix.get_angle2()) / 2.0;
          angle = (mean_angle < 0.0) ? (-M_PI - angle) : (+M_PI - angle);
          tparams.insert(
              std::make_pair(geomtools::helix_3d::angle_to_t(angle),
                             snemo::datamodel::particle_track::vertex_on_x_calorimeter_label()));
        }
      }
    }

    // Gvetos
    {
      for (size_t iwall = 0; iwall < snemo::geometry::xcalo_locator::NWALLS_PER_SIDE; ++iwall) {
        const double t = a_helix.get_t_from_z(zcalo_bd[iwall]);
        tparams.insert(
            std::make_pair(t, snemo::datamodel::particle_track::vertex_on_gamma_veto_label()));
      }
    }

    // Choose which helix angle to change
    const double t1 = a_helix.get_t1();
    const double t2 = a_helix.get_t2();

    std::pair<double, double> new_ts;
    datatools::invalidate(new_ts.first);
    datatools::invalidate(new_ts.second);
    std::pair<double, double> min_distances;
    datatools::infinity(min_distances.first);
    datatools::infinity(min_distances.second);
    std::pair<std::string, std::string> category_flags;

    for (auto it = tparams.begin(); it != tparams.end(); ++it) {
      const double t = it->first;
      const std::string &category = it->second;

      // Calculate delta t values as well as new lengths
      const double delta1 = std::fabs(t1 - t);
      const double delta2 = std::fabs(t2 - t);

      // Keep smallest distance but remove also too long extrapolation
      if (delta1 < delta2) {
        if (delta1 > min_distances.first) {
          continue;
        }
        new_ts.first = t;
        category_flags.first = category;
        min_distances.first = delta1;
      } else {
        if (delta2 > min_distances.second) {
          continue;
        }
        new_ts.second = t;
        category_flags.second = category;
        min_distances.second = delta2;
      }
    }

    // New angle & calorimeter category (if length not too long)
    const double delta2length =
        2 * M_PI * hypot(a_helix.get_radius(), a_helix.get_step() / (2 * M_PI));
    const double length = a_helix.get_length();
    // Create a mutable helix object to set the new angle
    geomtools::helix_3d *a_mutable_helix = const_cast<geomtools::helix_3d *>(&a_helix);
    if (datatools::is_valid(new_ts.first)) {
      const double new_length = delta2length * std::abs(new_ts.first - a_helix.get_t1());
      const std::string &category = category_flags.first;
      if (_use_vertices_[category] && new_length < length) {
        a_mutable_helix->set_t1(new_ts.first);
        vertices.push_back(std::make_pair(category, a_mutable_helix->get_first()));
      } else {
        vertices.push_back(std::make_pair(snemo::datamodel::particle_track::vertex_on_wire_label(),
                                          a_helix.get_first()));
      }
    }
    if (datatools::is_valid(new_ts.second)) {
      const double new_length = delta2length * std::abs(new_ts.second - a_helix.get_t2());
      const std::string &category = category_flags.second;
      if (_use_vertices_[category] && new_length < length) {
        a_mutable_helix->set_t2(new_ts.second);
        vertices.push_back(std::make_pair(category, a_mutable_helix->get_last()));
      } else {
        vertices.push_back(std::make_pair(snemo::datamodel::particle_track::vertex_on_wire_label(),
                                          a_helix.get_last()));
      }
    }
  }
  // ----- end of helix pattern


  // Save new vertices
  for (auto it = vertices.begin(); it != vertices.end(); ++it) {
    // Check vertex side is on the same side as the trajectory
    if ((side == snemo::geometry::utils::SIDE_BACK && it->second.x() > 0.0) ||
        (side == snemo::geometry::utils::SIDE_FRONT && it->second.x() < 0.0)) {
      DT_LOG_DEBUG(get_logging_priority(), "Closest vertex is on the opposite side!");
    }
    auto spot = datatools::make_handle<geomtools::blur_spot>();
    spot->set_hit_id(vertices_.size());
    spot->grab_auxiliaries().update(snemo::datamodel::particle_track::vertex_type_key(), it->first);
    // Future: determine the GID of the scintillator block or source strip
    // associated to the impact vertex:
    //
    //   int module = 0;
    //   int side = 0;
    //   int strip = -1;
    //   spot.grab_geom_id().set_type(???);
    //   spot.grab_geom_id().set_addresses(module, side/strip?, wall?, column?, row?...);
    // or:
    //   spot.set_geom_id(matchind_gid?);
    //

    // For now it is dimension 3 with no errors nor rotation defined:
    spot->set_blur_dimension(geomtools::blur_spot::dimension_three);
    spot->set_position(it->second);

    vertices_.push_back(spot);

    //
    // Future implementation:= ???
    //
    //   double sigma_x = ???; // Computed from the TrackFit error matrix
    //   double sigma_y = ???; // Computed from the TrackFit error matrix
    //   double sigma_z = ???; // Computed from the TrackFit error matrix
    //   spot.grab_placement().set_translation(it->second);
    //   if (snemo::datamodel::particle_track::vertex_is_on_main_calorimeter(spot)) {
    //     //
    //     // Possibility:
    //     //    +---------------------------------+
    //     //   /                                 /|
    //     //  /                                 / |
    //     // +---------------------------------+  |
    //     // |           ^ z        Calo block |  |
    //     // |  sigma(z) :            entrance |  |
    //     // |          ---            surface |  |
    //     // |         / : \                   |  |
    //     // |        /  :  \ sigma(y)         |  |
    //     // |    - - |- + -|- - -> y          |  |
    //     // |        | /:  |                  |  |
    //     // |  --->   / : /  2D blur spot     |  +
    //     // | normal / ---                    | /
    //     // |       L   :                     |/
    //     // +---------------------------------+
    //     //
    //     //
    //     spot.set_blur_dimension(geomtools::blur_spot::dimension_two);
    //     if (side == snemo::geometry::utils::SIDE_BACK) {
    //       spot.grab_placement().set_orientation(ROTATION_AXIS_Y, 90.0 * CLHEP::degree);
    //     } else {
    //       spot.grab_placement().set_orientation(ROTATION_AXIS_Y, -90.0 * CLHEP::degree);
    //     }
    //     spot.set_errors(sigma_y, sigma_z);
    //   } else ...
    //
  }
}


void vertex_extrapolation_driver::_check_vertices_(
    const snemo::datamodel::tracker_trajectory &trajectory_) {
  if (!trajectory_.has_cluster()) {
    return;
  }
  // Reset values of booleans
  namespace sdm = snemo::datamodel;
  _use_vertices_[sdm::particle_track::vertex_on_source_foil_label()] = false;
  _use_vertices_[sdm::particle_track::vertex_on_main_calorimeter_label()] = false;
  _use_vertices_[sdm::particle_track::vertex_on_x_calorimeter_label()] = false;
  _use_vertices_[sdm::particle_track::vertex_on_gamma_veto_label()] = false;

  const sdm::tracker_cluster &a_cluster = trajectory_.get_cluster();
  const sdm::calibrated_tracker_hit::collection_type &the_hits = a_cluster.get_hits();
  for (const datatools::handle<sdm::calibrated_tracker_hit> a_hit : the_hits) {
    const geomtools::geom_id &a_gid = a_hit->get_geom_id();

    // Extract layer
    const snemo::geometry::gg_locator &gg_locator = _locator_plugin_->get_gg_locator();
    const uint32_t layer = gg_locator.extract_layer(a_gid);
    if (layer < 1) {
      // Extrapolate vertex to the foil if the first GG layers are fired
      _use_vertices_[sdm::particle_track::vertex_on_source_foil_label()] = true;
    }

    const uint32_t side = gg_locator.extract_side(a_gid);
    if (layer >= gg_locator.get_number_of_layers(side) - 1) {
      _use_vertices_[sdm::particle_track::vertex_on_main_calorimeter_label()] = true;
    }

    const uint32_t row = gg_locator.extract_row(a_gid);
    if (row <= 1 || row >= gg_locator.get_number_of_rows(side) - 1) {
      _use_vertices_[sdm::particle_track::vertex_on_x_calorimeter_label()] = true;
    }
  }
}

// static
void vertex_extrapolation_driver::init_ocd(datatools::object_configuration_description &ocd_) {
  // Prefix "VED" stands for "Vertex Extrapolation Driver" :
  datatools::logger::declare_ocd_logging_configuration(ocd_, "fatal", "VED.");

  {
    // Description of the 'VED.use_linear_extrapolation' configuration property :
    datatools::configuration_property_description &cpd = ocd_.add_property_info();
    cpd.set_name_pattern("VED.use_linear_extrapolation")
        .set_from("snemo::reconstruction::vertex_extrapolation_driver")
        .set_terse_description("Use a linear extrapolation to determine vertex position")
        .set_traits(datatools::TYPE_BOOLEAN)
        .set_mandatory(false)
        .set_default_value_boolean(false)
        .set_long_description("Feature to be implemented")
        .add_example(
            "Set the default value::                      \n"
            "                                             \n"
            "  VED.use_linear_extrapolation : boolean = 0 \n"
            "                                             \n");
  }
}

}  // end of namespace reconstruction

}  // end of namespace snemo

/* OCD support */
#include <datatools/object_configuration_description.h>
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(snemo::reconstruction::vertex_extrapolation_driver, ocd_) {
  ocd_.set_class_name("snemo::reconstruction::vertex_extrapolation_driver");
  ocd_.set_class_description("A driver class for vertex extrapolation algorithm");
  ocd_.set_class_library("Falaise_ChargedParticleTracking");
  ocd_.set_class_documentation(
      "This drivers does the extrapolation of tracker trajectories     \n"
      "and builds the list of vertices attached to the particle track. \n");

  // Invoke specific OCD support :
  ::snemo::reconstruction::vertex_extrapolation_driver::init_ocd(ocd_);

  ocd_.set_validation_support(true);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()  // Closing macro for implementation
DOCD_CLASS_SYSTEM_REGISTRATION(snemo::reconstruction::vertex_extrapolation_driver,
                               "snemo::reconstruction::vertex_extrapolation_driver")

// end of falaise/snemo/reconstruction/vertex_extrapolation_driver.cc
