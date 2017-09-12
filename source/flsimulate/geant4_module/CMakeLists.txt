# - Build Falaise's Geant4 simulation module

# Need Boost thread....
find_package(Boost 1.60 REQUIRED system thread)

# - Find G4, set up data libraries
find_package(Geant4 REQUIRED ui_all vis_all gdml NO_MODULE)
if(Geant4_DATASETS)
  set(MCTOOLS_G4_DATA_ASSIGNMENTS)
  foreach(_ds ${Geant4_DATASETS})
    list(APPEND MCTOOLS_G4_DATA_ASSIGNMENTS "(\"${Geant4_DATASET_${_ds}_ENVVAR}\", \"${Geant4_DATASET_${_ds}_PATH}\")")
  endforeach()
  string(REPLACE ";" "" MCTOOLS_G4_DATA_ASSIGNMENTS ${MCTOOLS_G4_DATA_ASSIGNMENTS})
endif()

configure_file(data_libraries.h.in data_libraries.h @ONLY)

add_library(falaise_geant4_module SHARED
  data_libraries.h
  base_physics_constructor.cc
  base_physics_constructor.h
  biasing_manager.cc
  biasing_manager.h
  detector_construction.cc
  detector_construction.h
  electromagnetic_field.cc
  electromagnetic_field.h
  em_field_equation_of_motion.cc
  em_field_equation_of_motion.h
  em_field_g4_stuff.cc
  em_field_g4_stuff.h
  em_field_g4_utils.h
  em_physics_constructor.cc
  em_physics_constructor.h
  event_action.cc
  event_action.h
  g4_prng.cc
  g4_prng.h
  loggable_support.cc
  loggable_support.h
  magnetic_field.cc
  magnetic_field.h
  manager.cc
  manager.h
  manager_parameters.cc
  manager_parameters.h
  neutrons_physics_constructor.cc
  neutrons_physics_constructor.h
  particles_physics_constructor.cc
  particles_physics_constructor.h
  physics_list.cc
  physics_list.h
  physics_list_utils.cc
  physics_list_utils.h
  primary_generator.cc
  primary_generator.h
  region_tools.cc
  region_tools.h
  run_action.cc
  run_action.h
  sensitive_detector.cc
  sensitive_detector.h
  sensitive_hit.cc
  sensitive_hit.h
  sensitive_hit_collection.cc
  sensitive_hit_collection.h
  simulation_ctrl.cc
  simulation_ctrl.h
  simulation_module.cc
  simulation_module.h
  stacking_action.cc
  stacking_action.h
  stepping_action.cc
  stepping_action.h
  stepping_verbose.cc
  stepping_verbose.h
  track_history.cc
  track_history.h
  tracking_action.cc
  tracking_action.h
  processes/em_extra_models.cc
  processes/em_extra_models.h
  processes/em_model_factory.cc
  processes/em_model_factory.h
  processes/utils.cc
  processes/utils.h
  )

target_include_directories(falaise_geant4_module
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${Geant4_INCLUDE_DIRS}
  )
target_link_libraries(falaise_geant4_module
  PUBLIC Bayeux::Bayeux ${Geant4_LIBRARIES}
  PRIVATE Boost::thread
  )


