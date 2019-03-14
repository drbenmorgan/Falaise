// -*- mode: c++ ; -*-
/** \file falaise/snemo/processing/mock_tracker_s2c_module.h
 * Author(s) :    Francois Mauger <mauger@lpccaen.in2p3.fr>
 * Creation date: 2011-01-09
 * Last modified: 2014-02-27
 *
 * License:
 *
 * Description:
 *
 *   Mock simulation data processor for tracker MC hits
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_PROCESSING_MOCK_TRACKER_S2C_MODULE_H
#define FALAISE_SNEMO_PROCESSING_MOCK_TRACKER_S2C_MODULE_H 1

// Standard library:
#include <map>
#include <string>
#include <vector>

// Third party:
// - Bayeux/mygsl:
#include <mygsl/rng.h>
// - Bayeux/dpp:
#include <dpp/base_module.h>

// This project :
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <falaise/snemo/datamodels/mock_raw_tracker_hit.h>
#include <falaise/snemo/processing/geiger_regime.h>

namespace geomtools {
class manager;
}

namespace mctools {
class simulated_data;
}

namespace snemo {

namespace processing {

/// \brief Simple modelling of the time and space measurement with the SuperNEMO drift cells in
/// Geiger mode
class mock_tracker_s2c_module : public dpp::base_module {
 public:
  /// Collection of raw tracker hit Intermediate :
  typedef std::list<snemo::datamodel::mock_raw_tracker_hit> raw_tracker_hit_col_type;

  // Because dpp::base_module is insane
  virtual ~mock_tracker_s2c_module() {this->reset();}

  /// Return the drift time threshold for peripheral Geiger hits (far from the anode wire)
  double get_peripheral_drift_time_threshold() const;

  /// Return the drift time threshold for delayed Geiger hits
  double get_delayed_drift_time_threshold() const;

  /// Initialization
  virtual void initialize(const datatools::properties& setup_,
                          datatools::service_manager& service_manager_,
                          dpp::module_handle_dict_type& module_dict_);

  /// Reset
  virtual void reset();

  /// Data record processing
  virtual process_status process(datatools::things& data_);

 protected:
  /// Digitize tracker hits
  void _digitizeHits(const mctools::simulated_data& simulated_data_,
                                     raw_tracker_hit_col_type& raw_tracker_hits_);

  /// Calibrate tracker hits (longitudinal and transverse spread)
  void _calibrateHits(
      const raw_tracker_hit_col_type& raw_tracker_hits_,
      snemo::datamodel::calibrated_data::tracker_hit_collection_type& calibrated_tracker_hits_);

  /// Main process function
  void _process(
      const mctools::simulated_data& simulated_data_,
      snemo::datamodel::calibrated_data::tracker_hit_collection_type& calibrated_tracker_hits_);

 private:
  std::string geoServiceTag {};                   //!< The label of the geometry service
  const geomtools::manager* geoManager {nullptr};  //!< The geometry manager
  std::string _module_category_ {};             //!< The geometry category of the SuperNEMO module
  std::string _hit_category_ {};                //!< The category of the input Geiger hits
  geiger_regime _geiger_ {};                    //!< Geiger regime tools
  mygsl::rng RNG_ {};                       //!< internal PRN generator
  double _peripheral_drift_time_threshold_ {datatools::invalid_real_double()};  //!< Peripheral drift time threshold
  double _delayed_drift_time_threshold_ {datatools::invalid_real_double()};     //!< Delayed drift time threshold
  std::string sdInputTag {};                    //!< The label of the simulated data bank
  std::string cdOutputTag {};                    //!< The label of the calibrated data bank
  bool _store_mc_hit_id_ {false};                    //!< Flag to store the MC true hit ID
  bool _store_mc_truth_track_ids_ {false};  //!< The flag to reference the MC engine track and parent track
                                    //!< IDs associated to this calibrated Geiger hit

  // Macro to automate the registration of the module :
  DPP_MODULE_REGISTRATION_INTERFACE(mock_tracker_s2c_module)
};

}  // end of namespace processing

}  // end of namespace snemo

/***************************
 * OCD support : interface *
 ***************************/

#include <datatools/ocd_macros.h>

// @arg snemo::processing::mock_tracker_s2c_module the name the registered class
DOCD_CLASS_DECLARATION(snemo::processing::mock_tracker_s2c_module)

#endif  // FALAISE_SNEMO_PROCESSING_MOCK_TRACKER_S2C_MODULE_H

// end of falaise/snemo/processing/mock_tracker_s2c_module.h
