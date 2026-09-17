#=============================================================================#
# Googletest deg:                                                             #
#=============================================================================#

INCLUDE(FetchContent)
ENABLE_TESTING()

FIND_PACKAGE(GTest)

IF (NOT GTest_FOUND)
  FETCHCONTENT_DECLARE(
    GTest
    GIT_REPOSITORY git@github.com:google/googletest.git
    GIT_TAG        v1.15.2
  )

  FetchContent_MakeAvailable(GTest)
ENDIF()

INCLUDE(GoogleTest)
FOREACH(TEST json_parser parser annotations serializer tokenizer schema) 
  ADD_EXECUTABLE(test_${TEST} test_${TEST}.cpp)

  TARGET_INCLUDE_DIRECTORIES(
    test_${TEST} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utxx/include
  )

  TARGET_COMPILE_OPTIONS(test_${TEST} PRIVATE ${CXXFLAGS})
  TARGET_LINK_LIBRARIES(test_${TEST} PRIVATE GTest::gtest_main)
  GTEST_DISCOVER_TESTS(test_${TEST})
ENDFOREACH()
