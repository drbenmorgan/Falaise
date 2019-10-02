// -*- mode: c++ ; -*-
/// \file falaise/snemo/datamodels/particle_track.ipp

#ifndef FALAISE_SNEMO_DATAMODELS_PARTICLE_TRACK_IPP
#define FALAISE_SNEMO_DATAMODELS_PARTICLE_TRACK_IPP 1

// Ourselves:
#include <falaise/snemo/datamodels/particle_track.h>

// Third party:
// - Boost:
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
// - Bayeux/geomtools:
#include <geomtools/blur_spot.ipp>

// This project:
#include <falaise/snemo/datamodels/boost_io/tracker_trajectory.ipp>

namespace snemo {

namespace datamodel {

template <class Archive>
void particle_track::serialize(Archive& ar_, const unsigned int /*version_*/) {
  ar_& BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_hit);
  ar_& boost::serialization::make_nvp("charge_from_source", charge_from_source_);
  ar_& boost::serialization::make_nvp("trajectory", trajectory_);
  ar_& boost::serialization::make_nvp("vertices", vertices_);
  ar_& boost::serialization::make_nvp("associated_calorimeter_hits", associated_calorimeters_);
}

}  // end of namespace datamodel

}  // end of namespace snemo

#endif  // FALAISE_SNEMO_DATAMODELS_PARTICLE_TRACK_IPP
