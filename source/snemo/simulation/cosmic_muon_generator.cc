// falaise/snemo/simulation/cosmic_muon_generator.cc

// Ourselves
#include <falaise/snemo/simulation/cosmic_muon_generator.h>

// Standard library:
#include <stdexcept>

// Third party:
// - Bayeux/datatools:
#include <datatools/exception.h>
#include <datatools/logger.h>
#include <datatools/properties.h>
#include <datatools/units.h>
#include <datatools/utils.h>
// - Bayeux/mygsl:
#include <mygsl/histogram.h>
#include <mygsl/i_unary_function.h>
#include <mygsl/tabulated_function.h>
#include <mygsl/von_neumann_method.h>
// - Bayeux/genbb_help:
#include <genbb_help/primary_event.h>
#include <genbb_help/single_particle_generator.h>

namespace snemo {

namespace simulation {

GENBB_PG_REGISTRATION_IMPLEMENT(cosmic_muon_generator, "snemo::simulation::cosmic_muon_generator")

/* sea_level_toy_theta_density_function */

struct sea_level_toy_theta_density_function : public mygsl::i_unary_function {
 protected:
  double _eval(double x_) const override;
};

double sea_level_toy_theta_density_function::_eval(double x_) const {
  double c = std::cos(x_);
  return c * c;
}

/* cosmic_muon_generator::sea_level_toy_setup */

bool cosmic_muon_generator::can_external_random() const { return true; }

void cosmic_muon_generator::sea_level_toy_setup::reset() {
  if (angular_VNM != nullptr) {
    delete angular_VNM;
    angular_VNM = nullptr;
  }
  if (theta_density_function != nullptr) {
    delete theta_density_function;
    theta_density_function = nullptr;
  }
  set_defaults();
}

void cosmic_muon_generator::sea_level_toy_setup::set_defaults() {
  energy_mean = 4.0 * CLHEP::GeV;
  energy_sigma = 1.0 * CLHEP::GeV;
  maximum_theta = 70. * CLHEP::degree;
  muon_ratio = 1.2;
  theta_density_function = nullptr;
  angular_VNM = nullptr;
}

cosmic_muon_generator::sea_level_toy_setup::sea_level_toy_setup() { set_defaults(); }

/* cosmic_muon_generator::sea_level_pdg_setup */

void cosmic_muon_generator::sea_level_pdg_setup::set_defaults() {
  /*
   *
   *
   */
}

void cosmic_muon_generator::sea_level_pdg_setup::reset() { set_defaults(); }

cosmic_muon_generator::sea_level_pdg_setup::sea_level_pdg_setup() { set_defaults(); }

/* cosmic_muon_generator::underground_setup */

void cosmic_muon_generator::underground_setup::set_defaults() {
  underground_lab = "lsm";
  underground_depth = 4800.0;
}

void cosmic_muon_generator::underground_setup::reset() { set_defaults(); }

cosmic_muon_generator::underground_setup::underground_setup() { set_defaults(); }

/*** cosmic_muon_generator ***/

bool cosmic_muon_generator::is_initialized() const { return _initialized_; }

int cosmic_muon_generator::get_mode() const { return _mode_; }

void cosmic_muon_generator::set_mode(int new_value_) {
  DT_THROW_IF(is_initialized(), std::logic_error, "Generator is already initialized !");
  _mode_ = new_value_;
}

cosmic_muon_generator::cosmic_muon_generator() {
  _initialized_ = false;

  _at_reset_();

  _seed_ = 0;
}

cosmic_muon_generator::~cosmic_muon_generator() {
  if (_initialized_) {
    reset();
  }
}

const mygsl::rng& cosmic_muon_generator::get_random() const {
  if (has_external_random()) {
    return get_external_random();
  }
  return _random_;
}

mygsl::rng& cosmic_muon_generator::grab_random() {
  if (has_external_random()) {
    return grab_external_random();
  }
  return _random_;
}

void cosmic_muon_generator::_at_reset_() {
  _mode_ = MODE_INVALID;

  _sea_level_mode_ = SEA_LEVEL_INVALID;
  _sea_level_toy_setup_.reset();
  _sea_level_pdg_setup_.reset();
  _underground_setup_.reset();
  if (_random_.is_initialized()) {
    _random_.reset();
  }
  _seed_ = 0;
}

void cosmic_muon_generator::reset() {
  if (!_initialized_) {
    return;
  }
  _initialized_ = false;
  _at_reset_();
}

void cosmic_muon_generator::initialize(const datatools::properties& dps,
                                       datatools::service_manager& /* unused */,
                                       genbb::detail::pg_dict_type& /* unused */) {
  DT_THROW_IF(_initialized_, std::logic_error,
              "Operation prohibited ! Object is already initialized !");

  _initialize_base(dps);

  if (!has_external_random()) {
    DT_THROW_IF(!dps.has_key("seed"), std::logic_error,
                "Missing 'seed' property for particle generator '" << get_name() << "' !");
    long seed = dps.fetch_integer("seed");
    DT_THROW_IF(seed < 0, std::logic_error,
                "Invalid seed value (>=0) for particle generator '" << get_name() << "' !");
    _seed_ = seed;
  }

  if (dps.has_key("mode")) {
    std::string mode_str = dps.fetch_string("mode");
    if (mode_str == "sea_level") {
      set_mode(MODE_SEA_LEVEL);
    } else if (mode_str == "underground") {
      set_mode(MODE_UNDERGROUND);
    }
  } else {
    DT_THROW_IF(true, std::logic_error,
                "Missing 'mode' property for particle generator '" << get_name() << "' !");
  }

  if (_mode_ == MODE_SEA_LEVEL) {
    DT_THROW_IF(
        !dps.has_key("sea_level.mode"), std::logic_error,
        "Missing 'sea_level.mode' property  for particle generator '" << get_name() << "' !");
    std::string mode_str = dps.fetch_string("sea_level.mode");
    if (mode_str == "toy") {
      _sea_level_mode_ = SEA_LEVEL_TOY;
    } else if (mode_str == "pdg") {
      _sea_level_mode_ = SEA_LEVEL_PDG;
    } else {
      DT_THROW_IF(
          true, std::logic_error,
          "Invalid 'see level mode' property for particle generator '" << get_name() << "' !");
    }

    if (_sea_level_mode_ == SEA_LEVEL_TOY) {
      if (dps.has_key("sea_level_toy.energy_mean")) {
        _sea_level_toy_setup_.energy_mean =
            dps.fetch_real_with_explicit_dimension("sea_level_toy.energy_mean", "energy");
      }
      if (dps.has_key("sea_level_toy.energy_sigma")) {
        _sea_level_toy_setup_.energy_sigma =
            dps.fetch_real_with_explicit_dimension("sea_level_toy.energy_sigma", "energy");
      }
      if (dps.has_key("sea_level_toy.maximum_theta")) {
        _sea_level_toy_setup_.maximum_theta =
            dps.fetch_real_with_explicit_dimension("sea_level_toy.maximum_theta", "angle");
        DT_THROW_IF(_sea_level_toy_setup_.maximum_theta < 0.0 ||
                        _sea_level_toy_setup_.maximum_theta > 90.0 * CLHEP::degree,
                    std::range_error,
                    "Invalid 'sea_level_toy.maximum_theta' value for particle generator '"
                        << get_name() << "' !");
      }
      if (dps.has_key("sea_level_toy.muon_ratio")) {
        _sea_level_toy_setup_.muon_ratio = dps.fetch_real("sea_level_toy.muon_ratio");
        DT_THROW_IF(_sea_level_toy_setup_.muon_ratio < 0.0, std::logic_error,
                    "Invalid 'sea_level_toy.muon_ratio' value for particle generator '"
                        << get_name() << "'!");
      }
    } else if (_sea_level_mode_ == SEA_LEVEL_PDG) {
      DT_THROW_IF(true, std::logic_error, "The 'see level PDG mode' is not implemented yet !");
    }
  } else if (_mode_ == MODE_UNDERGROUND) {
    DT_THROW_IF(true, std::logic_error, "The 'underground mode' is not implemented yet !");
  } else {
    DT_THROW_IF(true, std::logic_error, "Invalid mode !");
  }

  _at_init_();

  _initialized_ = true;
}

void cosmic_muon_generator::_at_init_() {
  // Initialize the PRNG :
  if (!has_external_random()) {
    _random_.init("taus2", _seed_);
  }

  if (_mode_ == MODE_SEA_LEVEL) {
    if (_sea_level_mode_ == SEA_LEVEL_TOY) {
      _sea_level_toy_setup_.theta_density_function = new sea_level_toy_theta_density_function;
      _sea_level_toy_setup_.angular_VNM = new mygsl::von_neumann_method(
          0.0, _sea_level_toy_setup_.maximum_theta, *_sea_level_toy_setup_.theta_density_function,
          mygsl::von_neumann_method::AUTO_FMAX, 100, 100);
    }
  }
}

bool cosmic_muon_generator::has_next() { return true; }

void cosmic_muon_generator::_load_next(::genbb::primary_event& event_,
                                       bool compute_classification_) {
  DT_LOG_DEBUG(get_logging_priority(), "Entering...");
  DT_THROW_IF(!_initialized_, std::logic_error, "Generator is not locked/initialized !");
  event_.reset();

  double muon_mass = ::genbb::single_particle_generator::get_particle_mass_from_label("mu+");
  int muon_type = ::genbb::primary_particle::MUON_PLUS;

  if (_mode_ == MODE_SEA_LEVEL) {
    double px(0.0), py(0.0), pz(0.0);
    if (_sea_level_mode_ == SEA_LEVEL_TOY) {
      // randomize kinetic energy :
      double kinetic_energy = grab_random().gaussian(_sea_level_toy_setup_.energy_mean,
                                                     _sea_level_toy_setup_.energy_sigma);
      if (kinetic_energy < 0.0) {
        kinetic_energy = std::max(
            0.0, _sea_level_toy_setup_.energy_mean - 2. * _sea_level_toy_setup_.energy_sigma);
      }

      // shoot "mu+" or "mu-" :
      if (_sea_level_toy_setup_.muon_ratio < 1.0e-3) {
        // only "mu-"
        muon_type = ::genbb::primary_particle::MUON_MINUS;
      } else if (_sea_level_toy_setup_.muon_ratio < 1.0e+3) {
        // a mix "mu+/mu-"
        double prob_muon_minus = 1. / (_sea_level_toy_setup_.muon_ratio + 1);
        if (grab_random().uniform() < prob_muon_minus) {
          muon_type = ::genbb::primary_particle::MUON_MINUS;
        }
      } else {
        // default: only "mu+"
      }

      double momentum = std::sqrt(kinetic_energy * (kinetic_energy + 2 * muon_mass));
      double phi = grab_random().flat(0., 2. * M_PI);
      double theta = M_PI - _sea_level_toy_setup_.angular_VNM->shoot(grab_random());
      px = momentum * std::sin(theta) * std::cos(phi);
      py = momentum * std::sin(theta) * std::sin(phi);
      pz = momentum * std::cos(theta);
    } else if (_sea_level_mode_ == SEA_LEVEL_PDG) {
      DT_THROW_IF(true, std::logic_error, "'SEA_LEVEL_PDG' is not implemented yet !");
    }

    event_.set_time(0.0 * CLHEP::second);
    ::genbb::primary_particle pp;
    pp.set_type(muon_type);
    pp.set_time(0.0 * CLHEP::second);
    geomtools::vector_3d p(px, py, pz);
    pp.set_momentum(p);
    event_.add_particle(pp);
  }

  if (compute_classification_) {
    event_.compute_classification();
  }

  DT_LOG_DEBUG(get_logging_priority(), "Exiting.");
}

double cosmic_muon_generator::energy_spectrum_at_sea_level_HE(double muon_cos_theta,
                                                              double muon_energy) {
  double Emu = muon_energy / CLHEP::GeV;
  double ct = muon_cos_theta;
  double dnde = 0.14 * std::pow(Emu, -2.7) *
                (1.000 / (1. + (1.1 * Emu * ct / 115.)) + 0.054 / (1. + (1.1 * Emu * ct / 850.)));
  return dnde;
}

}  // end of namespace simulation

}  // end of namespace snemo
