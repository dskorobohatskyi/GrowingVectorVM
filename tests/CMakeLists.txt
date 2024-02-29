set(gtest_force_shared_crt on)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

add_executable(test_main ${PROJECT_SOURCE_DIR}/tests/test_main.cpp)

target_sources(test_main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VectorsAdapter.h)

target_link_libraries(
    test_main
    PRIVATE
    GTest::GTest
    GrowingVectorVM)

if (BUILD_SHARED_LIBS)
  add_custom_command(TARGET test_main POST_BUILD
      COMMAND copy "$(SolutionDir)growingvectorvm\\$(Configuration)\\GrowingVectorVM.dll" "$(TargetDir)" 
  )
  add_custom_command(TARGET test_main POST_BUILD
      COMMAND copy "$(SolutionDir)bin\\$(Configuration)\\gtest.dll" "$(TargetDir)" 
  )
endif()
    
add_test(growingvector_gtests test_main)


