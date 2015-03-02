# Locate OVR library
# This module defines
# OVR_LIBRARY, the name of the library to link against
# OVR_INCLUDE_DIR, the directory containing OVR_CAPI.h
# OVR_FOUND, if false, do not try to link to OVR
#
# By default, this looks to see if OculusSDK is in the source directory under external/
#

SET(OVR_SEARCH_PATHS
  ${CMAKE_CURRENT_SOURCE_DIR}/external/OculusSDK/LibOVR/
  ${CMAKE_CURRENT_SOURCE_DIR}/external/oculussdk/LibOVR/
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

SET (OVR_SEARCH_PREFIXES
	lib 
	lib64
)

IF (APPLE)
SET (OVR_SEARCH_PREFIXES ${OVR_SEARCH_PREFIXES}
	Lib/Mac/Release/
) 
ELSEIF (UNIX)
SET (OVR_SEARCH_PREFIXES ${OVR_SEARCH_PREFIXES}
	Lib/Linux/Release/i386/
	Lib/Linux/Release/x86_64/
) 
ENDIF (APPLE)

FIND_PATH(OVR_INCLUDE_DIR OVR_CAPI.h
  HINTS
  $ENV{OVRDIR}
  PATH_SUFFIXES include/ Include/ src/ Src/
  PATHS ${OVR_SEARCH_PATHS}
)

FIND_LIBRARY(OVR_LIBRARY_TEMP
  NAMES ovr
  HINTS
  $ENV{OVRDIR}
  PATH_SUFFIXES ${OVR_SEARCH_PREFIXES}
  PATHS ${OVR_SEARCH_PATHS}
)


IF(OVR_LIBRARY_TEMP)
  # For OS X, OVR uses Cocoa as a backend so it must link to Cocoa.
  # CMake doesn't display the -framework Cocoa string in the UI even
  # though it actually is there if I modify a pre-used variable.
  # I think it has something to do with the CACHE STRING.
  # So I use a temporary variable until the end so I can set the
  # "real" variable in one-shot.
  IF(APPLE)
    SET(OVR_LIBRARY_TEMP ${OVR_LIBRARY_TEMP} "-framework IOKit")
  ENDIF(APPLE)

  # Set the final string here so the GUI reflects the final state.
  SET(OVR_LIBRARY ${OVR_LIBRARY_TEMP} CACHE STRING "Where the OVR Library can be found")
  # Set the temp variable to INTERNAL so it is not seen in the CMake GUI
  SET(OVR_LIBRARY_TEMP "${OVR_LIBRARY_TEMP}" CACHE INTERNAL "")
ENDIF(OVR_LIBRARY_TEMP)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(OVR REQUIRED_VARS OVR_LIBRARY OVR_INCLUDE_DIR)
