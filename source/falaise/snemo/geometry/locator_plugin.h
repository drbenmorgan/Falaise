// -*- mode: c++ ; -*-
/** \file falaise/snemo/geometry/locator_plugin.h */
/* Author (s) :   Xavier Garrido <garrido@lal.in2p3.fr>
 * Creation date: 2013-07-15
 * Last modified: 2014-01-28
 *
 * License:
 *
 * Description:
 *
 *   A geometry manager plugin with embeded locators.
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_GEOMETRY_LOCATOR_PLUGIN_H
#define FALAISE_SNEMO_GEOMETRY_LOCATOR_PLUGIN_H 1

// Third party:
// - Boost :
#include <boost/cstdint.hpp>
// - Bayeux/geomtools
#include <geomtools/manager.h>
#include <geomtools/manager_macros.h>

namespace geomtools {
  class i_base_locator;
}

namespace snemo {

  namespace geometry {

    /// \brief A geometry manager plugin with embedded SuperNEMO locators.
    class locator_plugin : public geomtools::manager::base_plugin
    {
    public:

      typedef datatools::handle<geomtools::base_locator> locator_handle_type;

      struct locator_entry_type
      {
        std::string label;
        std::string category_name;
        int         category_type;
        uint32_t    status;
        locator_handle_type locator_handle;
      };

      typedef std::map<std::string, locator_entry_type> locator_dict_type;

      /// Default constructor
      locator_plugin ();

      /// Destructor
      virtual ~locator_plugin ();

      /// Main plugin initialization method
      virtual int initialize(const datatools::properties & config_,
                             const geomtools::manager::plugins_dict_type & plugins_,
                             const datatools::service_dict_type & services_);

      /// Plugin reset method
      virtual int reset ();

      /// Check if plugin is initialized
      virtual bool is_initialized () const;

      /// Returns a non-mutable reference to the dictionary of locators
      const locator_dict_type & get_locators () const;

      /// Returns a mutable reference to the dictionary of locators
      locator_dict_type & grab_locators ();

      /// Returns a non-mutable reference to the geiger locator
      const geomtools::base_locator & get_gg_locator () const;

      /// Returns a non-mutable reference to the main wall locator
      const geomtools::base_locator & get_calo_locator () const;

      /// Returns a non-mutable reference to the X wall locator
      const geomtools::base_locator & get_xcalo_locator () const;

      /// Returns a non-mutable reference to the gamma veto locator
      const geomtools::base_locator & get_gveto_locator () const;

     protected:

      /// Internal mapping build method
      void _build_locators (const datatools::properties & config_);

    private:

      bool                _initialized_;   //!< Initialization flag
      locator_dict_type   _locators_;      //!< Locator dictionary
      locator_handle_type _gg_locator_;    //!< Handle to Geiger locator
      locator_handle_type _calo_locator_;  //!< Handle to main wall locator
      locator_handle_type _xcalo_locator_; //!< Handle to X-wall locator
      locator_handle_type _gveto_locator_; //!< Handle to gamma-veto locator

      GEOMTOOLS_PLUGIN_REGISTRATION_INTERFACE(locator_plugin);

    };

  } // end of namespace geometry

} // end of namespace snemo

#endif // FALAISE_SNEMO_GEOMETRY_LOCATOR_PLUGIN_H
