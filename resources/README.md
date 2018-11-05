Falaise Resource Files
======================

This directory contains the tree of configuration files for simualtion
and reconstruction. It is organized as follows:

genbb
-----

Definitions of Bayeux/GenBB event generators. These only describe
the particles/kinematics rather than vertices. Definitions are
organized into files based on an overall category such as Double Beta,
Backgrouns, Calibration and Generic.

It also provides generators than can be modified using the Bayeux
"variants" system.


geometry
--------

"Common" Bayeux/geomtools models. Only has PMT and tracker cells in
at the moment. See also snemo/demonstrator/geometry.


materials
---------

"Common" Bayeux/materials definitions for use in geometry/simulation.


snemo/demonstrator
------------------

Geometry, reconstruction and simulation for the SuperNEMO Demonstrator
detector.

- snemo/demonstrator/geometry
  - full Bayeux/geomtools model for the detector
- snemo/demonstrator/reconstruction
  - current flreconstruct pipelines for reconstruction
- snemo/demonstrator/simulation/vertexes
  - Bayeux/genvtx definitions for vertex generation in the detector
  - Effectively coupled to snemo/demonstrator/geometry
- snemo/demonstrator/simulation/geant4_control
  - Geant4 configuration for the detector, used in flsimulate
  - Uses geometry, vertexes, genbb and materials either directly
    or via other clients using them.

urn
---
Indirection mechanism for resource files. To be reviewed.

