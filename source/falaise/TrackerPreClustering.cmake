# TrackerPreClustering specific classes:

list(APPEND FalaiseLibrary_HEADERS
  TrackerPreClustering/interface.h
  TrackerPreClustering/pre_clusterizer.h
  )


list(APPEND FalaiseLibrary_SOURCES
  TrackerPreClustering/interface.cc
  TrackerPreClustering/pre_clusterizer.cc
  )

list(APPEND FalaiseLibrary_TESTS
  TrackerPreClustering/testing/test_trackerpreclustering.cxx
  )
