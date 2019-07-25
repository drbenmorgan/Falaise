/// \file falaise/snemo/reconstruction/charge_computation_driver.cc

// Ourselves:
#include <ChargedParticleTracking/charge_computation_driver.h>

// Standard library:
#include <sstream>

// This project (Falaise):
#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>
#include <falaise/snemo/datamodels/line_trajectory_pattern.h>
#include <falaise/snemo/datamodels/particle_track.h>
#include <falaise/snemo/datamodels/tracker_trajectory.h>

namespace snemo {

namespace reconstruction {

const std::string &charge_computation_driver::get_id() {
  static const std::string s("CCD");
  return s;
}

void charge_computation_driver::set_initialized(const bool initialized_) {
  _initialized_ = initialized_;
}

bool charge_computation_driver::is_initialized() const { return _initialized_; }

/// Initialize the driver through configuration properties
void charge_computation_driver::initialize(const falaise::config::property_set& ps) {
  DT_THROW_IF(is_initialized(), std::logic_error, "Driver is already initialized !");

  _charge_from_source_ = ps.get<bool>("charge_from_source",true);

  auto a_direction = ps.get<std::string>("magnetic_field_direction","+z");
  if (a_direction == "+z") {
    _magnetic_field_direction_ = +1;
  } else if (a_direction == "-z") {
    _magnetic_field_direction_ = -1;
  } else {
    DT_THROW_IF(true, std::logic_error,
                "Value for 'magnetic_field_direction' must be either '+z' or '-z'!");
  }

  set_initialized(true);
}

/// Reset the driver
void charge_computation_driver::reset() {
  _initialized_ = false;
  _charge_from_source_ = true;
  _magnetic_field_direction_ = +1;
}

void charge_computation_driver::process(const snemo::datamodel::tracker_trajectory &trajectory_,
                                        snemo::datamodel::particle_track &particle_) {
  DT_THROW_IF(!is_initialized(), std::logic_error, "Driver is not initialized !");

  using snedm = snemo::datamodel;

  // Look first if trajectory pattern is an helix or not
  const snedm::base_trajectory_pattern &a_track_pattern = trajectory_.get_pattern();

  if (a_track_pattern.get_pattern_id() == snedm::line_trajectory_pattern::pattern_id()) {
    particle_.set_charge(snedm::particle_track::undefined);
    return;
  }

  // Retrieve helix trajectory
  const snedm::helix_trajectory_pattern *ptr_helix = 0;
  if (a_track_pattern.get_pattern_id() ==
      snedm::helix_trajectory_pattern::pattern_id()) {
    ptr_helix = dynamic_cast<const snedm::helix_trajectory_pattern *>(&a_track_pattern);
  }
  if (!ptr_helix) {
    return;
  }

  // Retrieve starting and ending point of helix trajectory
  const geomtools::vector_3d first_point = ptr_helix->get_helix().get_first();
  const geomtools::vector_3d last_point = ptr_helix->get_helix().get_last();
  const bool is_negative = std::fabs(first_point.x()) < std::fabs(last_point.x());

  int a_charge = (is_negative ? -1 : +1);
  a_charge *= _magnetic_field_direction_;
  if (!_charge_from_source_) {
    a_charge *= -1;
  }

  if (a_charge < 0) {
    particle_.set_charge(snedm::particle_track::negative);
  } else {
    particle_.set_charge(snedm::particle_track::positive);
  }
}

// static
void charge_computation_driver::init_ocd(datatools::object_configuration_description &ocd_) {
  // Prefix "CCD" stands for "Charge Computation Driver" :
  datatools::logger::declare_ocd_logging_configuration(ocd_, "fatal", "CCD.");

  {
    // Description of the 'CCD.charge_from_source' configuration property :
    datatools::configuration_property_description &cpd = ocd_.add_property_info();
    cpd.set_name_pattern("CCD.charge_from_source")
        .set_from("snemo::reconstruction::charge_computation_driver")
        .set_terse_description(
            "Set the default origin of the particle track to compute its electric charge")
        .set_traits(datatools::TYPE_BOOLEAN)
        .set_mandatory(false)
        .set_default_value_boolean(true)
        .add_example(
            "Set the default value::                \n"
            "                                       \n"
            "  CCD.charge_from_source : boolean = 1 \n"
            "                                       \n");
  }
  {
    // Description of the 'CCD.magnetic_field_direction' configuration property :
    datatools::configuration_property_description &cpd = ocd_.add_property_info();
    cpd.set_name_pattern("CCD.magnetic_field_direction")
        .set_from("snemo::reconstruction::charge_computation_driver")
        .set_terse_description("Set the magnetic field direction i.e. \"+z\" or \"-z\"")
        .set_traits(datatools::TYPE_STRING)
        .set_mandatory(false)
        .set_default_value_string("+z")
        .add_example(
            "Set the default value::                          \n"
            "                                                 \n"
            "  CCD.magnetic_field_direction : string = \"+z\" \n"
            "                                                 \n");
  }
}

}  // end of namespace reconstruction

}  // end of namespace snemo

/* OCD support */
#include <datatools/object_configuration_description.h>
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(snemo::reconstruction::charge_computation_driver, ocd_) {
  ocd_.set_class_name("snemo::reconstruction::charge_computation_driver");
  ocd_.set_class_description("A driver class for electric charge computation algorithm");
  ocd_.set_class_library("Falaise_ChargedParticleTracking");
  ocd_.set_class_documentation(
      "This driver determines the electric charge of the particle track. \n");

  // Invoke specific OCD support :
  ::snemo::reconstruction::charge_computation_driver::init_ocd(ocd_);

  ocd_.set_validation_support(true);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()  // Closing macro for implementation
DOCD_CLASS_SYSTEM_REGISTRATION(snemo::reconstruction::charge_computation_driver,
                               "snemo::reconstruction::charge_computation_driver")

// end of falaise/snemo/reconstruction/charge_computation_driver.cc
