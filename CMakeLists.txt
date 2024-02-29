cmake_minimum_required(VERSION 3.12)
project(GrowingVectorVM VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the test suite" OFF)
option(BUILD_EXAMPLES "Build the examples" ON)
# option(BUILD_STATIC_LIB "Build as a static library" ON) # TODO fix it GROWINGVECTORVM_API is set to dllimport
option(BUILD_SHARED_LIBS "Build using shared libraries" ON) # the only option now
option(MAKE_EXAMPLES_DEFAULT_PROJECT "Set Examples project as default in Visual Studio" ON)

# Library header-only target
#if (BUILD_STATIC_LIB)
#    add_library(GrowingVectorVM STATIC ${PROJECT_SOURCE_DIR}/src/GrowingVectorVM.cpp)
#    target_compile_definitions(GrowingVectorVM PRIVATE GROWINGVECTORVM_STATIC)
#endif ()
if (BUILD_SHARED_LIBS)
    add_library(GrowingVectorVM SHARED ${PROJECT_SOURCE_DIR}/src/GrowingVectorVM.cpp)
    target_compile_definitions(GrowingVectorVM PRIVATE GROWINGVECTORVM_EXPORTS)
endif ()

target_include_directories(GrowingVectorVM PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_sources(GrowingVectorVM PUBLIC ${PROJECT_SOURCE_DIR}/include/GrowingVectorVM.h)
target_sources(GrowingVectorVM PRIVATE ${PROJECT_SOURCE_DIR}/include/GrowingVectorVM.natvis)


set_target_properties(GrowingVectorVM PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/growingvectorvm"
)


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
