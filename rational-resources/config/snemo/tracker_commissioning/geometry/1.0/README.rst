======================================================
SuperNEMO tracker commissioning virtual geometry setup
======================================================

:Authors: François Mauger
:Date:    2013-10-18

.. contents::
   :depth: 3
..

Presentation
============

This directory  contains the files  needed to describe  the simplified
geometry  setup  of  the   C0  tracker  sub-module  of  the  SuperNEMO
demonstrator   module.   This   setup  corresponds   to   the  tracker
commissioning phase:

 * Setup label is : ``snemo::tracker_commissioning``
 * Setup version is : ``1.0``

Files:

 * ``README.rst`` : this file,
 * ``manager.conf``   :   the   main   configuration   file   of   the
   Bayeux/geomtools geometry manager object,
 * ``categories.lis``  : the  description of  all geometry  categories
   associated to  physical volumes of  interest in the  geometry setup
   (drift  volumes, wires,  frame beams...);  this allows  to build  a
   dictionnary of geometry IDs to enable automated location of volumes
   and navigation within the geometry model,
 * ``geometry_service.conf`` : Configuration file for the geometry service
   associated to this geometry setup,
 * ``models/`` : the directory that contains files  dedicated   to  the
   building  of   *geometry  models*  (ala Geant4/GDML logical volume factories).

   * ``C0_shape_layers.geom`` : description of the tracking
     layers,
   * ``C0_module.geom`` : description of the C0 module,
   * ``muon_trigger.geom`` : description of the muon trigger
     plates,
   * ``setup.geom``  : description  of  the  full  tracker
     commissioning setup with the world top volume and an experimental
     area that hosts the detector.

 * ``plugins/`` : the directory that contains files that describe
   geometry plugins.

   * ``materials_plugin.conf`` : the material plugin.

Check the geometry:

  1. First make sure the Bayeux software is installed and setup::

      $ which bxquery
      $ bxquery --version
      ...

  2. Then run::

      $ cd {Falaise source directory}
      $ bxgeomtools_inspector \
          --datatools::logging "trace" \
          --datatools::resource-path "falaise@$(pwd)/resources" \
          --manager-config "@falaise:config/snemo/tracker_commissioning/geometry/1.0/manager.conf"

     where:

       * ``--datatools::resource-path "falaise@$(pwd)/resources"``
         registers  the   Falaise  resource  base  directory   in  the
         datatools' kernel for automated search for configuration file
         paths,
       * ``--manager-config "@falaise:config/geometry/snemo/tracker_commissioning/1.0/manager.conf"``
         indicates  the  main  configuration   file  of  the  geometry
         manager.
