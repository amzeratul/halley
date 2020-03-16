# - Try to find ShaderConductor
# Once done, this will define
#
#  ShaderConductor_FOUND - system has ShaderConductor
#  ShaderConductor_INCLUDE_DIR - the ShaderConductor include directory
#  ShaderConductor_LIBRARY - link this to use ShaderConductor

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(ShaderConductor_PKGCONF ShaderConductor)

# Include dir
find_path(ShaderConductor_INCLUDE_DIR
  NAMES ShaderConductor/ShaderConductor.hpp
  PATHS ${ShaderConductor_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(ShaderConductor_LIBRARY
  NAMES ShaderConductor
  PATHS ${ShaderConductor_PKGCONF_LIBRARY_DIRS}
)

libfind_process(ShaderConductor)
