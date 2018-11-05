================================================================
Vertex generation for SuperNEMO demonstrator simulation
================================================================

:Authors: François Mauger
:Date:    2016-11-13

.. contents::
   :depth: 3
..


Presentation
============

This directory  contains the files  needed to describe  several vertex
  generators as input for SuperNEMO demonstrator simulation.

 * Version is : ``4.0``

Files
=====

  * ``README.rst`` : This file.
  * ``manager.conf``  :  The main  configuration  file  of the  Bayeux
    ``genbb::manager`` object.
  * ``generators/`` : Directory for vertex generators definitions.
  * ``variants/`` : Directory for variant definitions.

Check the configuration
=======================

First make sure the Bayeux and Falaise software is installed and setup: ::

  $ which bxgenvtx_production
  ...

Browse and edit primary event generation variant parameters and options
===============================================================================

1. First make sure the Bayeux software is installed and setup, particularly the variant support: ::

.. code:: sh

   $ which bxvariant_inspector
   ...
..

2. From the build directory, browse/edit the primary event generation variant:

.. code:: sh

   $ bxvariant_inspector \
          --datatools::resource-path="falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
          --variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
          --variant-gui \
	  --variant-store "myprofile.conf"
..

Print the list of available vertex generators
---------------------------------------------

From  Falaise build  directory (this  is preliminary),  run:

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
   bxgenvtx_production \
	 --logging "fatal" \
	 --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
	 --load-dll Falaise \
	 --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
	 --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
	 --variant-config           "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
	 --variant-gui \
	 --list

Generate 10000 vertexes from the surface of the experimental hall roof
----------------------------------------------------------------------

Run from the Falaise build directory (preliminary):

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "fatal" \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
     --load-dll Falaise \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 10000 \
     --vertex-modulo 100 \
     --output-file "vertices.txt" \
     --variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --variant-gui \
     --variant-store "profile.conf" \
     --vertex-generator "experimental_hall_roof" \
     --visu \
     --visu-spot-zoom 2.0 \
     --visu-spot-color "magenta" \
     --visu-output-file "vertices-visu-dd.data.gz"
..

Generate 10000 vertexes from the internal surface of the shielding walls
---------------------------------------------------------------------------------

Run from the Falaise build directory (preliminary):

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "fatal" \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
     --load-dll Falaise \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 10000 \
     --vertex-modulo 100 \
     --output-file "vertices.txt" \
     --variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --variant-gui \
     --variant-store "profile.conf" \
     --vertex-generator "shielding_all_internal_surfaces" \
     --visu \
     --visu-spot-zoom 2.0 \
     --visu-spot-color "magenta" \
     --visu-output-file "vertices-visu-dd.data.gz"
..


Generate 10000 vertexes from the bulk of the shielding walls
---------------------------------------------------------------------------------

Run from the Falaise build directory (preliminary):

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "fatal" \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
     --load-dll Falaise \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 10000 \
     --vertex-modulo 100 \
     --output-file "vertices.txt" \
     --variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --variant-gui \
     --variant-store "profile.conf" \
     --vertex-generator "shielding_left_right_bulk" \
     --visu \
     --visu-spot-zoom 2.0 \
     --visu-spot-color "magenta" \
     --visu-output-file "vertices-visu-dd.data.gz"
..


Generate 10000 vertexes from the source pads bulk volume
----------------------------------------------------------------------

Run from the Falaise build directory (preliminary):

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "fatal" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-3.0.0/resources" \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 10000 \
     --vertex-modulo 100 \
     --output-file "vertices.txt" \
     --variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --variant-gui \
     --variant-store "profile.conf" \
     --vertex-generator "source_pads_bulk" \
     --visu \
     --visu-object "[1100:0]" \
     --visu-spot-zoom 2.0 \
     --visu-spot-color "magenta" \
     --visu-output-file "vertices-visu-dd.data.gz"
..

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgeomtools_inspector \
     --logging "warning" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-2.1.0/resources" \
     --manager-config           "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --datatools::variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --datatools::variant-load "profile.conf"
   geomtools> ldd vtx vertices-visu-dd.data.gz
   geomtools> G --with-category source_submodule
   List of available GIDs :
   [1100:0] as 'source_submodule'
   geomtools> display -yz [1100:0]
   ...
   geomtools> q
..

Vertex generator from calibration source with basic layout:

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "warning" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-2.1.0/resources" \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 10000 \
     --vertex-modulo 500 \
     --datatools::variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --datatools::variant-set "geometry:layout=Basic" \
     --datatools::variant-set "geometry:layout/if_basic/source_calibration=true" \
     --datatools::variant-qt-gui \
     --datatools::variant-store "calib_profile.rep" \
     --vertex-generator "source_calibration_all_spots" \
     --output-file "calib_vertices.txt" \
     --visu \
     --visu-spot-zoom 2.0 \
     --visu-spot-size "0.05 mm" \
     --visu-spot-color "red" \
     --visu-output-file "calib_vertices-visu-dd.data.gz"
..

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgeomtools_inspector \
     --logging "warning" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-2.1.0/resources" \
     --manager-config           "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --datatools::variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --datatools::variant-load "calib_profile.rep"
   geomtools> ldd vtx calib_vertices-visu-dd.data.gz
   geomtools> G --with-category source_submodule
   List of available GIDs :
   [1100:0] as 'source_submodule'
   geomtools> display -yz [1100:0]
..


Vertex generator with half-commissioning layout:

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgenvtx_production \
     --logging "warning" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-2.1.0/resources" \
     --datatools::variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --datatools::variant-set "geometry:layout=HalfCommissioning" \
     --datatools::variant-set "vertexes.commissioning:type=SingleSlot" \
     --datatools::variant-set "vertexes.commissioning:type/if_single_slot/column=48" \
     --datatools::variant-set "vertexes.commissioning:type/if_single_slot/row=1" \
     --datatools::variant-qt-gui \
     --datatools::variant-store "hc_profile.rep" \
     --geometry-manager         "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --vertex-generator-manager "@falaise:snemo/demonstrator/simulation/vertexes/4.0/manager.conf" \
     --shoot \
     --prng-seed 314159 \
     --number-of-vertices 100 \
     --vertex-modulo 20 \
     --output-file "hc_vertices.txt" \
     --vertex-generator "commissioning.single_spot" \
     --visu-spot-zoom 2.0 \
     --visu-spot-size "0.05 mm" \
     --visu-spot-color "red" \
     --visu-output-file "hc_vertices-visu-dd.data.gz"
..


Other available generator in half-commissioning layout:

.. raw:: sh

     --vertex-generator "commissioning.all_spots"
..

.. raw:: sh

   $ LD_LIBRARY_PATH="$(pwd)/BuildProducts/lib:${LD_LIBRARY_PATH}" \
     bxgeomtools_inspector \
     --logging "warning" \
     --load-dll Falaise \
     --datatools::resource-path "falaise@$(pwd)/BuildProducts/share/Falaise-2.1.0/resources" \
     --manager-config           "@falaise:snemo/demonstrator/geometry/manager.conf" \
     --datatools::variant-config "@falaise:snemo/demonstrator/simulation/vertexes/4.0/variants/repository.conf" \
     --datatools::variant-load "hc_profile.rep"
   geomtools> ldd vtx hc_vertices-visu-dd.data.gz
   geomtools> G --with-category commissioning_source_plane
   List of available GIDs :
   [1500:0] as 'source_submodule'
   geomtools> display -yz [1500:0]
..


Prepare list of supported vertex generator for the variant system
----------------------------------------------------------------------

Extract the list of supported vertex generator from definition files (``generators/*.lis``)
and store it in the ``variants/models/vertexes_generators.csv`` file:

.. raw: sh

   $ ./tools/_prepare_csv.sh
   $ ls variants/models/vertexes_generators.csv
..

Print the number of available generators:
.. raw: sh

   $ wc -l variants/models/vertexes_generators.csv
..

Print the list of groups of vertex generators:
.. raw: sh

   $ cat variants/models/vertexes_generators.csv | cut -d ':' -f3 | sort | uniq
..


.. END.
