cmake_minimum_required(VERSION 3.16)
project(KungFuChess)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------
# Source file collection
# ---------------------------------------------------------------------
# Gather implementation sources / headers
file(GLOB_RECURSE ALL_CPP "src/*.cpp")
file(GLOB_RECURSE HEADERS  "src/*.hpp")

# Separate the program entry point (main.cpp) so the core can be built as
# a library that unit-tests link against without pulling in the GUI/entry
# concerns.
set(MAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
set(SOURCES ${ALL_CPP})
list(REMOVE_ITEM SOURCES ${MAIN_SRC})

# ---------------------------------------------------------------------
# Core library ‑ contains all engine code (no main())
# ---------------------------------------------------------------------
add_library(kungfu_chess_lib STATIC ${SOURCES} ${HEADERS})

# ---------------------------------------------------------------------
# Executable – small wrapper that links against the core library
# ---------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${MAIN_SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE kungfu_chess_lib)

# Set OpenCV paths
set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/OpenCV_451")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
set(OPENCV_LIB_DIR "${OPENCV_DIR}/bin")

# Include directories (apply to both lib & exe)
target_include_directories(kungfu_chess_lib PRIVATE
    ${OPENCV_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/img
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json
    ${JSON_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME}   PRIVATE
    ${OPENCV_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/img
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json)

target_link_directories(kungfu_chess_lib PRIVATE ${OPENCV_LIB_DIR})
target_link_directories(${PROJECT_NAME}   PRIVATE ${OPENCV_LIB_DIR})

# Link OpenCV libraries
target_link_libraries(kungfu_chess_lib 
    $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.lib>
    $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.lib>
)

# Executable already links against the core lib which carries OpenCV, so no extra
# linkage is needed – but you may add additional GUI libs here if required.

# Copy OpenCV DLLs to output directory
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.dll>
        $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()

# Add option to build unit tests ------------------------------------------------
option(KFC_BUILD_TESTS "Build doctest-based unit tests" ON)

# ---------------------------------------------------------------------
# Local single-header nlohmann/json stub (under src/json)
# ---------------------------------------------------------------------
set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/json)

# ---------------------------------------------------------------------
# Link libraries + includes for main executable
# ---------------------------------------------------------------------
# Already have target ${PROJECT_NAME}

target_include_directories(${PROJECT_NAME} PRIVATE ${JSON_INCLUDE_DIR})

# ---------------------------------------------------------------------
# Unit tests target adjustments
# ---------------------------------------------------------------------
if(KFC_BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    add_executable(kungfu_chess_tests ${TEST_SOURCES})
    target_include_directories(kungfu_chess_tests PRIVATE
        ${OPENCV_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${JSON_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/extern)
    target_link_libraries(kungfu_chess_tests PRIVATE
        kungfu_chess_lib )

    # Enable CTest integration so `ctest` can run the suite
    enable_testing()
    add_test(NAME kungfu_chess_tests COMMAND kungfu_chess_tests)
endif()

# Print found sources for debugging
message(STATUS "Found source files: ${SOURCES}")
message(STATUS "Found header files: ${HEADERS}") 