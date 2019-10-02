// falaise/snemo/simulation/calorimeter_step_hit_processor.cc

// Ourselves:
#include <falaise/snemo/simulation/calorimeter_step_hit_processor.h>

// This project:
#include <falaise/snemo/geometry/calo_locator.h>
#include <falaise/snemo/geometry/gveto_locator.h>
#include <falaise/snemo/geometry/locator_plugin.h>
#include <falaise/snemo/geometry/xcalo_locator.h>
#include "falaise/property_set.h"
#include "falaise/snemo/geometry/locator_helpers.h"

namespace snemo {

namespace simulation {

MCTOOLS_STEP_HIT_PROCESSOR_REGISTRATION_IMPLEMENT(
    calorimeter_step_hit_processor, "snemo::simulation::calorimeter_step_hit_processor")

bool calorimeter_step_hit_processor::locate_calorimeter_block(const geomtools::vector_3d& position,
                                                              geomtools::geom_id& gid) const {
  if (geoLocator_->caloLocator().findBlockGID(position, gid)) {
    return true;
  }
  if (geoLocator_->xcaloLocator().findBlockGID(position, gid)) {
    return true;
  }
  if (geoLocator_->gvetoLocator().findBlockGID(position, gid)) {
    return true;
  }
  // Fallback locator from the parent class:
  if (this->mctools::calorimeter_step_hit_processor::locate_calorimeter_block(position, gid)) {
    return true;
  }
  return false;
}

void calorimeter_step_hit_processor::initialize(const datatools::properties& config,
                                                datatools::service_manager& services) {
  this->mctools::calorimeter_step_hit_processor::initialize(config, services);
  falaise::property_set ps{config};
  auto lpname = ps.get<std::string>("locator_plugin_name", "");
  geoLocator_ = snemo::geometry::getSNemoLocator(get_geom_manager(), lpname);
}

}  // end of namespace simulation

}  // end of namespace snemo
