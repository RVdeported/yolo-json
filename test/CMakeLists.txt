#=============================================================================#
# Googletest deg:                                                             #
#=============================================================================#
ENABLE_TESTING()

FIND_PACKAGE(GTest)

IF (NOT googletest_FOUND)
  FETCHCONTENT_DECLARE(
    googletest
    GIT_REPOSITORY git@github.com:google/googletest.git
    GIT_TAG        v1.15.2
  )

  FetchContent_MakeAvailable(googletest)
ENDIF()

#=============================================================================#
# Tests for Include/json_parser.hpp:                                          #
#=============================================================================#
ADD_EXECUTABLE(test_json_parser test_json_parser.cpp)

TARGET_INCLUDE_DIRECTORIES(
  test_json_parser PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/utxx/include
)

TARGET_LINK_LIBRARIES(test_json_parser PRIVATE GTest::gtest_main)

INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(test_json_parser)
