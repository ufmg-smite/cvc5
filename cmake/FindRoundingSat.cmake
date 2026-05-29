###############################################################################
# Top contributors (to current version):
#   Alan Prado, Pedro Saccomani
#
# This file is part of the cvc5 project.
#
# Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
# in the top-level source directory and their institutional affiliations.
# All rights reserved.  See the file COPYING in the top-level source
# directory for licensing information.
# #############################################################################
#
# Find RoundingSat
# RoundingSat_FOUND - system has RoundingSat lib
# RoundingSat_INCLUDE_DIR - the RoundingSat include directory
# RoundingSat_LIBRARIES - Libraries needed to use RoundingSat
##

include(deps-helper)
include(ExternalProject)

# We consume a prebuilt static library + headers published as a GitLab release
# of psaccomani15/roundingsat (the `pb-solver-api` work, tag v0.1.0). The
# release ships:
#   lib/libroundingsat_lib.a   - the PB solver as a static library
#   include/                   - public API (api/PbSolver.hpp) + internal headers
# The public header surface (api/PbSolver.hpp) uses only standard-library types,
# so cvc5 does NOT need Boost: boost::multiprecision is compiled into the
# archive and the boost::iostreams CLI/parsing path is never referenced by the
# API, so static linking drops it.

#TODO: add rules to use the user's installation of RoundingSat (if there is one)
set(RoundingSat_FOUND_SYSTEM FALSE)
if(NOT ENABLE_AUTO_DOWNLOAD)
  message(FATAL_ERROR "Could not find the required dependency RoundingSat \
                      ${depname} in the system. Please use --auto-download to \
                      let us download and build it for you.")
endif()

set(RoundingSat_VERSION "1.0.0")

# The published release this build pins to. The release ships two PIC-built
# toolchain variants; we must match cvc5's C++ standard library, because the
# archive embeds std:: symbols (libc++ uses the std::__1 inline namespace,
# libstdc++ uses std::__cxx11) and mixing them is an ABI mismatch.
set(RoundingSat_TAG "v0.1.5")

# Detect the standard library in effect (honouring the user's CXXFLAGS, e.g.
# -stdlib=libc++). libc++ defines _LIBCPP_VERSION; libstdc++ does not.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
  #include <version>
  #ifndef _LIBCPP_VERSION
  #error not libc++
  #endif
  int main() { return 0; }"
  RoundingSat_STDLIB_IS_LIBCXX)

if(RoundingSat_STDLIB_IS_LIBCXX)
  set(RoundingSat_VARIANT "clang-libcxx")
  set(RoundingSat_CHECKSUM
      "77421a04443f88134554d02702a612c7ec0f6b51c689d27f9a0c8f92da455fd3")
else()
  set(RoundingSat_VARIANT "gcc-libstdcxx")
  set(RoundingSat_CHECKSUM
      "97c1cd6136c07f0a3695e99098c055ec871ce635f13e5c027964d55fe4e36a78")
endif()

set(RoundingSat_ARCHIVE "roundingsat-lib-${RoundingSat_VARIANT}-${RoundingSat_TAG}.tar.gz")
set(RoundingSat_URL
    "https://gitlab.com/api/v4/projects/82575733/packages/generic/roundingsat-lib/${RoundingSat_TAG}/${RoundingSat_ARCHIVE}")

# Destinations need to exist before the imported target references them.
file(MAKE_DIRECTORY "${DEPS_BASE}/lib")
file(MAKE_DIRECTORY "${DEPS_BASE}/include/roundingsat")

ExternalProject_Add(
  RoundingSat-EP
  ${COMMON_EP_CONFIG}
  URL "${RoundingSat_URL}"
  DOWNLOAD_NAME roundingsat-lib.tar.gz
  URL_HASH SHA256=${RoundingSat_CHECKSUM}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E copy
      <SOURCE_DIR>/lib/libroundingsat_lib.a
      ${DEPS_BASE}/lib/libroundingsat_lib.a
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory
      <SOURCE_DIR>/include ${DEPS_BASE}/include/roundingsat
  BUILD_BYPRODUCTS ${DEPS_BASE}/lib/libroundingsat_lib.a
)

set(RoundingSat_INCLUDE_DIR "${DEPS_BASE}/include/roundingsat")
set(RoundingSat_LIBRARIES "${DEPS_BASE}/lib/libroundingsat_lib.a")

set(RoundingSat_FOUND TRUE)

add_library(RoundingSat STATIC IMPORTED GLOBAL)
set_target_properties(RoundingSat PROPERTIES
  IMPORTED_LOCATION "${RoundingSat_LIBRARIES}")
set_target_properties(RoundingSat PROPERTIES
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${RoundingSat_INCLUDE_DIR}")

mark_as_advanced(RoundingSat_FOUND)
mark_as_advanced(RoundingSat_FOUND_SYSTEM)
mark_as_advanced(RoundingSat_INCLUDE_DIR)
mark_as_advanced(RoundingSat_LIBRARIES)

add_dependencies(RoundingSat RoundingSat-EP)
