// -*- mode: c++ ; -*-
/** \file falaise/snemo/processing/CalorimeterModel.cc
 */

// Ourselves:
#include <falaise/snemo/processing/calorimeter_regime.h>

// Standard library:
#include <cmath>

// Third party:
// - Bayeux/datatools:
#include <datatools/clhep_units.h>
#include <datatools/properties.h>
// - Bayeux/mygsl:
#include <mygsl/rng.h>

namespace {
  const double fwhm2sig{1.0 / (2 * sqrt(2 * log(2.0)))}; 
}

namespace snemo {

namespace processing {

CalorimeterModel::CalorimeterModel(datatools::properties const& config) : CalorimeterModel::CalorimeterModel() {
  // This is not ideal, but best we can do for now...
  const double energy_unit = CLHEP::keV;
  const double time_unit = CLHEP::ns;

  if (config.has_key("energy.resolution")) {
    _resolution_ = config_.fetch_real_with_explicit_dimension("energy.resolution", "fraction");
  }

  if (config.has_key("energy.high_threshold")) {
    _high_threshold_ = config_.fetch_real_with_explicit_dimension("energy.high_threshold", "energy");
  }

  if (config.has_key("energy.low_threshold")) {
    _low_threshold_ = config_.fetch_real_with_explicit_dimension("energy.low_threshold", "energy");
  }

  // Alpha quenching fit parameters
  const std::string key_name = "alpha_quenching_parameters";
  if (config.has_key(key_name)) {
    _alpha_quenching_0_ = config_.fetch_real_vector(key_name, 0);
    _alpha_quenching_1_ = config_.fetch_real_vector(key_name, 1);
    _alpha_quenching_2_ = config_.fetch_real_vector(key_name, 2);
  }

  // Scintillator relaxation time for time resolution
  if (config_.has_key("scintillator_relaxation_time")) {
    _scintillator_relaxation_time_
      = config.fetch_real_with_explicit_dimension("scintillator_relaxation_time", "time");
  }
}

double CalorimeterModel::randomize_energy(mygsl::rng& ran_, const double energy_) const {
  // 2015-01-08 XG: Implement a better energy calibration based on Poisson
  // statistics for the number of photons inside scintillator. This
  // technique should be more accurate for low energy deposit.
  // const double fwhm2sig = 2*sqrt(2*log(2.0));
  // const double nrj2photon = std::pow(fwhm2sig/energyResolution, 2);
  // const double mu = energy_ / CLHEP::MeV * nrj2photon;
  // const double spread_energy = ran_.poisson(mu) / nrj2photon;
  // return spread_energy;

  // 2016-06-01 XG: Get back to gaussian fluctuation to avoid fixed number
  // of photon-electron due to Poisson distribution
  const double sigma_energy = get_sigma_energy(energy_);
  const double spread_energy = ran_.gaussian(energy_, sigma_energy);
  return (spread_energy < 0.0 ? 0.0 : spread_energy);
}

double CalorimeterModel::get_sigma_energy(const double energy_) const {
  return energyResolution * fwhm2sig * sqrt(energy_ / CLHEP::MeV);
}

double CalorimeterModel::quench_alpha_energy(const double energy_) const {
  const double energy = energy_ * CLHEP::MeV;

  const double mod_energy = 1.0 / (alphaQuenching_1 * energy + 1.0);
  const double quenching_factor =
      -alphaQuenching_0 * (std::pow(mod_energy, alphaQuenching_2) - std::pow(mod_energy, alphaQuenching_2 / 2.0));

  return energy / quenching_factor;
}

double CalorimeterModel::randomize_time(mygsl::rng& ran_, const double time_,
                                          const double energy_) const {
  const double sigma_time = get_sigma_time(energy_);
  // Negative time are physical since input time is relative
  return ran_.gaussian(time_, sigma_time);
}

double CalorimeterModel::get_sigma_time(const double energy) const {
  // Have a look inside Gregoire Pichenot thesis(NEMO2) and
  // L. Simard parametrization for NEMO3 simulation
  const double sigma_e = energyResolution * fwhm2sig;
  return relaxationTime * sigma_e / sqrt(energy / CLHEP::MeV);
}

bool CalorimeterModel::is_high_threshold(const double energy_) const {
  return (energy_ >= highEnergyThreshold);
}

bool CalorimeterModel::is_low_threshold(const double energy_) const {
  return (energy_ >= lowEnergyThreshold);
}

}  // end of namespace processing

}  // end of namespace snemo

/********************************
 * OCD support : implementation *
 ********************************/

#include <datatools/object_configuration_description.h>

/** Opening macro for implementation
 *  @arg snemo::processing::CalorimeterModel  the full class name
 *  @arg ocd_ is the identifier of the 'datatools::object_configuration_description'
 *            to be initialized (passed by mutable reference).
 */
DOCD_CLASS_IMPLEMENT_LOAD_BEGIN(snemo::processing::CalorimeterModel, ocd_) {
  ocd_.set_class_name("snemo::processing::CalorimeterModel");
  ocd_.set_class_description(
      "A model of the energy/time smearing in SuperNEMO Wall and Veto Calorimeters");
  ocd_.set_class_library("falaise");
  // ocd_.set_class_documentation("");

  {
    // Description of the 'energy.resolution' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("energy.resolution")
        .set_terse_description("The optical line energy resolution for electrons at 1 MeV")
        .set_traits(datatools::TYPE_REAL)
        .set_explicit_unit(true)
        .set_unit_label("fraction")
        .set_unit_symbol("%")
        .add_example(
            "Set the default value::                          \n"
            "                                                 \n"
            "  energy.resolution : real as fraction = 8 %     \n"
            "                                                 \n");
  }

  {
    // Description of the 'energy.low_threshold' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("energy.low_threshold")
        .set_terse_description("The optical line low energy threshold")
        .set_traits(datatools::TYPE_REAL)
        //.set_long_description("Default value: ``50 keV``             \n")
        .set_explicit_unit(true)
        .set_unit_label("energy")
        .set_unit_symbol("keV")
        .add_example(
            "Set the default value::                          \n"
            "                                                 \n"
            "  energy.low_threshold : real as energy = 50 keV \n"
            "                                                 \n");
  }

  {
    // Description of the 'energy.high_threshold' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("energy.high_threshold")
        .set_terse_description("The optical line high energy threshold")
        .set_traits(datatools::TYPE_REAL)
        //.set_long_description("Default value: ``150 keV``           \n")
        .set_explicit_unit(true)
        .set_unit_label("energy")
        .set_unit_symbol("keV")
        .add_example(
            "Set the default value::                            \n"
            "                                                   \n"
            "  energy.high_threshold : real as energy = 150 keV \n"
            "                                                   \n");
  }

  {
    // Description of the 'scintillator_relaxation_time' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("scintillator_relaxation_time")
        .set_terse_description("The scintillator relaxation time")
        .set_traits(datatools::TYPE_REAL)
        .set_long_description(
            "This parameter is used to compute the time resolution of the calorimeter. \n")
        .set_explicit_unit(true)
        .set_unit_label("time")
        .set_unit_symbol("ns")
        .add_example(
            "Set the default value::                                \n"
            "                                                       \n"
            "  scintillator_relaxation_time : real as time = 6.0 ns \n"
            "                                                       \n");
  }

  {
    // Description of the 'alpha_quenching_parameters' configuration property :
    datatools::configuration_property_description& cpd = ocd_.add_property_info();
    cpd.set_name_pattern("alpha_quenching_parameters")
        .set_terse_description("Activation of the quenching for alpha particles")
        .set_traits(datatools::TYPE_REAL, configuration_property_description::ARRAY, 3)
        .set_long_description(
            "The current implementation use a fit with 3 parameters. \n"
            "The default dimensionless values are: ``77.4  0.639  2.34``    \n")
        .set_explicit_unit(false)
        .add_example(
            "Set the default value::                     \n"
            "                                            \n"
            "  alpha_quenching_parameters : real[3] = \\ \n"
            "      77.4  0.639  2.34                     \n"
            "                                            \n");
  }

  // Additionnal configuration hints :
  ocd_.set_configuration_hints(
      "Here is a full configuration example in the      \n"
      "``datatools::properties`` ASCII format::         \n"
      "                                                 \n"
      "  energy.resolution     : real as fraction = 8 %         \n"
      "  energy.low_threshold  : real as energy = 50 keV        \n"
      "  energy.high_threshold : real as energy = 150 keV       \n"
      "  scintillator_relaxation_time : real as time = 6.0 ns   \n"
      "  alpha_quenching_parameters : real[3] = 77.4 0.639 2.34 \n"
      "                                                         \n");

  ocd_.set_validation_support(true);
  ocd_.lock();
  return;
}
DOCD_CLASS_IMPLEMENT_LOAD_END()  // Closing macro for implementation

// Registration macro for class 'snemo::processing::CalorimeterModel' :
DOCD_CLASS_SYSTEM_REGISTRATION(snemo::processing::CalorimeterModel,
                               "snemo::processing::CalorimeterModel")

