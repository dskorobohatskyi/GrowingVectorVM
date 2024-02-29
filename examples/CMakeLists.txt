# Example executable target
add_executable(Examples ${PROJECT_SOURCE_DIR}/examples/main.cpp)
target_link_libraries(Examples PRIVATE GrowingVectorVM) # why not PUBLIC?

# Set the output directory for the executable
set_target_properties(Examples PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/examples"
)

if (BUILD_SHARED_LIBS)
    add_custom_command(TARGET Examples PRE_BUILD
        COMMAND copy "$(SolutionDir)growingvectorvm\\$(Configuration)\\GrowingVectorVM.dll" "$(TargetDir)"
    )
endif()
