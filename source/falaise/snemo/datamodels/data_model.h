/** \file falaise/snemo/datamodels/data_model.h */
// -*- mode: c++ ; -*-
/* Author(s) :    Francois Mauger <mauger@lpccaen.in2p3.fr>
 * Creation date: 2011-03-21
 * Last modified: 2014-01-28
 *
 * Description:
 *
 *   SuperNEMO data model
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_DATAMODEL_DATA_MODEL_H
#define FALAISE_SNEMO_DATAMODEL_DATA_MODEL_H 1

// Standard library
#include <string>

// Third party
// - Bayeux/datatools
#include <datatools/things.h>

namespace snemo {

    namespace datamodel {

      class data_info
      {
      public:

        /// Default string label/name for the 'event record'
        static const std::string EVENT_RECORD_LABEL;

        // Data bank standard labels/names :

        /// Default string label/name for the 'event header'
        static const std::string EVENT_HEADER_LABEL;

        /// Default string label/name for the 'simulated data'
        static const std::string SIMULATED_DATA_LABEL;

        /// Default string label/name for the 'raw data'
        static const std::string RAW_DATA_LABEL;

        /// Default string label/name for the 'unified data'
        static const std::string UNIFIED_DATA_LABEL;

        /// Default string label/name for the 'calibrated data'
        static const std::string CALIBRATED_DATA_LABEL;

        /// Default string label/name for the 'tracker clustering data'
        static const std::string TRACKER_CLUSTERING_DATA_LABEL;

        /// Default string label/name for the 'tracker trajectory data'
        static const std::string TRACKER_TRAJECTORY_DATA_LABEL;

      };

      /// Alias for the event record container type
      typedef datatools::things event_record;

    } // end of namespace datamodel

} // end of namespace snemo

#endif // FALAISE_SNEMO_DATAMODEL_DATA_MODEL_H

// end of falaise/snemo/datamodels/data_model.h