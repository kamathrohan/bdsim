cmake_minimum_required(VERSION 3.2)
project(usercomponent)

# point cmake to my own custom installation directory that's not a system dir
# this is where BDSIMConfig.cmake exists
# if you install bdsim in a system dir then you may not need this and can comment it out
set(CMAKE_PREFIX_PATH /Users/nevay/physics/reps/bdsim-develop-install/lib/cmake/bdsim)

# find the package and set up variables
find_package(BDSIM REQUIRED)

# sources - put all compiled sources in a library
file(GLOB SOURCES "source/*.cc")
add_library(udipole SHARED ${SOURCES})
target_include_directories(udipole PRIVATE include ${BDSIM_INCLUDE_DIR})
target_link_libraries(udipole ${BDSIM_LIBRARIES})

# make a program and link to udipole source code library
add_executable(bdsimuser usercomponent.cc)
target_include_directories(bdsimuser PRIVATE include ${BDSIM_INCLUDE_DIR})
# here we don't link to gmad and bdsim again - just udipole library as that
# has the symbols we need already
target_link_libraries(bdsimuser udipole)
