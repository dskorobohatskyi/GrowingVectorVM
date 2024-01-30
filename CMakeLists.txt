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
target_include_directories(GrowingVectorVM INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_sources(GrowingVectorVM INTERFACE ${PROJECT_SOURCE_DIR}/src/GrowingVectorVM.h)


if (BUILD_EXAMPLES)
    # Example executable target
    add_executable(Examples ${PROJECT_SOURCE_DIR}/examples/main.cpp)
    target_link_libraries(Examples PRIVATE GrowingVectorVM)

    # Set the output directory for the executable
    set_target_properties(Examples PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
    )
    if (MAKE_EXAMPLES_DEFAULT_PROJECT)
        # Set Examples as the default target
        set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Examples)
    endif ()
endif ()

# Add tests if needed
# add_executable(Tests ${PROJECT_SOURCE_DIR}/tests/test_main.cpp ${PROJECT_SOURCE_DIR}/tests/your_tests.cpp)
# target_link_libraries(Tests PRIVATE GrowingVectorVM)
# enable_testing()
# add_test(NAME YourTests COMMAND Tests)