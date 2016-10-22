//! \file    PropertyReader.h
//! \brief   Convienience functions for reading datatools::properties
//! \details The interface provided by datatools::properties often requires
//!          significant boilerplate code to check for and extract values
//!          associated with keys. The functions here help to minimize this
//!          and assist in binding a struct to a properties definition
//
// Copyright (c) 2016 by Ben Morgan <Ben.Morgan@warwick.ac.uk>
// Copyright (c) 2016 by The University of Warwick
//
// This file is part of Falaise.
//
// Falaise is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Falaise is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Falaise.  If not, see <http://www.gnu.org/licenses/>.

#ifndef FALAISE_PROPERTYREADER_HH
#define FALAISE_PROPERTYREADER_HH

// Standard Library
#include <exception>
#include <vector>

// Third Party
// - Bayeux
#include <bayeux/datatools/properties.h>

// - Boost
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/utility/enable_if.hpp>

// This Project

namespace falaise {
namespace Properties {
// Exceptions
typedef std::logic_error WrongType;
typedef std::logic_error MissingKey;

//! List of types that can be stored in and read from datatools::properties
typedef boost::mpl::vector<int,
                           double,
                           bool,
                           //std::string,
                           std::vector<int> //,
                           //std::vector<double>
                           //std::vector<bool>
                           //std::vector<std::string>
                           //Path (to allow proper checking of "strings as
                           //paths"
                           //Quantity/Length/etc (to check reals with required unit,
                           //needs a bit of thought for behaviour, as units are
                           //converted, can probably just check dimension)
                               > AllowedTypes;

//! Functions for visiting a property item and checking its type is valid
namespace TypeCheckVisitor {
namespace detail {
bool visit_impl(const datatools::properties& p, const std::string& key, int) {
  return p.is_integer(key) && p.is_scalar(key);
}

bool visit_impl(const datatools::properties& p, const std::string& key, double) {
  return p.is_real(key) && p.is_scalar(key);
}

bool visit_impl(const datatools::properties& p, const std::string& key, bool) {
  return p.is_boolean(key) && p.is_scalar(key);
}

bool visit_impl(const datatools::properties& p, const std::string& key, std::vector<int>) {
  return p.is_integer(key) && p.is_vector(key);
}
}

// Compile-time fail on any other type (needs improvement, but does the job
// for now (Look also at enable_if on return type (could use mpl vector of
// allowed types))
template <typename T>
bool visit(const datatools::properties& p, const std::string& key) {
  // Static assert form - gives slightly clearer message...
  static_assert(boost::mpl::contains<AllowedTypes,T>::type::value, "Unsupported type for visitation");
  return detail::visit_impl(p, key, T {});
}
}

// Units are converted automatically, but may want to assert that the given
// key has a unit and that this is of specific dimension
// The same for interpreting strings as paths...

// This should throw on missing key or wrong type
template <typename T>
T getRequiredValue(const datatools::properties& p, const std::string& key) {
  if (!p.has_key(key)) {
    throw Properties::MissingKey("No key '"+key+"'");
  }
  if (!TypeCheckVisitor::visit<T>(p,key)) {
    throw Properties::WrongType("Key '"+key+"' has incorrect type");
  }

  T tmp;
  p.fetch(key, tmp);
  return tmp;
}

// This should throw when key is set, but has the wrong type
// Also consider rvalue for default.
template <typename T>
T getValueOrDefault(const datatools::properties& p, const std::string& key, T defaultValue) {
  T tmp {defaultValue};

  if (p.has_key(key)) {
    if(!TypeCheckVisitor::visit<T>(p,key)) {
      throw Properties::WrongType("Key '"+key+"' is set but has incorrect type");
    }
    p.fetch(key, tmp);
  }

  return tmp;
}

} // namespace Properties
} // namespace falaise


#endif // FALAISE_PROPERTYREADER_HH

