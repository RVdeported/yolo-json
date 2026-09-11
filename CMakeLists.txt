#=============================================================================#
#                        "CMakeLists.txt" (Top-Level):                        #
#=============================================================================#
#=============================================================================#
# CMake Settings:                                                             #
#=============================================================================#
# Export flags for convenience when using Vim's YCM plugin:
#
SET(PROJ_NAME yolo-json)
PROJECT($PROJ_NAME LANGUAGES CXX)
CMAKE_MINIMUM_REQUIRED(VERSION 3.23)
CMAKE_POLICY(          VERSION 3.23)

IF (NOT UNIX)
  MESSAGE(FATAL_ERROR "Non-UNIX platforms are not supported")
ENDIF()

#=============================================================================#
# Project Settings:                                                           #
#=============================================================================#
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate identity string for this build:
EXECUTE_PROCESS(
  COMMAND bash -c "cd ${CMAKE_SOURCE_DIR} && git rev-parse HEAD"
  OUTPUT_VARIABLE COMMIT)
STRING(STRIP ${COMMIT} COMMIT_NAME)

EXECUTE_PROCESS(
  COMMAND bash -c "cd ${CMAKE_SOURCE_DIR} && git rev-parse --abbrev-ref HEAD"
  OUTPUT_VARIABLE BRANCH)
STRING(STRIP ${BRANCH} BRANCH_NAME)

EXECUTE_PROCESS(
  COMMAND bash -c "cd ${CMAKE_SOURCE_DIR} && git config --get remote.origin.url"
  OUTPUT_VARIABLE REMOTE)
STRING(STRIP ${REMOTE} REMOTE_NAME)

STRING(TIMESTAMP DATE "%Y-%m-%d %H-%M-%S" UTC)

MESSAGE(STATUS
  "Remote\t${REMOTE_NAME}\nBranch\t${BRANCH_NAME}\n\
Commit\t${COMMIT_NAME}\nTime\t${DATE}\n")

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${LIB_DIR}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIB_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")
SET(CXXFLAGS "-freflection" "-std=c++26" "-Wno-deprecated-enum-float-conversion")
# ==========================================================
# DISPLAY CONFIGURATIONS
# ==========================================================
message(STATUS "=======================================================")
message(STATUS "Compiler Configuration:")
message(STATUS "  Compiler ID:      ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Compiler Path:    ${CMAKE_CXX_COMPILER}")
message(STATUS "-------------------------------------------------------")

message(STATUS "Git Information:")
message(STATUS "  Branch:           ${BRANCH_NAME}")
message(STATUS "  Remote:           ${REMOTE_NAME}")
message(STATUS "  Last Commit Hash: ${COMMIT_NAME}")
message(STATUS "-------------------------------------------------------")

message(STATUS "Build Configuration:")
message(STATUS "  Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:     C++${CMAKE_CXX_STANDARD}")
message(STATUS "=======================================================")
message(STATUS "  CFLAGS:    ${CXXFLAGS}")

#=============================================================================#
# Deps:                                                                       #
#=============================================================================#
INCLUDE(FetchContent)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/utxx/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Include)

#=============================================================================#
# Tests (GoogleTest):                                                         #
#=============================================================================#
OPTION(BUILD_TESTS "Do build test?" OFF)
IF(BUILD_TESTS)
  ADD_SUBDIRECTORY(test)
ENDIF()
#=============================================================================#
# Benchmarks                                                                  #
#=============================================================================#
OPTION(BUILD_BENCH "Do build benchmark with simdjson?" OFF)
IF(BUILD_BENCH)
  ADD_SUBDIRECTORY(benchmark)
ENDIF()

#=============================================================================#
# Install:                                                                    #
#=============================================================================#
INCLUDE(GNUInstallDirs)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/Include/yolo-json/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJ_NAME})

SET(UTXX_INSTALL_HEADERS
    utxx/compiler_hints.hpp
    utxx/convert.hpp
    utxx/meta.hpp
    utxx/bits.hpp
    utxx/types.hpp
    utxx/detail/bit_count.hpp
)

FOREACH(HEADER ${UTXX_INSTALL_HEADERS})
  GET_FILENAME_COMPONENT(HEADER_DIR ${HEADER} DIRECTORY)
  INSTALL(FILES ${CMAKE_SOURCE_DIR}/utxx/include/${HEADER}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${HEADER_DIR})
ENDFOREACH()
