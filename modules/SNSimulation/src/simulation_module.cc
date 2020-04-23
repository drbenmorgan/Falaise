/* simulation_module.cc */

// Ourselves:
#include <mctools/g4/simulation_module.h>

// Standard library:
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Third party:
// - Falaise
#include <falaise/property_set.h>
#include <falaise/snemo/services/geometry.h>
#include <falaise/snemo/services/service_handle.h>

// - Bayeux/datatools:
#include <datatools/exception.h>
#include <datatools/ioutils.h>
#include <datatools/properties.h>
#include <datatools/service_manager.h>
#include <datatools/utils.h>
// - Bayeux/geomtools:
#include <geomtools/geometry_service.h>
// - Bayeux/mctools:
#include <mctools/simulated_data.h>
#include <mctools/utils.h>

// This project:
#include <mctools/g4/event_action.h>
#include <mctools/g4/manager.h>
#include <mctools/g4/simulation_ctrl.h>

namespace {
template <typename T>
void maybe_assign(falaise::property_set const& ps, std::string const& key, T& parameter) {
  parameter = ps.get<T>(key, parameter);
}
}  // namespace

namespace mctools {
namespace g4 {

// Registration instantiation macro :
DPP_MODULE_REGISTRATION_IMPLEMENT(simulation_module, "mctools::g4::simulation_module")

// Constructor :
simulation_module::simulation_module(datatools::logger::priority logging_priority)
    : dpp::base_module(logging_priority) {
  geometryServiceName_ = "";
  simdataBankName_ = "";
  geometryManagerRef_ = nullptr;
  geant4Simulation_ = nullptr;
  geant4SimulationController_ = nullptr;
}

// Destructor :
simulation_module::~simulation_module() { reset(); }

// Initialization :
void simulation_module::initialize(const datatools::properties& ps,
                                   datatools::service_manager& service_manager_,
                                   dpp::module_handle_dict_type& /*module_dict_*/) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized !");

  // Parsing configuration starts here :
  this->_common_initialize(ps);

  falaise::property_set fps{ps};
  maybe_assign(fps, "Geo_label", geometryServiceName_);
  maybe_assign(fps, "SD_label", simdataBankName_);
  if (simdataBankName_.empty()) {
    // Use the default label for the 'simulated data' bank :
    simdataBankName_ = ::mctools::event_utils::EVENT_DEFAULT_SIMULATED_DATA_LABEL;  // "SD"
  }

  // Special setup parameters for the mctools::g4 simulation manager :
  // Force non-interactive parameters:
  geant4Parameters_.interactive = false;
  geant4Parameters_.g4_visu = false;
  geant4Parameters_.g4_macro = "";
  geant4Parameters_.output_data_file.clear();

  // Extract from pset
  auto geant4PSet = fps.get<falaise::property_set>("manager", {});
  maybe_assign(geant4PSet, "logging", geant4Parameters_.logging);
  maybe_assign(geant4PSet, "configuration_filename", geant4Parameters_.manager_config_filename);
  maybe_assign(geant4PSet, "seed", geant4Parameters_.mgr_seed);
  maybe_assign(geant4PSet, "vertex_generator_name", geant4Parameters_.vg_name);
  maybe_assign(geant4PSet, "vertex_generator_seed", geant4Parameters_.vg_seed);
  maybe_assign(geant4PSet, "event_generator_name", geant4Parameters_.eg_name);
  maybe_assign(geant4PSet, "event_generator_seed", geant4Parameters_.eg_seed);
  maybe_assign(geant4PSet, "shpf_seed", geant4Parameters_.shpf_seed);
  maybe_assign(geant4PSet, "input_prng_seeds_file", geant4Parameters_.input_prng_seeds_file);
  maybe_assign(geant4PSet, "output_prng_seeds_file", geant4Parameters_.output_prng_seeds_file);
  maybe_assign(geant4PSet, "input_prng_states_file", geant4Parameters_.input_prng_states_file);
  maybe_assign(geant4PSet, "output_prng_states_file", geant4Parameters_.output_prng_states_file);
  maybe_assign(geant4PSet, "prng_states_save_modulo", geant4Parameters_.prng_states_save_modulo);
  // PSet is strict here on signed/unsigned
  // maybe_assign(geant4PSet, "number_of_events_modulo", geant4Parameters_.number_of_events_modulo);
  // But this is fine...
  geant4Parameters_.number_of_events_modulo = geant4PSet.get<int>("number_of_events_modulo", 0);
  // could extract always extract integers as maxint_t, then check value fits in requested type?

  // Services setup
  if (geometryManagerRef_ == nullptr) {
    snemo::service_handle<snemo::geometry_svc> tmpHandle{service_manager_};
    this->set_geometry_manager(*(tmpHandle.operator->()));
    // Double check (though service_handle should guarantee a non-nullptr)
    DT_THROW_IF(geometryManagerRef_ == nullptr, std::logic_error, "Missing geometry manager !");
  }
  this->_initialize_manager(service_manager_);
  this->_set_initialized(true);
}

// Reset :
void simulation_module::reset() {
  _set_initialized(false);

  if (geant4SimulationController_ != nullptr) {
    // Destruction of the thread synchronization object :
    geant4SimulationController_->set_stop_requested();
    delete geant4SimulationController_;
    geant4SimulationController_ = nullptr;
  }

  // Destroy internal resources :
  _terminate_manager();

  // Blank the module with default neutral values :
  geometryManagerRef_ = nullptr;
  simdataBankName_ = "";
  geometryServiceName_ = "";
}

// Processing :
auto simulation_module::process(datatools::things& event) -> dpp::base_module::process_status {
  DT_THROW_IF(!this->is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");
  // Bank name must be unique
  DT_THROW_IF(event.has(simdataBankName_), std::runtime_error,
              "Work item input to module '" << this->get_name() << "' already has data bank named '"
                                            << simdataBankName_ << "'");

  auto& sdBank = event.add<mctools::simulated_data>(simdataBankName_);

  int status = this->_simulate_event(sdBank);

  return status == 0 ? dpp::base_module::PROCESS_OK : dpp::base_module::PROCESS_FATAL;
}

void simulation_module::set_geo_label(const std::string& geo_) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized !");
  geometryServiceName_ = geo_;
}

auto simulation_module::get_geo_label() const -> const std::string& { return geometryServiceName_; }

void simulation_module::set_sd_label(const std::string& sd_) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized !");
  simdataBankName_ = sd_;
}

auto simulation_module::get_sd_label() const -> const std::string& { return simdataBankName_; }

void simulation_module::set_geometry_manager(const geomtools::manager& geometry_manager_) {
  DT_THROW_IF(geometryManagerRef_ != nullptr && geometryManagerRef_->is_initialized(),
              std::logic_error, "Embedded geometry manager is already initialized !");
  geometryManagerRef_ = &geometry_manager_;
}

void simulation_module::set_geant4_parameters(const manager_parameters& params_) {
  DT_THROW_IF(is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is already initialized !");
  geant4Parameters_ = params_;
}

auto simulation_module::get_geant4_parameters() const -> const manager_parameters& {
  return geant4Parameters_;
}

auto simulation_module::get_seed_manager() const -> const mygsl::seed_manager& {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");
  return geant4Simulation_->get_seed_manager();
}

auto simulation_module::get_state_manager() const -> const mygsl::prng_state_manager& {
  DT_THROW_IF(!is_initialized(), std::logic_error,
              "Module '" << get_name() << "' is not initialized !");
  return geant4Simulation_->get_state_manager();
}

void simulation_module::_initialize_manager(datatools::service_manager& smgr_) {
  // Allocate the simulation manager :
  geant4Simulation_ = new manager;
  geant4Simulation_->set_service_manager(smgr_);
  // Install the geometry manager accessed from the
  // Geometry service (bypassing the embedded geometry manager
  // in the simulation manager) :
  geant4Simulation_->set_external_geom_manager(*geometryManagerRef_);

  // Install a simulation controler in the manager :
  if (geant4SimulationController_ == nullptr) {
    geant4SimulationController_ = new simulation_ctrl(*geant4Simulation_);
    geant4Simulation_->set_simulation_ctrl(*geant4SimulationController_);
  }

  // Setup :
  mctools::g4::manager_parameters::setup(geant4Parameters_, *geant4Simulation_);
}

void simulation_module::_terminate_manager() {
  delete geant4SimulationController_;
  geant4SimulationController_ = nullptr;

  delete geant4Simulation_;
  geant4Simulation_ = nullptr;

  geant4Parameters_.reset();
}

auto simulation_module::_simulate_event(mctools::simulated_data& sdBank) -> int {
  geant4Simulation_->grab_user_event_action().set_external_event_data(sdBank);
  {
    boost::mutex::scoped_lock lock(*geant4SimulationController_->event_mutex);

    if (geant4SimulationController_->simulation_thread == nullptr) {
      geant4SimulationController_->start();
    }
    geant4SimulationController_->event_availability_status = simulation_ctrl::AVAILABLE_FOR_G4;
    geant4SimulationController_->event_available_condition->notify_one();
  }

  // Wait for the release of the event control by the G4 process :
  {
    boost::mutex::scoped_lock lock(*geant4SimulationController_->event_mutex);
    while (geant4SimulationController_->event_availability_status ==
           simulation_ctrl::AVAILABLE_FOR_G4) {
      geant4SimulationController_->event_available_condition->wait(
          *geant4SimulationController_->event_mutex);
    }
  }
  return 0;
}
}  // end of namespace g4
}  // end of namespace mctools

// OCD support for class 'mctools::g4::simulation_module' :
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(::mctools::g4::simulation_module, ocd_) {
  ocd_.set_class_name("mctools::g4::simulation_module");
  ocd_.set_class_library("mctools_g4");
  ocd_.set_class_description("A module to generate Monte-Carlo events through the Geant4 library");

  ocd_.set_configuration_hints(
      "A ``mctools::g4::simulation_module`` object can be setup with the    \n"
      "following syntax in a ``datatools::multi_properties`` configuration  \n"
      "file, typically from a module manager object.                        \n"
      "                                                                     \n"
      "Example::                                                            \n"
      "                                                                     \n"
      "  #@description A module that generates raw data                     \n"
      "  #@key_label   \"name\"                                             \n"
      "  #@meta_label  \"type\"                                             \n"
      "                                                                     \n"
      "  [name=\"g4sim\" type=\"mctools::g4::simulation_module\"]           \n"
      "  #@config A Geant4 simulation module                                \n"
      "  foo : string = \"bar\"                                             \n"
      "                                                                     \n");

  ocd_.set_validation_support(false);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()
DOCD_CLASS_SYSTEM_REGISTRATION(::mctools::g4::simulation_module, "mctools::g4::simulation_module")

/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/
