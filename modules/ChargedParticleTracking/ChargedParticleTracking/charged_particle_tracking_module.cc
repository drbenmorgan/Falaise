// \file falaise/snemo/reconstruction/charged_particle_tracking_module.cc

// Ourselves:
#include <ChargedParticleTracking/charged_particle_tracking_module.h>

// Standard library:
#include <sstream>
#include <stdexcept>

// Third party:
// - Bayeux/datatools:
#include <bayeux/datatools/service_manager.h>
// - Bayeux/geomtools:
#include <bayeux/geomtools/geometry_service.h>
#include <bayeux/geomtools/manager.h>

// This project (Falaise):
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <falaise/snemo/datamodels/data_model.h>
#include <falaise/snemo/datamodels/particle_track_data.h>
#include <falaise/snemo/datamodels/tracker_clustering_data.h>
#include <falaise/snemo/datamodels/tracker_trajectory_data.h>
#include <falaise/snemo/services/services.h>

// This plugin (ChargedParticleTracking):
#include <ChargedParticleTracking/alpha_finder_driver.h>
#include <ChargedParticleTracking/calorimeter_association_driver.h>
#include <ChargedParticleTracking/charge_computation_driver.h>
#include <ChargedParticleTracking/vertex_extrapolation_driver.h>

namespace snemo {

namespace reconstruction {

// Registration instantiation macro
DPP_MODULE_REGISTRATION_IMPLEMENT(charged_particle_tracking_module,
                                  "snemo::reconstruction::charged_particle_tracking_module")


void charged_particle_tracking_module::_set_defaults() {
  _CD_label_ = snemo::datamodel::data_info::default_calibrated_data_label();
  _TTD_label_ = snemo::datamodel::data_info::default_tracker_trajectory_data_label();
  _PTD_label_ = snemo::datamodel::data_info::default_particle_track_data_label();

  _geometry_manager_ = snemo::service_handle<snemo::geometry_svc>{};

  _VED_.reset();
  _CCD_.reset();
  _CAD_.reset();
  _AFD_.reset();
}

void charged_particle_tracking_module::initialize(
    const datatools::properties& setup_, datatools::service_manager& service_manager_,
    dpp::module_handle_dict_type& /* module_dict_ */) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized ! ");

  dpp::base_module::_common_initialize(setup_);

  if (setup_.has_key("CD_label")) {
    _CD_label_ = setup_.fetch_string("CD_label");
  }

  if (setup_.has_key("TTD_label")) {
    _TTD_label_ = setup_.fetch_string("TTD_label");
  }

  if (setup_.has_key("PTD_label")) {
    _PTD_label_ = setup_.fetch_string("PTD_label");
  }

  // Geometry manager :
  _geometry_manager_ = snemo::service_handle<snemo::geometry_svc>{service_manager_};

  // Drivers :
  std::vector<std::string> driver_names;
  if (setup_.has_key("drivers")) {
    setup_.fetch("drivers", driver_names);
  } else {
    // Add default set of drivers
    driver_names.push_back(snemo::reconstruction::vertex_extrapolation_driver::get_id());
    driver_names.push_back(snemo::reconstruction::charge_computation_driver::get_id());
    driver_names.push_back(snemo::reconstruction::calorimeter_association_driver::get_id());
    driver_names.push_back(snemo::reconstruction::alpha_finder_driver::get_id());
  }
  for (const std::string& a_driver_name : driver_names) {
    if (a_driver_name == snemo::reconstruction::vertex_extrapolation_driver::get_id()) {
      // Initialize Vertex Extrapolation Driver
      _VED_.reset(new snemo::reconstruction::vertex_extrapolation_driver);
      _VED_->set_geometry_manager(*(_geometry_manager_.operator->()));
      datatools::properties VED_config;
      setup_.export_and_rename_starting_with(VED_config, a_driver_name + ".", "");
      _VED_->initialize(VED_config);
    } else if (a_driver_name == snemo::reconstruction::charge_computation_driver::get_id()) {
      // Initialize Charge Computation Driver
      _CCD_.reset(new snemo::reconstruction::charge_computation_driver);
      datatools::properties CCD_config;
      setup_.export_and_rename_starting_with(CCD_config, a_driver_name + ".", "");
      _CCD_->initialize(CCD_config);
    } else if (a_driver_name == snemo::reconstruction::calorimeter_association_driver::get_id()) {
      // Initialize Calorimeter Association Driver
      _CAD_.reset(new snemo::reconstruction::calorimeter_association_driver);
      _CAD_->set_geometry_manager(*(_geometry_manager_.operator->()));
      datatools::properties CAD_config;
      setup_.export_and_rename_starting_with(CAD_config, a_driver_name + ".", "");
      _CAD_->initialize(CAD_config);
    } else if (a_driver_name == snemo::reconstruction::alpha_finder_driver::get_id()) {
      // Initialize Alpha Finder Driver
      _AFD_.reset(new snemo::reconstruction::alpha_finder_driver);
      _AFD_->set_geometry_manager(*(_geometry_manager_.operator->()));
      datatools::properties AFD_config;
      setup_.export_and_rename_starting_with(AFD_config, a_driver_name + ".", "");
      _AFD_->initialize(AFD_config);
    } else {
      DT_THROW_IF(true, std::logic_error, "Driver '" << a_driver_name << "' does not exist !");
    }
  }
  // Tag the module as initialized :
  _set_initialized(true);
}

void charged_particle_tracking_module::reset() {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");
  _set_initialized(false);
  _set_defaults();
}

// Constructor :
charged_particle_tracking_module::charged_particle_tracking_module(
    datatools::logger::priority logging_priority_)
    : dpp::base_module(logging_priority_) {
  _set_defaults();
}

// Destructor :
charged_particle_tracking_module::~charged_particle_tracking_module() {
  if (is_initialized()) {
    charged_particle_tracking_module::reset();
  }
}

// Processing :
dpp::base_module::process_status charged_particle_tracking_module::process(
    datatools::things& data_record_) {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");
  // Get required input products
  const auto& the_calibrated_data = data_record_.get<snemo::datamodel::calibrated_data>(_CD_label_);
  const auto& the_tracker_trajectory_data = data_record_.get<snemo::datamodel::tracker_trajectory_data>(_TTD_label_);

  // Create or reset output bank
  auto the_particle_track_data = snemo::datamodel::getOrAddToEvent<snemo::datamodel::particle_track_data>(_PTD_label_, data_record_);
  the_particle_track_data.reset();

  // Main processing method :
  this->_process(the_calibrated_data, the_tracker_trajectory_data, the_particle_track_data);
  this->_post_process(the_calibrated_data, the_particle_track_data);

  return dpp::base_module::PROCESS_SUCCESS;
}

void charged_particle_tracking_module::_process(
    const snemo::datamodel::calibrated_data& calibrated_data_,
    const snemo::datamodel::tracker_trajectory_data& tracker_trajectory_data_,
    snemo::datamodel::particle_track_data& particle_track_data_) {
  if (!tracker_trajectory_data_.has_default_solution()) {
    // Fill non associated calorimeter hits
    for (const auto& chit : calibrated_data_.calibrated_calorimeter_hits()) {
      particle_track_data_.grab_non_associated_calorimeters().push_back(chit);
    }
    return;
  }

  const snemo::datamodel::tracker_trajectory_solution& a_solution =
      tracker_trajectory_data_.get_default_solution();
  const snemo::datamodel::tracker_trajectory_solution::trajectory_col_type& trajectories =
      a_solution.get_trajectories();

  for (const datatools::handle<snemo::datamodel::tracker_trajectory> a_trajectory : trajectories) {
    // Look into properties to find the default
    // trajectory. Here, default means the one with the best
    // chi2. This flag is set by the 'fitting' module.
    // Implies that there should be a member function of
    // tracker_trajectory_solution to get the default?
    if (!a_trajectory->get_auxiliaries().has_flag("default")) {
      continue;
    }

    // Add a new particle_track
    auto hPT = datatools::make_handle<snemo::datamodel::particle_track>();
    hPT->set_trajectory_handle(a_trajectory);
    hPT->set_track_id(particle_track_data_.get_number_of_particles());
    particle_track_data_.add_particle(hPT);

    // Compute particle charge
    if (_CCD_) {
      _CCD_->process(*a_trajectory, *hPT);
    }
    // Determine track vertices
    if (_VED_) {
      _VED_->process(*a_trajectory, *hPT);
    }
    // Associate vertices to calorimeter hits
    if (_CAD_) {
      _CAD_->process(calibrated_data_.calibrated_calorimeter_hits(), *hPT);
    }
  }

  // Alpha finder
  if (_AFD_) {
    _AFD_->process(tracker_trajectory_data_, particle_track_data_);
  }
}

void charged_particle_tracking_module::_post_process(
    const snemo::datamodel::calibrated_data& calibrated_data_,
    snemo::datamodel::particle_track_data& particle_track_data_) {

  // Grab non associated calorimeters :
  if (!particle_track_data_.has_non_associated_calorimeters()) {
    geomtools::base_hit::has_flag_predicate asso_pred(calorimeter_utils::associated_flag());
    geomtools::base_hit::negates_predicate not_asso_pred(asso_pred);
    // Wrapper predicates :
    datatools::mother_to_daughter_predicate<geomtools::base_hit,
                                            snemo::datamodel::calibrated_calorimeter_hit>
        pred_M2D(not_asso_pred);
    datatools::handle_predicate<snemo::datamodel::calibrated_calorimeter_hit> pred_via_handle(
        pred_M2D);

    const snemo::datamodel::calibrated_data::calorimeter_hit_collection_type& chits =
        calibrated_data_.calibrated_calorimeter_hits();
    // The below might be better with copy_if and back_inserter?
    auto ihit = std::find_if(chits.begin(), chits.end(), pred_via_handle);
    while (ihit != chits.end()) {
      particle_track_data_.grab_non_associated_calorimeters().push_back(*ihit);
      ihit = std::find_if(++ihit, chits.end(), pred_via_handle);
    }
  }

  // 2015/12/02 XG: Also look if the non associated calorimeters are
  // isolated i.e. without Geiger cells in front or not: tag them
  // consequently
  const snemo::datamodel::calibrated_data::tracker_hit_collection_type& thits =
      calibrated_data_.calibrated_tracker_hits();
  snemo::datamodel::calibrated_data::calorimeter_hit_collection_type& chits =
      particle_track_data_.grab_non_associated_calorimeters();

  for (auto chit = chits.begin(); chit != chits.end(); ++chit) {
    snemo::datamodel::calibrated_calorimeter_hit& a_calo_hit = chit->grab();
    const bool has_neighbors =
        calorimeter_utils::has_flag(a_calo_hit, calorimeter_utils::neighbor_flag());
    bool has_gg_in_front = false;

    // Getting geometry mapping for parted block
    const geomtools::mapping& the_mapping = _geometry_manager_->get_mapping();
    std::vector<geomtools::geom_id> gids;
    the_mapping.compute_matching_geom_id(a_calo_hit.get_geom_id(), gids);

    for (const geomtools::geom_id& a_gid : gids) {
      const geomtools::geom_info* ginfo_ptr = the_mapping.get_geom_info_ptr(a_gid);
      if (!ginfo_ptr) {
        DT_LOG_WARNING(get_logging_priority(), "Unmapped geom id " << a_gid << "!");
        continue;
      }
      // Loop over all calibrated geiger hits to find one close enough
      for (auto thit = thits.begin(); thit != thits.end(); ++thit) {
        const snemo::datamodel::calibrated_tracker_hit& a_tracker_hit = thit->get();
        if (!a_tracker_hit.has_xy()) {
          continue;
        }
        const geomtools::vector_3d cell_pos(a_tracker_hit.get_x(), a_tracker_hit.get_y(),
                                            a_tracker_hit.get_z());
        // Tolerance must be understood as 'skin' tolerance so must be
        // multiplied by a factor of 2
        const double tolerance = 100 * CLHEP::mm;
        if (the_mapping.check_inside(*ginfo_ptr, cell_pos, tolerance, true)) {
          has_gg_in_front = true;
          break;
        }
      }  // end of tracker hits

      if (has_gg_in_front) {
        break;
      }
    }  // end of calorimeter geom ids

    if (!has_gg_in_front || (has_neighbors && has_gg_in_front)) {
      calorimeter_utils::flag_as(a_calo_hit, calorimeter_utils::isolated_flag());
    }
  }  // end of calorimeter hits
}

}  // end of namespace reconstruction

}  // end of namespace snemo

/* OCD support */
#include <datatools/object_configuration_description.h>
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(snemo::reconstruction::charged_particle_tracking_module, ocd_) {
  ocd_.set_class_name("snemo::reconstruction::charged_particle_tracking_module");
  ocd_.set_class_description(
      "A module that performs the physical interpretation of tracker trajectory");
  ocd_.set_class_library("Falaise_ChargedParticleTracking");
  ocd_.set_class_documentation(
      "This module uses some dedicated drivers to reconstruct physical quantities        \n"
      "such as electric charge or track vertices.                                        \n"
      "It uses 3 dedicated drivers to perform reconstruction steps:                      \n"
      " 1) Charge Computation Driver determines the electric charge of the track         \n"
      " 2) Vertex Extrapolation Driver builds the list of vertices such as               \n"
      "    foil vertex or calorimeter wall vertices                                      \n"
      " 3) Calorimeter Association Driver associates a track with a calorimeter hit      \n"
      " 4) Alpha Finder Driver looks for short alpha track with only 1 or 2 tracker hits \n");

  // Invoke OCD support from parent class :
  dpp::base_module::common_ocd(ocd_);

  {
    // Description of the 'CD_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("CD_label")
        .set_terse_description("The label/name of the 'calibrated data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the bank to be used  \n"
            "as the source of input calorimeter hits. \n")
        .set_default_value_string(snemo::datamodel::data_info::default_calibrated_data_label())
        .add_example(
            "Use an alternative name for the \n"
            "'calibrated data' bank::        \n"
            "                                \n"
            "  CD_label : string = \"CD2\"   \n"
            "                                \n");
  }

  {
    // Description of the 'TTD_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("TTD_label")
        .set_terse_description("The label/name of the 'tracker trajectory data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the bank to be used      \n"
            "as the source of input tracker trajectories. \n")
        .set_default_value_string(
            snemo::datamodel::data_info::default_tracker_trajectory_data_label())
        .add_example(
            "Use an alternative name for the  \n"
            "'tracker trajectory data' bank:: \n"
            "                                 \n"
            "  TTD_label : string = \"TTD2\"  \n"
            "                                 \n");
  }

  {
    // Description of the 'PTD_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("PTD_label")
        .set_terse_description("The label/name of the 'particle track data' bank")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the bank to be used as \n"
            "the sink of output particle tracks.        \n")
        .set_default_value_string(snemo::datamodel::data_info::default_particle_track_data_label())
        .add_example(
            "Use an alternative name for the \n"
            "'particle track data' bank::    \n"
            "                                \n"
            "  PTD_label : string = \"PTD2\" \n"
            "                                \n");
  }

  {
    // Description of the 'Geo_label' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("Geo_label")
        .set_terse_description("The label/name of the geometry service")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_long_description(
            "This is the name of the service to be used as the \n"
            "geometry service.                                 \n"
            "This property is only used if no geometry manager \n"
            "has been provided to the module.                  \n")
        .set_default_value_string(snemo::service_info::default_geometry_service_label())
        .add_example(
            "Use an alternative name for the geometry service:: \n"
            "                                                   \n"
            "  Geo_label : string = \"geometry2\"               \n"
            "                                                   \n");
  }

  {
    // Description of the 'drivers' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("drivers")
        .set_terse_description("The list of drivers id to be used")
        .set_traits(datatools::TYPE_STRING, datatools::configuration_property_description::ARRAY)
        .set_long_description(
            "Supported values are:                         \n"
            "                                              \n"
            " * ``CCD`` for Charge Computation Driver      \n"
            " * ``VED`` for Vertex Extrapolation Driver    \n"
            " * ``CAD`` for Calorimeter Association Driver \n"
            " * ``AFD`` for Alpha Finder Driver            \n"
            "                                              \n")
        .set_mandatory(false)
        .add_example(
            "Use Vertex Extrapolation Driver only:: \n"
            "                                       \n"
            "  drivers : string[1] = \"VED\"        \n"
            "                                       \n");
  }

  // Invoke specific OCD support from the driver class:
  ::snemo::reconstruction::vertex_extrapolation_driver::init_ocd(ocd_);
  ::snemo::reconstruction::charge_computation_driver::init_ocd(ocd_);
  ::snemo::reconstruction::calorimeter_association_driver::init_ocd(ocd_);

  // Additionnal configuration hints :
  ocd_.set_configuration_hints(
      "Here is a full configuration example in the ``datatools::properties`` \n"
      "ASCII format::                                                        \n"
      "                                                                      \n"
      "  CD_label                     : string = \"CD\"                      \n"
      "  TTD_label                    : string = \"TTD\"                     \n"
      "  PTD_label                    : string = \"PTD\"                     \n"
      "  Geo_label                    : string = \"geometry\"                \n"
      "  drivers                      : string[3] = \"VED\" \"CCD\" \"CAD\"  \n"
      "  VED.logging.priority         : string = \"fatal\"                   \n"
      "  VED.use_linear_extrapolation : boolean = 0                          \n"
      "  CCD.logging_priority         : string = \"fatal\"                   \n"
      "  CCD.charge_from_source       : boolean = 1                          \n"
      "  CAD.logging_priority         : string = \"fatal\"                   \n"
      "  CAD.matching_tolerance       : real as length = 50 mm               \n"
      "                                                                      \n");

  ocd_.set_validation_support(true);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()  // Closing macro for implementation
DOCD_CLASS_SYSTEM_REGISTRATION(snemo::reconstruction::charged_particle_tracking_module,
                               "snemo::reconstruction::charged_particle_tracking_module")

// end of falaise/snemo/reconstruction/charged_particle_tracking_module.cc
