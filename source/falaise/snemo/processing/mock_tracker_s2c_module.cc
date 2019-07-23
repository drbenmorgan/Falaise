// -*- mode: c++ ; -*-
/** \file falaise/snemo/processing/mock_tracker_s2c_module.cc
 */

// Ourselves:
#include <falaise/snemo/processing/mock_tracker_s2c_module.h>

// Standard library:
#include <sstream>
#include <stdexcept>

// Third party:
// - Bayeux/datatools:
#include <datatools/service_manager.h>
// - Bayeux/geomtools:
#include <geomtools/geometry_service.h>
#include <geomtools/manager.h>
// - Bayeux/mctools:
#include <mctools/simulated_data.h>
#include <mctools/utils.h>

// Try enumerations with Boost iterator/range until we have range-v3
#include <boost/range/adaptor/indexed.hpp>

// This project :
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/services/services.h>

namespace {
// Get object stored at key, adding it if not found§
template <typename T>
T& getOrAdd(std::string const& key, datatools::things& event) {
  if (event.has(key)) {
    return event.grab<T>(key);
  }
  return event.add<T>(key);
}

// Extract a service
template <typename Service>
const Service& service_handle(std::string const& name, datatools::service_manager const& provider) {
  DT_THROW_IF(!provider.has(name), std::logic_error, "No service '" << name << "' available");
  DT_THROW_IF(!provider.is_a<Service>(name), std::logic_error,
              "Service '" << name << "' is not of requested type");
  return provider.get<Service>(name);
}

}  // namespace

namespace snemo {

namespace processing {

// Registration instantiation macro :
DPP_MODULE_REGISTRATION_IMPLEMENT(mock_tracker_s2c_module,
                                  "snemo::processing::mock_tracker_s2c_module")

void mock_tracker_s2c_module::initialize(const datatools::properties& ps,
                                         datatools::service_manager& services,
                                         dpp::module_handle_dict_type& /* module_dict_ */) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized ! ");

  this->base_module::_common_initialize(ps);

  sdInputTag = snemo::datamodel::data_info::default_simulated_data_label();
  if (ps.has_key("SD_label")) {
    sdInputTag = ps.fetch_string("SD_label");
  }

  cdOutputTag = snemo::datamodel::data_info::default_calibrated_data_label();
  if (ps.has_key("CD_label")) {
    cdOutputTag = ps.fetch_string("CD_label");
  }

  geoServiceTag = snemo::processing::service_info::default_geometry_service_label();
  if (ps.has_key("Geo_label")) {
    geoServiceTag = ps.fetch_string("Geo_label");
  }

  auto& Geo = service_handle<geomtools::geometry_service>(geoServiceTag, services);
  geoManager = &(Geo.get_geom_manager());

  const std::string& setup_label = geoManager->get_setup_label();
  DT_THROW_IF(
      !(setup_label == "snemo::demonstrator" || setup_label == "snemo::tracker_commissioning"),
      std::logic_error, "Setup label '" << setup_label << "' is not supported !");

  // Module geometry category:
  _module_category_ = "module";
  if (ps.has_key("module_category")) {
    _module_category_ = ps.fetch_string("module_category");
  }

  // Hit category:
  _hit_category_ = "gg";
  if (ps.has_key("hit_category")) {
    _hit_category_ = ps.fetch_string("hit_category");
  }

  int random_seed = 12345;
  if (ps.has_key("random.seed")) {
    random_seed = ps.fetch_integer("random.seed");
  }
  std::string random_id = "mt19937";
  if (ps.has_key("random.id")) {
    random_id = ps.fetch_string("random.id");
  }
  // Initialize the embedded random number generator:
  RNG_.init(random_id, random_seed);

  // Initialize the Geiger regime utility:
  _geiger_.initialize(ps);

  // Set minimum drift time for peripheral hits:
  if (ps.has_key("peripheral_drift_time_threshold")) {
    _peripheral_drift_time_threshold_
      = ps.fetch_real_with_explicit_dimension("peripheral_drift_time_threshold", "time");
  }

  // Default value:
  if (!datatools::is_valid(_peripheral_drift_time_threshold_)) {
    _peripheral_drift_time_threshold_ = _geiger_.get_t0();
  }

  // Set minium drift time for delayed hits:
  if (ps.has_key("delayed_drift_time_threshold")) {
    _delayed_drift_time_threshold_
      = ps.fetch_real_with_explicit_dimension("delayed_drift_time_threshold", "time");
  }

  // Default value:
  if (!datatools::is_valid(_delayed_drift_time_threshold_)) {
    _delayed_drift_time_threshold_ = _geiger_.get_tcut();
  }

  // 2012-07-26 FM : support reference to the MC true hit ID
  _store_mc_hit_id_ = ps.has_flag("store_mc_hit_id");

  // 2014-03-06 FM : support reference to the track ID associated
  // to this calibrated hit
  _store_mc_truth_track_ids_ = ps.has_flag("_store_mc_truth_track_ids");

  this->base_module::_set_initialized(true);
}

void mock_tracker_s2c_module::reset() { this->base_module::_set_initialized(false); }

// Processing :
dpp::base_module::process_status mock_tracker_s2c_module::process(datatools::things& event) {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");

  // Get the 'simulated_data' entry from the data model :
  auto& simulatedData = event.get<mctools::simulated_data>(sdInputTag);

  // Only process if the data we need is present
  if (simulatedData.has_step_hits(_hit_category_)) {
    sim_tracker_hit_col_t const& simTrackerHits = simulatedData.get_step_hits(_hit_category_);

    auto& currentCalData = getOrAdd<snemo::datamodel::calibrated_data>(cdOutputTag, event);
    cal_tracker_hit_col_t& calTrackerHits = currentCalData.calibrated_tracker_hits();

    calTrackerHits = process_(simTrackerHits);
  }

  return dpp::base_module::PROCESS_SUCCESS;
}

/**
 * Here collect the Geiger raw hits from the simulation data source
 * and build the final list of digitized 'tracker' hits.
 *
 */
mock_tracker_s2c_module::raw_tracker_hit_col_t mock_tracker_s2c_module::digitizeHits_(
    const sim_tracker_hit_col_t& hits) {
  // reset the output raw tracker hits collection:
  raw_tracker_hit_col_t rawTrackerDigits{};

  // pickup the ID mapping from the geometry manager:
  const geomtools::mapping& the_mapping = geoManager->get_mapping();

  // Loop on Geiger step hits:
  for (auto const& hit : hits | boost::adaptors::indexed(0)) {
    // Does work, but wait for C++17 structured bindings!
    // boost::tie(a_tracker_hit, raw_tracker_hit_id) = hit;
    auto& a_tracker_hit = hit.value();
    uint32_t const& raw_tracker_hit_id = hit.index();

    int true_tracker_hit_id = a_tracker_hit->get_hit_id();
    int true_tracker_truth_track_id = a_tracker_hit->get_track_id();
    int true_tracker_truth_parent_track_id = a_tracker_hit->get_parent_track_id();
    // extract the corresponding geom ID:
    const geomtools::geom_id& gid = a_tracker_hit->get_geom_id();

    // extract the geom info of the corresponding cell:
    const geomtools::geom_info& ginfo = the_mapping.get_geom_info(gid);

    // the position of the ion/electron pair creation within the cell volume:
    const geomtools::vector_3d& ionization_world_pos = a_tracker_hit->get_position_start();
    // the position of the Geiger avalanche impact on the anode wire:
    const geomtools::vector_3d& avalanche_impact_world_pos = a_tracker_hit->get_position_stop();

    // compute the position of the anode impact in the drift cell coordinates reference frame:
    geomtools::vector_3d avalanche_impact_cell_pos;
    ginfo.get_world_placement().mother_to_child(avalanche_impact_world_pos,
                                                avalanche_impact_cell_pos);
    // longitudinal position:
    const double longitudinal_position = avalanche_impact_cell_pos.z();

    // true drift distance:
    const double drift_distance = (avalanche_impact_world_pos - ionization_world_pos).mag();
    const double anode_efficiency = _geiger_.get_anode_efficiency(drift_distance);
    const double r = RNG_.uniform();
    if (r > anode_efficiency) {
      // This hit is lost due to anode signal inefficiency:
      continue;
    }

    // the time of the ion/electron pair creation:
    const double ionization_time = a_tracker_hit->get_time_start();

    /*** Anode TDC ***/
    // randomize the expected Geiger drift time:
    const double expected_drift_time =
        _geiger_.randomize_drift_time_from_drift_distance(RNG_, drift_distance);
    const double anode_time = ionization_time + expected_drift_time;
    const double sigma_anode_time = _geiger_.get_sigma_anode_time(anode_time);

    /*** Cathodes TDCs ***/
    const double cathode_efficiency = _geiger_.get_cathode_efficiency();
    double bottom_cathode_time{datatools::invalid_real_double()};
    double top_cathode_time{datatools::invalid_real_double()};
    const double sigma_cathode_time = _geiger_.get_sigma_cathode_time();
    size_t missing_cathodes = 2;
    const double r1 = RNG_.uniform();
    if (r1 < cathode_efficiency) {
      const double l_bottom = longitudinal_position + 0.5 * _geiger_.get_cell_length();
      const double mean_bottom_cathode_time = l_bottom / _geiger_.get_plasma_longitudinal_speed();
      const double sigma_bottom_cathode_time = 0.0;
      bottom_cathode_time = RNG_.gaussian(mean_bottom_cathode_time, sigma_bottom_cathode_time);
      if (bottom_cathode_time < 0.0) bottom_cathode_time = 0.0;
      missing_cathodes--;
    }
    const double r2 = RNG_.uniform();
    if (r2 < cathode_efficiency) {
      const double l_top = 0.5 * _geiger_.get_cell_length() - longitudinal_position;
      const double mean_top_cathode_time = l_top / _geiger_.get_plasma_longitudinal_speed();
      const double sigma_top_cathode_time = 0.0;
      top_cathode_time = RNG_.gaussian(mean_top_cathode_time, sigma_top_cathode_time);
      if (top_cathode_time < 0.0) top_cathode_time = 0.0;
      missing_cathodes--;
    }

    // find if some tracker hit already uses this geom ID:
    geomtools::base_hit::has_geom_id_predicate pred_has_gid(gid);
    auto found = std::find_if(rawTrackerDigits.begin(), rawTrackerDigits.end(), pred_has_gid);
    if (found == rawTrackerDigits.end()) {
      // This geom_id is not used by any previous tracker hit: we create a new tracker hit !
      rawTrackerDigits.emplace_back(snemo::datamodel::mock_raw_tracker_hit{});
      snemo::datamodel::mock_raw_tracker_hit& new_raw_tracker_hit = rawTrackerDigits.back();

      // assign a hit ID and the geometry ID to the hit:
      new_raw_tracker_hit.set_hit_id(raw_tracker_hit_id);
      new_raw_tracker_hit.set_geom_id(a_tracker_hit->get_geom_id());
      // 2012-07-26 FM : support reference to the MC truth hit ID
      if (_store_mc_hit_id_ && true_tracker_hit_id > geomtools::base_hit::INVALID_HIT_ID) {
        new_raw_tracker_hit.grab_auxiliaries().store(mctools::hit_utils::HIT_MC_HIT_ID_KEY,
                                                     true_tracker_hit_id);
      }
      // 2014-04-06 FM : support reference to the MC truth track and parent track IDs
      if (_store_mc_truth_track_ids_) {
        if (true_tracker_truth_track_id > mctools::track_utils::INVALID_TRACK_ID) {
          new_raw_tracker_hit.grab_auxiliaries().store(mctools::track_utils::TRACK_ID_KEY,
                                                       true_tracker_truth_track_id);
        }
        if (true_tracker_truth_parent_track_id > mctools::track_utils::INVALID_TRACK_ID) {
          new_raw_tracker_hit.grab_auxiliaries().store(mctools::track_utils::PARENT_TRACK_ID_KEY,
                                                       true_tracker_truth_parent_track_id);
        }
      }

      if (datatools::is_valid(anode_time)) {
        new_raw_tracker_hit.set_drift_time(anode_time);
        new_raw_tracker_hit.set_sigma_drift_time(sigma_anode_time);
        if (datatools::is_valid(top_cathode_time)) {
          new_raw_tracker_hit.set_top_time(top_cathode_time);
          new_raw_tracker_hit.set_sigma_top_time(sigma_cathode_time);
        }
        if (datatools::is_valid(bottom_cathode_time)) {
          new_raw_tracker_hit.set_bottom_time(bottom_cathode_time);
          new_raw_tracker_hit.set_sigma_bottom_time(sigma_cathode_time);
        }
      }
    } else {
      // This geom_id is already used by some previous tracker hit: we update this hit !
      snemo::datamodel::mock_raw_tracker_hit& some_raw_tracker_hit = *found;

      if (datatools::is_valid(anode_time)) {
        if (anode_time < some_raw_tracker_hit.get_drift_time()) {
          // reset TDC infos for the current hit:
          some_raw_tracker_hit.invalidate_times();
          // update TDC infos:
          some_raw_tracker_hit.set_drift_time(anode_time);
          some_raw_tracker_hit.set_sigma_drift_time(sigma_anode_time);
          if (datatools::is_valid(top_cathode_time)) {
            some_raw_tracker_hit.set_top_time(top_cathode_time);
            some_raw_tracker_hit.set_sigma_top_time(sigma_cathode_time);
          }
          if (datatools::is_valid(bottom_cathode_time)) {
            some_raw_tracker_hit.set_bottom_time(bottom_cathode_time);
            some_raw_tracker_hit.set_sigma_bottom_time(sigma_cathode_time);
          }

          // 2012-07-26 FM : support reference to the MC true hit ID
          if (_store_mc_hit_id_ && true_tracker_hit_id > geomtools::base_hit::INVALID_HIT_ID) {
            some_raw_tracker_hit.grab_auxiliaries().update(mctools::hit_utils::HIT_MC_HIT_ID_KEY,
                                                           true_tracker_hit_id);
          }

          // 2014-04-06 FM : support reference to the MC truth track and parent track IDs
          if (_store_mc_truth_track_ids_) {
            if (true_tracker_truth_track_id > mctools::track_utils::INVALID_TRACK_ID) {
              some_raw_tracker_hit.grab_auxiliaries().update(mctools::track_utils::TRACK_ID_KEY,
                                                             true_tracker_truth_track_id);
            }
            if (true_tracker_truth_parent_track_id > mctools::track_utils::INVALID_TRACK_ID) {
              some_raw_tracker_hit.grab_auxiliaries().update(
                  mctools::track_utils::PARENT_TRACK_ID_KEY, true_tracker_truth_parent_track_id);
            }
          }
        }
      }
    }
  }

  return rawTrackerDigits;
}

/** Calibrate tracker hits from digitization informations:
 */
mock_tracker_s2c_module::cal_tracker_hit_col_t mock_tracker_s2c_module::calibrateHits_(
    const raw_tracker_hit_col_t& tracker_digits) {
  cal_tracker_hit_col_t calTrackerHits{};
  calTrackerHits.reserve(tracker_digits.size());

  // pickup the ID mapping from the geometry manager:
  const geomtools::mapping& the_mapping = geoManager->get_mapping();
  const geomtools::id_mgr& the_id_mgr = geoManager->get_id_mgr();

  // current module geometry ID and information:
  int module_number = geomtools::geom_id::INVALID_ADDRESS;
  const geomtools::geom_info* module_ginfo = 0;
  const geomtools::placement* module_placement = 0;

  // Loop on raw tracker hits:
  for (auto const& hit : tracker_digits | boost::adaptors::indexed(0)) {
    // Does work, but wait for C++17 structured bindings!
    // boost::tie(a_tracker_hit, raw_tracker_hit_id) = hit;
    auto& the_raw_tracker_hit = hit.value();

    auto calTrackerHit = datatools::make_handle<snemo::datamodel::calibrated_tracker_hit>();

    // Hit and GeomIDs
    calTrackerHit->set_hit_id(hit.index());
    const geomtools::geom_id& gid = the_raw_tracker_hit.get_geom_id();
    calTrackerHit->set_geom_id(gid);

    // Use the anode time :
    const double anode_time = the_raw_tracker_hit.get_drift_time();

    // Calibrate the transverse drift distance:
    double radius{datatools::invalid_real_double()};
    double sigma_radius{datatools::invalid_real_double()};

    if (datatools::is_valid(anode_time)) {
      if (anode_time <= _delayed_drift_time_threshold_) {
        // Case of a normal/prompt hit :
        _geiger_.calibrate_drift_radius_from_drift_time(anode_time, radius, sigma_radius);
        calTrackerHit->set_anode_time(anode_time);
        if (anode_time > _peripheral_drift_time_threshold_) {
          calTrackerHit->set_peripheral(true);
        }
      } else {
        // 2012-03-29 FM : store the anode_time as the reference delayed time
        calTrackerHit->set_delayed_time(anode_time, _geiger_.get_sigma_anode_time(anode_time));
      }
    } else {
      calTrackerHit->set_noisy(true);
    }
    if (datatools::is_valid(radius)) calTrackerHit->set_r(radius);
    if (datatools::is_valid(sigma_radius)) calTrackerHit->set_sigma_r(sigma_radius);

    // Calibrate the longitudinal drift distance:
    const double t1 = the_raw_tracker_hit.get_bottom_time();
    const double t2 = the_raw_tracker_hit.get_top_time();
    double z{datatools::invalid_real_double()};
    double sigma_z{datatools::invalid_real_double()};

    const double plasma_propagation_speed = _geiger_.get_plasma_longitudinal_speed();
    size_t missing_cathodes = 0;
    if (!datatools::is_valid(t1) && !datatools::is_valid(t2)) {
      // missing top/bottom cathode signals:
      missing_cathodes = 2;
      sigma_z = _geiger_.get_sigma_z(z, missing_cathodes);
      z = 0.0;
      calTrackerHit->set_top_cathode_missing(true);
      calTrackerHit->set_bottom_cathode_missing(true);
    } else if (!datatools::is_valid(t1) && datatools::is_valid(t2)) {
      // missing bottom cathode signal:
      missing_cathodes = 1;
      const double mean_z = 0.5 * _geiger_.get_cell_length() - t2 * plasma_propagation_speed;
      sigma_z = _geiger_.get_sigma_z(mean_z, missing_cathodes);
      z = _geiger_.randomize_z(RNG_, mean_z, sigma_z);
      calTrackerHit->set_bottom_cathode_missing(true);
    } else if (datatools::is_valid(t1) && !datatools::is_valid(t2)) {
      // missing top cathode signal:
      missing_cathodes = 1;
      const double mean_z = t1 * plasma_propagation_speed - 0.5 * _geiger_.get_cell_length();
      sigma_z = _geiger_.get_sigma_z(mean_z, missing_cathodes);
      z = _geiger_.randomize_z(RNG_, mean_z, sigma_z);
      calTrackerHit->set_top_cathode_missing(true);
    } else {
      missing_cathodes = 0;
      const double plasma_propagation_speed_2 = _geiger_.get_cell_length() / (t1 + t2);
      const double mean_z = 0.5 * _geiger_.get_cell_length() - t2 * plasma_propagation_speed_2;
      sigma_z = _geiger_.get_sigma_z(mean_z, missing_cathodes);
      z = _geiger_.randomize_z(RNG_, mean_z, sigma_z);
    }

    // set values in the calibrated tracker hit:
    if (datatools::is_valid(z)) calTrackerHit->set_z(z);
    if (datatools::is_valid(sigma_z)) calTrackerHit->set_sigma_z(sigma_z);

    // COORDINATES...
    // int this_cell_module_number = geom_manager_->get_id_mgr().get(gid, "module");
    const int this_cell_module_number = gid.get(0);
    if (this_cell_module_number != module_number) {
      // build the module GID by extraction from the cell GID:
      geomtools::geom_id module_gid;
      the_id_mgr.make_id(_module_category_, module_gid);
      the_id_mgr.extract(gid, module_gid);
      module_number = this_cell_module_number;
      module_ginfo = &the_mapping.get_geom_info(module_gid);
      module_placement = &(module_ginfo->get_world_placement());
    }

    // extract the geom info of the corresponding cell:
    const geomtools::geom_info& ginfo = the_mapping.get_geom_info(gid);
    // store the X-Y position of the cell within the module coordinate system:
    geomtools::vector_3d cell_self_pos(0.0, 0.0, 0.0);
    geomtools::vector_3d cell_world_pos;
    ginfo.get_world_placement().child_to_mother(cell_self_pos, cell_world_pos);
    geomtools::vector_3d cell_module_pos;
    module_placement->mother_to_child(cell_world_pos, cell_module_pos);
    calTrackerHit->set_xy(cell_module_pos.getX(), cell_module_pos.getY());

    // 2012-07-26 FM : add a reference to the MC true hit ID
    if (_store_mc_hit_id_) {
      if (the_raw_tracker_hit.get_auxiliaries().has_key(mctools::hit_utils::HIT_MC_HIT_ID_KEY)) {
        const int true_tracker_hit_id = the_raw_tracker_hit.get_auxiliaries().fetch_integer(
            mctools::hit_utils::HIT_MC_HIT_ID_KEY);
        calTrackerHit->grab_auxiliaries().update(mctools::hit_utils::HIT_MC_HIT_ID_KEY,
                                                 true_tracker_hit_id);
      }
    }

    // save the calibrate tracker hit:
    calTrackerHits.emplace_back(calTrackerHit);
  }  // loop over raw tracker hits

  return calTrackerHits;
}

mock_tracker_s2c_module::cal_tracker_hit_col_t mock_tracker_s2c_module::process_(
    const sim_tracker_hit_col_t& hits) {
  raw_tracker_hit_col_t rawTrackerHits{digitizeHits_(hits)};
  return calibrateHits_(rawTrackerHits);
}

}  // end of namespace processing

}  // end of namespace snemo

/********************************
 * OCD support : implementation *
 ********************************/

#include <datatools/object_configuration_description.h>

/** Opening macro for implementation
 *  @arg snemo::processing::mock_tracker_s2c_module the full class name
 *  @arg ocd_ is the identifier of the 'datatools::object_configuration_description'
 *            to be initialized (passed by mutable reference).
 */
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(snemo::processing::mock_tracker_s2c_module, ocd_) {
  ocd_.set_class_name("snemo::processing::mock_tracker_s2c_module");
  ocd_.set_class_description(
      "A module that performs a mock digitization/calibration of the tracker simulated data");
  ocd_.set_class_library("falaise");
  // ocd_.set_class_documentation("");

  dpp::base_module::common_ocd(ocd_);

  {
    // Description of the 'SD_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("SD_label")
        .set_terse_description("The label/name of the 'simulated data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the bank to be used    \n"
            "as the output calibrated calorimeter hits. \n")
        .set_default_value_string(snemo::datamodel::data_info::default_simulated_data_label())
        .add_example(
            "Use an alternative name for the 'simulated data' bank:: \n"
            "                                \n"
            "  SD_label : string = \"SD2\"   \n"
            "                                \n");
  }

  {
    // Description of the 'CD_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("CD_label")
        .set_terse_description("The label/name of the 'calibrated data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the bank to be used  \n"
            "as the input simulated calorimeter hits. \n")
        .set_default_value_string(snemo::datamodel::data_info::default_calibrated_data_label())
        .add_example(
            "Use an alternative name for the 'calibrated data' bank:: \n"
            "                                \n"
            "  CD_label : string = \"CD2\"   \n"
            "                                \n");
  }

  {
    std::ostringstream ldesc;
    ldesc << "This is the name of the service to be used as the \n"
          << "geometry service.                                 \n"
          << "This property is only used if no geometry manager \n"
          << "as been provided to the module.                   \n";
    // Description of the 'Geo_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("Geo_label")
        .set_terse_description("The label/name of the geometry service")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(ldesc.str())
        .set_default_value_string(snemo::service_info::default_geometry_service_label())
        .add_example(
            "Use an alternative name for the geometry service:: \n"
            "                                    \n"
            "  Geo_label : string = \"geometry\" \n"
            "                                    \n");
  }

  {
    // Description of the 'random.seed' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("random.seed")
        .set_terse_description("The seed for the embedded PRNG")
        .set_traits(datatools::TYPE_INTEGER)
        .set_mandatory(false)
        .set_long_description("Default value: ``12345``")
        .set_complex_triggering_conditions(true)
        .set_default_value_integer(12345)
        .add_example(
            "Use an alternative seed for the PRNG:: \n"
            "                                       \n"
            "  random.seed : integer = 314159       \n"
            "                                       \n");
  }

  {
    // Description of the 'random.id' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("random.id")
        .set_terse_description("The Id for the embedded PRNG (GSL)")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description("Default value: ``mt19937``")
        .set_complex_triggering_conditions(true)
        .set_default_value_string("mt19937")
        .add_example(
            "Use an alternative Id for the PRNG::   \n"
            "                                       \n"
            "  random.id : string = \"taus2\"       \n"
            "                                       \n");
  }

  {
    // Description of the 'module_category' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("module_category")
        .set_terse_description("The geometry category of the SuperNEMO module")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        //.set_long_description("Default value: ``\"module\"``")
        .set_default_value_string("module")
        .add_example(
            "Use the default value::                 \n"
            "                                        \n"
            "  module_category : string = \"module\" \n"
            "                                        \n");
  }

  {
    // Description of the 'peripheral_drift_time_threshold' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("peripheral_drift_time_threshold")
        .set_terse_description("The minimum drift time for peripheral Geiger hit")
        .set_traits(datatools::TYPE_REAL)
        .set_mandatory(false)
        // .set_long_description("Default value: ``4.0 us``")
        .set_explicit_unit(true)
        .set_unit_label("time")
        .set_unit_symbol("us")
        .set_default_value_real(4.0 * CLHEP::microsecond)
        .add_example(
            "Use the default value::                         \n"
            "                                       \n"
            "  peripheral_drift_time_threshold : real = 4.0 us \n"
            "                                       \n");
  }

  {
    // Description of the 'delayed_drift_time_threshold' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("delayed_drift_time_threshold")
        .set_terse_description("The minimum drift time for delayed Geiger hit")
        .set_traits(datatools::TYPE_REAL)
        .set_mandatory(false)
        // .set_long_description("Default value: ``10.0 us``")
        .set_explicit_unit(true)
        .set_unit_label("time")
        .set_unit_symbol("us")
        .set_default_value_real(10.0 * CLHEP::microsecond)
        .add_example(
            "Use the default value::                         \n"
            "                                                \n"
            "  delayed_drift_time_threshold : real = 10.0 us \n"
            "                                                \n");
  }

  {
    // Description of the 'store_mc_hit_id' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("store_mc_hit_id")
        .set_terse_description(
            "Flag to activate the storage of the truth hit Ids in the calibrated hits")
        .set_traits(datatools::TYPE_BOOLEAN)
        .set_mandatory(false)
        .set_long_description("Default value: ``0`` ")
        .set_default_value_boolean(false)
        .add_example(
            "Use the default value::          \n"
            "                                 \n"
            "  store_mc_hit_id : boolean = 0  \n"
            "                                 \n");
  }

  {
    // Description of the 'store_mc_track_id' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("store_mc_track_id")
        .set_terse_description(
            "Flag to activate the storage of the truth track Id in the calibrated hits")
        .set_traits(datatools::TYPE_BOOLEAN)
        .set_mandatory(false)
        .set_long_description(
            "This information is only available under conditions: \n"
            "the step hit processor must be configured to register\n"
            "this information from the MC engine (Geant4).        \n")
        .set_default_value_boolean(false)
        .add_example(
            "Use the default value::          \n"
            "                                 \n"
            "  store_mc_track_id : boolean = 0  \n"
            "                                 \n");
  }

  {
    // Description of the 'hit_category' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("hit_category")
        .set_terse_description("The category of the traker hits to be processed")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "For this hit category, we must declare some \n"
            "configuration parameters for the associated \n"
            "geiger regime (see OCD support for the      \n"
            "``snemo::processing::geiger_regime`` class).\n")
        .set_default_value_string("gg")
        .add_example(
            "Use the default value::          \n"
            "                                 \n"
            "  hit_category : string = \"gg\" \n"
            "                                 \n");
  }

  // Additionnal configuration hints :
  ocd_.set_configuration_hints(
      "Here is a full configuration example in the                  \n"
      "``datatools::properties`` ASCII format::                     \n"
      "                                                             \n"
      "  SD_label        : string = \"SD\"                          \n"
      "  CD_label        : string = \"CD\"                          \n"
      "  Geo_label       : string = \"geometry\"                    \n"
      "  random.seed     : integer = 314159                         \n"
      "  random.id       : string = \"taus2\"                       \n"
      "  module_category : string = \"module\"                      \n"
      "  peripheral_drift_time_threshold : real = 4.0 us            \n"
      "  delayed_drift_time_threshold    : real = 10.0 us           \n"
      "  store_mc_hit_id  : boolean = 0                             \n"
      "  store_mc_track_id : boolean = 0                            \n"
      "  hit_category     : string = \"gg\"                         \n"
      "  cell_diameter    : real as length = 44 mm                  \n"
      "  cell_length      : real as length = 2900 mm                \n"
      "  sigma_anode_time : real as time = 12.5 ns                  \n"
      "  tcut             : real as time = 10.0 us                  \n"
      "  sigma_cathode_time      : real = 100 ns                    \n"
      "  sigma_z                 : real = 1.0 cm                    \n"
      "  sigma_z_missing_cathode : real as length = 5.0 cm          \n"
      "  sigma_r_a  : real as length = 0.425 mm                     \n"
      "  sigma_r_b  : real = 0.0083                                 \n"
      "  sigma_r_r0 : real as length = 12.25 mm                     \n"
      "  base_anode_efficiency     : real = 1.0                     \n"
      "  base_cathode_efficiency   : real = 1.0                     \n"
      "  plasma_longitudinal_speed : real = 5.0 cm/us               \n"
      "  sigma_plasma_longitudinal_speed : real = 0.5 cm/us         \n"
      "                                                             \n");

  ocd_.set_validation_support(true);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()  // Closing macro for implementation

// Registration macro for class 'snemo::processing::mock_tracker_s2c_module' :
DOCD_CLASS_SYSTEM_REGISTRATION(snemo::processing::mock_tracker_s2c_module,
                               "snemo::processing::mock_tracker_s2c_module")

// end of falaise/snemo/processing/mock_tracker_s2c_module.cc
