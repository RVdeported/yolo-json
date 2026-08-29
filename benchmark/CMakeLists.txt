
message(STATUS "=======================================================")
message(STATUS "Benchmark building: simdjson files download")

IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Src/simdjson.h)
  EXECUTE_PROCESS(
    COMMAND wget https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.h
            -O ${CMAKE_CURRENT_SOURCE_DIR}/Src/simdjson.h)
ENDIF()
IF(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Src/simdjson.cpp)
  EXECUTE_PROCESS(
    COMMAND wget https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.cpp
            -O ${CMAKE_CURRENT_SOURCE_DIR}/Src/simdjson.cpp)
ENDIF()

ADD_LIBRARY(simdjson STATIC Src/simdjson.cpp)
TARGET_INCLUDE_DIRECTORIES(
  simdjson PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

ADD_EXECUTABLE(bench Src/main.cpp)
TARGET_INCLUDE_DIRECTORIES(
  bench PRIVATE
  ${CMAKE_SOURCE_DIR}/Include
  ${CMAKE_SOURCE_DIR}/utxx/include
)

TARGET_LINK_LIBRARIES(bench PRIVATE simdjson)

TARGET_COMPILE_OPTIONS(bench PRIVATE "-freflection")
