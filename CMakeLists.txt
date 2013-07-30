cmake_minimum_required( VERSION 2.8 )
project( 3dstack )

set( CMAKE_DEBUG_POSTFIX "d" )

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

option( Boost_USE_AUTO_LINK "boost use autolink" OFF )
if( NOT ${Boost_USE_AUTO_LINK} )
	add_definitions( "-DBOOST_ALL_NO_LIB" )
endif()

find_package( Boost COMPONENTS
    date_time thread 
    system serialization 
    unit_test_framework
    program_options 
    chrono 
    filesystem 
    timer REQUIRED )

find_package( Qt4 COMPONENTS QtGui QtCore REQUIRED )

find_package( OpenSceneGraph COMPONENTS osgQt osgViewer osgGA osgDB osgUtil REQUIRED )

set(OPENSCENEGRAPH_LIBRARIES osgPPU ${OPENSCENEGRAPH_LIBRARIES})

#-- note that SYSTEM turns -I/path to -isystem and avoid warnings in CGAL and Boost
include_directories( SYSTEM 
    ${QT_INCLUDES} 
    ${OPENSCENEGRAPH_INCLUDE_DIRS} 
	${Boost_INCLUDE_DIRS}	
)
link_directories(
	${Boost_LIBRARY_DIRS}	
	${QT_LIBRARY_DIRS}	
	${OPENSCENEGRAPH_LIBRARY_DIRS}	
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-long-long -pedantic -Wpointer-arith -Wcast-align -Wcast-qual -Wno-overloaded-virtual -Wformat=2 -Winit-self -Wmissing-include-dirs -Wwrite-strings -Wno-error=undef")#-Wfloat-equal -Wconversion -Wshadow 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-error=format")
endif()

add_subdirectory( src )

