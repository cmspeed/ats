# -*- mode: cmake -*-

#
#  ATS
#    PK class
#

# ATS include directories
include_directories(${ATS_SOURCE_DIR}/src/operators/divgrad/)

add_library(pk_bases
#  pk_default_base.cc
  pk_bdf_default.cc
  pk_physical_default.cc
  pk_physical_bdf_default.cc
#  pk_physical_base.cc
  pk_explicit_default.cc
#  pk_bdf_base.cc
#  pk_default_base.cc
)

install(TARGETS pk_bases DESTINATION lib)

add_subdirectory(bc_factory)
#add_subdirectory(test_pks)
add_subdirectory(energy)
add_subdirectory(flow)
add_subdirectory(transport)
#add_subdirectory(chemistry)
add_subdirectory(deform)
add_subdirectory(surface_balance)
add_subdirectory(biogeochemistry)
add_subdirectory(mpc)
#add_subdirectory(dummy)






