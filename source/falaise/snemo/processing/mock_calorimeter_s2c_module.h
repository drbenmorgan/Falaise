// -*- mode: c++ ; -*-
/** \file falaise/snemo/processing/mock_calorimeter_s2c_module.h
 * Author(s) :    Francois Mauger <mauger@lpccaen.in2p3.fr>
 * Creation date: 2011-01-12
 * Last modified: 2014-02-27
 *
 * License:
 *
 * Description:
 *
 *   Mock simulation data processor for calorimeter MC hits
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H
#define FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H 1

// Standard library:
#include <map>
#include <string>
#include <vector>

// Third party:
// - Bayeux/mygsl:
#include <mygsl/rng.h>
// - Bayeux/dpp:
#include <dpp/base_module.h>
// - CLHEP
#include <CLHEP/Units/SystemOfUnits.h>

// This project :
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <falaise/snemo/processing/calorimeter_regime.h>

namespace geomtools {
class manager;
}

namespace mctools {
class simulated_data;
}

namespace snemo {

namespace processing {

/// \brief A mock calibration for SuperNEMO calorimeter hits
class mock_calorimeter_s2c_module : public dpp::base_module {
 public:

  /// Initialization
  virtual void initialize(const datatools::properties& setup_,
                          datatools::service_manager& service_manager_,
                          dpp::module_handle_dict_type& module_dict_);

  /// Reset
  virtual void reset();

  /// Data record processing
  virtual process_status process(datatools::things& data_);


 private:
  /// Digitize calorimeter hits
  void _digitizeHits(
      const mctools::simulated_data& simulated_data_,
      snemo::datamodel::calibrated_data::calorimeter_hit_collection_type&
          calibrated_calorimeter_hits_);

  /// Calibrate calorimeter hits (energy/time resolution spread)
  void _calibrateHits(
      snemo::datamodel::calibrated_data::calorimeter_hit_collection_type& calorimeter_hits_);

  /// Apply basic trigger filter
  void _triggerHits(
      snemo::datamodel::calibrated_data::calorimeter_hit_collection_type& calorimeter_hits_);

  /// Main process function
  void _process(const mctools::simulated_data& simulated_data_,
                snemo::datamodel::calibrated_data::calorimeter_hit_collection_type&
                    calibrated_calorimeter_hits_);

 private:
  mygsl::rng RNG_ {};                                //!< PRN generator
  std::vector<std::string> caloTypes {};          //!< Calorimeter hit categories
  typedef std::map<std::string, CalorimeterModel> CaloModelMap;
  CaloModelMap caloModels {};  //!< Calorimeter regime tools
  std::string sdInputTag {};                             //!< The label of the simulated data bank
  std::string cdOutputTag {};                             //!< The label of the calibrated data bank
  double timeWindow {100.*CLHEP::ns};                        //!< Time width of a calo cluster
  bool quenchAlphas {true};                             //!< Flag to (dis)activate the alpha quenching
  bool assocMCHitId {false};                             //!< The flag to reference MC true hit

  // Macro to automate the registration of the module :
  DPP_MODULE_REGISTRATION_INTERFACE(mock_calorimeter_s2c_module)
};

}  // end of namespace processing

}  // end of namespace snemo

/***************************
 * OCD support : interface *
 ***************************/

#include <datatools/ocd_macros.h>

// @arg snemo::processing::mock_calorimeter_s2c_module the name the registered class
DOCD_CLASS_DECLARATION(snemo::processing::mock_calorimeter_s2c_module)

#endif  // FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H

// end of falaise/snemo/processing/mock_calorimeter_s2c_module.h
