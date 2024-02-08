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
target_link_libraries(
    test_main
    PRIVATE
    GTest::GTest
    GrowingVectorVM)
    
add_test(growingvector_gtests test_main)


