cmake_minimum_required(VERSION 3.12)
project(GrowingVectorVM VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the test suite" OFF)
option(BUILD_EXAMPLES "Build the examples" ON)
option(MAKE_EXAMPLES_DEFAULT_PROJECT "Set Examples project as default in Visual Studio" ON)

# Library header-only target
add_library(GrowingVectorVM INTERFACE)
target_include_directories(GrowingVectorVM INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_sources(GrowingVectorVM INTERFACE ${PROJECT_SOURCE_DIR}/include/GrowingVectorVM.h)


if (BUILD_EXAMPLES)
    add_subdirectory(examples)

    if (MAKE_EXAMPLES_DEFAULT_PROJECT)
    # Set Examples as the default target
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Examples)
    endif ()
endif ()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
