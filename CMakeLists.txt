#=============================================================================#
#                        "CMakeLists.txt" (Top-Level):                        #
#=============================================================================#
#=============================================================================#
# CMake Settings:                                                             #
#=============================================================================#
# Export flags for convenience when using Vim's YCM plugin:
#
CMAKE_MINIMUM_REQUIRED(VERSION 3.23)
CMAKE_POLICY(          VERSION 3.23)

IF (NOT UNIX)
  MESSAGE(FATAL_ERROR "Non-UNIX platforms are not supported")
ENDIF()

#=============================================================================#
# Project Settings:                                                           #
#=============================================================================#
SET(PROJ_NAME areg-backtest)
set(CMAKE_CXX_STANDARD 26)
PROJECT($PROJ_NAME LANGUAGES CXX)

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


#=============================================================================#
# Deps:                                                                       #
#=============================================================================#
INCLUDE(FetchContent)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/utxx/include)
# IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
#   SET(CXXFLAGS "-O3 -DNDEBUG")
# ELSEIF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
#   SET(CXXFLAGS "-O0 -g")
# ELSE()
#   MESSAGE(WARNING "UnSupported BuildType: ${CMAKE_BUILD_TYPE}")
#   SET(CXXFLAGS "-O0 -g")
# ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(main main.cpp)
# TARGET_LINK_LIBRARIES(main PRIVATE )
TARGET_COMPILE_OPTIONS(main PRIVATE "-freflection" ${CXXFLAGS})

#=============================================================================#
# Tests (GoogleTest):                                                         #
#=============================================================================#
ADD_SUBDIRECTORY(test)
