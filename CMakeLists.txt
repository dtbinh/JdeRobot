project (jderobot)

cmake_minimum_required(VERSION 2.8)

# build presents
if (EXISTS buildpresets_userdefined.cmake)
	include(buildpresets_userdefined.cmake)
endif()
include(buildpresets.cmake)


# include Modules 
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

# include pythonAux Macros
include(${PROJECT_SOURCE_DIR}/cmake/pythonAux.cmake)

#check compiller
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	message (version: ${CMAKE_CXX_COMPILER_VERSION})
	if(COMPILER_SUPPORTS_CXX11)
    	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
        #boost have problems with gcc < 4.7 and c++0x
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.7.0")
    		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        endif()
	else()
		    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
	endif()
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" ) # Opciones para el compilador


# ENV VARS
SET(gladedir ./)
SET( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Deps) # Directory where the dependencies are there
# Folder with ICE cpp interfaces
SET( INTERFACES_CPP_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/cpp/jderobot ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/cpp/)
# Folder with ICE python interfaces
SET( INTERFACES_PYTHON_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/python/jderobot ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/python/)
# Folder with ICE java interfaces
SET( INTERFACES_JAVA_DIR ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/java/jderobot ${CMAKE_CURRENT_BINARY_DIR}/src/interfaces/java/)
# Folder JdeRobot libraries 
SET( LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/libs) 
# Folder ICE interface definitions
SET( SLICE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces/slice)
# Main libraries
SET( LIBS_NEEDED  jderobotutil colorspaces colorspacesmm easyiceconfig parallelIce depthLib geometry logger ns progeo pioneer fuzzylib  )
SET( LIBS_EXTRA     jderobotHandlers jderobotViewer   resourcelocator xmlParser cvblob cvfast imgAnalyze JderobotInterfaces)

# Create directories if necessary
file(MAKE_DIRECTORY ${INTERFACES_PYTHON_DIR})
file(MAKE_DIRECTORY ${INTERFACES_JAVA_DIR})
file(MAKE_DIRECTORY ${INTERFACES_CPP_DIR})


# MACRO to get the list of subdirectories
macro(list_subdirectories retval curdir return_relative)
  file(GLOB sub-dir RELATIVE ${curdir} *)
  set(list_of_dirs "")
  foreach(dir ${sub-dir})
    if(IS_DIRECTORY ${curdir}/${dir})
      if (${return_relative})
        if (NOT ${dir} MATCHES "CMakeFiles" AND NOT ${dir} MATCHES ".svn")
                set(list_of_dirs ${list_of_dirs} ${dir})
        endif()
      else()
        set(list_of_dirs ${list_of_dirs} ${curdir}/${dir})
      endif()
    endif()
  endforeach()
  set(${retval} ${list_of_dirs})
endmacro()

# MACRO to get the list subdir of other path distinct to current path
macro(list_subdirectories2 retval curdir return_relative)
  file(GLOB sub-dir RELATIVE ${curdir} ${curdir}*)
  set(list_of_dirs "")
  foreach(dir ${sub-dir})
    if(IS_DIRECTORY ${curdir}/${dir})
      if (${return_relative})        
        if (NOT ${dir} MATCHES "CMakeFiles" AND NOT ${dir} MATCHES ".svn")
                set(list_of_dirs ${list_of_dirs} ${dir})
        endif()
      else()
        set(list_of_dirs ${list_of_dirs} ${curdir}/${dir})
      endif()
    endif()
  endforeach()
  set(${retval} ${list_of_dirs})
endmacro()



###################
#                 #
#  CHECK SYSTEM   #
#                 #
###################

  # FIND & CHECK PRINCIPAL LIBRARIES

include(FindPkgConfig)


link_directories(${INTERFACES_CPP_DIR}/jderobot)
FOREACH(LibFile ${LIBS_NEEDED})
    link_directories(${LIBS_DIR}/${LibFile})
    MESSAGE("${LIBS_DIR}/${LibFile}")
ENDFOREACH(LibFile)

  # FIND AND CHECK OTHER DEPENDENCES

include(Deps/LoadJderobotDependencies.cmake)

list(APPEND DEPS_DEV git cmake make gcc-5 g++-5 gcc g++ pkg-config libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev)
list(APPEND DEPS_DEV "${DEPS}")

# list(APPEND uses ; separator. We need ,
STRING(REGEX REPLACE ";" ", " DEPS "${DEPS}")
STRING(REGEX REPLACE ";" ", " DEPS_DEV "${DEPS_DEV}")



# START RECURSIVE
add_subdirectory (${CMAKE_CURRENT_SOURCE_DIR}/src)



###################
#                 #
#    INSTALL      #
#                 #
###################


# Install Executables
list_subdirectories2( LIST_DRIVERS ${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/ 1)
list_subdirectories2( LIST_TOOLS ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ 1)

# Install Deps cmake's tree
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Deps DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jderobot)







#file(WRITE dummy_main1.c "int main(int argc, char** argv){return 0;}")
#add_executable(dummy_main1 dummy_main1.c)
#INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/dummy_main1 DESTINATION /tmp COMPONENT deps OPTIONAL PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)


###################
#                 #
#    UNINSTALL    #
#                 #
###################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#
# Deb packages
#

# Determine current architecture
macro(dpkg_arch VAR_NAME)
        find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
        if (DPKG_PROGRAM) 
          execute_process(
            COMMAND ${DPKG_PROGRAM} --print-architecture
            OUTPUT_VARIABLE ${VAR_NAME}
            OUTPUT_STRIP_TRAILING_WHITESPACE
          )
        endif(DPKG_PROGRAM)
endmacro(dpkg_arch)

include (InstallRequiredSystemLibraries)
SET (CPACK_GENERATOR "DEB")
SET (CPACK_SOURCE_GENERATOR TGZ)
SET (CPACK_SET_DESTDIR ON)
SET (CPACK_DEB_COMPONENT_INSTALL OFF)
SET (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET (CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")


# CPack version numbers for release tarball name.
SET (CPACK_PACKAGE_VERSION_MAJOR 5)
SET (CPACK_PACKAGE_VERSION_MINOR 5)
SET (CPACK_PACKAGE_VERSION_PATCH 2)
SET (CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})


SET (CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
SET (CPACK_DEBIAN_PACKAGE_SECTION "net")
dpkg_arch(CPACK_DEBIAN_PACKAGE_ARCHITECTURE)

SET (CPACK_MONOLITHIC_INSTALL ON)
#set(CPACK_COMPONENTS_ALL core deps)
#set(CPACK_COMPONENT_NONAME_FOR core)
#set(CPACK_COMPONENT_CORE_GROUP "Core")
#set(CPACK_COMPONENT_DEPS_GROUP "Development")

#SET (CPACK_DEBIAN_CORE_PACKAGE_SHLIBDEPS ON)
#SET (CPACK_DEBIAN_DEPS_PACKAGE_SHLIBDEPS ON)

SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${DEPS}")
#set(CPACK_DEBIAN_CORE_PACKAGE_DEPENDS "${DEPS}")
#set(CPACK_DEBIAN_DEPS_PACKAGE_DEPENDS "${DEPS_DEV}")

if(DEFINED debug_deps)
MESSAGE("Dependencias: ${DEPS}")
MESSAGE("Dependencias_dev: ${DEPS_DEV}")
endif()

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/postinst"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake/postrm")


## Include Git HEAD into description (feature: traceback builds)
execute_process(COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HEAD
)
string(STRIP "${GIT_HEAD}" GIT_HEAD)

SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY
"Software development suite for robotics and computer vision applications.")
SET (CPACK_PACKAGE_DESCRIPTION
"JdeRobot is a software development suite for robotics and computer vision applications. 
 The applications are made up of a collection of concurrent processes (named components) that
 interoperate among them. They exchange messages using ICE communication middleware. The components may 
 run in a distributed network of computers and may be written in several languages (C++, Python, JavaScript...). 
 JdeRobot includes a collection of drivers and tools.
 Get source from https://github.com/RoboticsURJC/JdeRobot
 Package created with revision ${GIT_HEAD}")

## Patch: CPACK_PACKAGE_DESCRIPTION behavior is broken. Always use SUMMARY
SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION})

SET (CPACK_PACKAGE_CONTACT "Roberto Calvo <rocapal@gsyc.urjc.es>")
SET (CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")


## Metapackages (at cmake time)
include(scripts/metapackages/FindMetapackages.cmake)

SET (PACKAGE_VERSION ${CPACK_DEBIAN_PACKAGE_VERSION})
set(PACKAGE_DEPENDS "${DEPS}")
configure_file(${MAKE_PACKAGE_CONFIG_DIR}/jderobot-deps.info.in     ${CMAKE_BINARY_DIR}/jderobot-deps_${PACKAGE_VERSION}_all.info)

set(PACKAGE_DEPENDS "${DEPS_DEV}")
configure_file(${MAKE_PACKAGE_CONFIG_DIR}/jderobot-deps-dev.info.in ${CMAKE_BINARY_DIR}/jderobot-deps-dev_${PACKAGE_VERSION}_all.info)

execute_process(
    COMMAND ${MAKE_PACKAGE_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


## Inject release roll up (incremental releases)
# this step should be only for jderobot package
#set(PROJECT_VERSION ${CPACK_DEBIAN_PACKAGE_VERSION})
#include(scripts/incremental_releases/incremental-releases.cmake)
#set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})


#SET (CPACK_COMPONENTS_ALL Libraries ApplicationData)
include (CPack Documentation)




SET(JdeRobot_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib/jderobot )
SET(ALL_LIBS  ${LIBS_NEEDED} ${LIBS_EXTRA})
foreach (l ${ALL_LIBS} )
	if (TARGET ${l})
		SET(JdeRobot_LIBS_NAMES ${JdeRobot_LIBS_NAMES} ${l})
	endif()
endforeach()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/JdeRobotInstallConfig.cmake.in "${PROJECT_BINARY_DIR}/JdeRobotInstallConfig.cmake" @ONLY)
install (SCRIPT "${PROJECT_BINARY_DIR}/JdeRobotInstallConfig.cmake")
SET(INSTALL_CMAKE_DIR ${CMAKE_INSTALL_PREFIX}/share/jderobot)
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/JdeRobotConfig.cmake"
  "${PROJECT_BINARY_DIR}/JdeRobotConfigVersion.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)


