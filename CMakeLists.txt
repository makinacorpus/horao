cmake_minimum_required( VERSION 2.8 )
project( 3dstack )

set( CMAKE_DEBUG_POSTFIX "d" )
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#----------------------------------------------------------------------------
# build options
#----------------------------------------------------------------------------
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

#-----------------------------------------------------------
# dependencies
#-----------------------------------------------------------

find_package( OpenSceneGraph COMPONENTS osgQt osgViewer osgGA osgDB osgUtil osgText REQUIRED )
set(OPENSCENEGRAPH_LIBRARIES osgPPU ${OPENSCENEGRAPH_LIBRARIES})

find_package( GDAL REQUIRED )

find_package(LibXml2 REQUIRED)

#-- note that SYSTEM turns -I/path to -isystem and avoid warnings in external libs
include_directories( SYSTEM 
    ${OPENSCENEGRAPH_INCLUDE_DIRS} 
    ${LIBXML2_INCLUDE_DIR} 
)

#-- Warnings, frounding-math and gprof  ------------------------------------------
if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	# Update if necessary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wno-long-long -pedantic -Wpointer-arith -Wcast-align -Wcast-qual -Wno-overloaded-virtual -Wformat=2 -Winit-self -Wmissing-include-dirs -Wwrite-strings -Wno-error=undef")#-Wfloat-equal -Wconversion -Wshadow 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=format")
endif()

add_subdirectory( src )

