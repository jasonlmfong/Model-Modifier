set(PROJECT_NAME Model-Modifier)

################################################################################
# Source files
################################################################################
set(Source_Files
    "src/main.cpp"
    "src/surface/Surface.cpp"
    "src/surface/Surface_CatmullClark.cpp"
    "src/surface/Surface_DooSabin.cpp"
    "src/surface/Surface_GarlandHeckbert.cpp"
    "src/surface/Surface_LiuRahimzadehZordan.cpp"
    "src/surface/Surface_Loop.cpp"
    "src/surface/OrderVertices.cpp"
)

set(Header_Files
    "src/surface/Surface.h"
    "src/surface/OrderVertices.h"
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME}
    ${Source_Files}
    ${Header_Files}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    # Access OreRenderer internal headers (Window, Input, CameraController, etc.)
    "${orerenderer_SOURCE_DIR}/OreRenderer/src"
    "${orerenderer_SOURCE_DIR}/OreRenderer/src/renderer"
)

################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:_DEBUG>"
    "$<$<CONFIG:Release>:NDEBUG>"
    "_CONSOLE"
    "UNICODE"
    "_UNICODE"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
        /permissive-;
        /std:c++17;
        /sdl;
        /W3;
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:ICF
        >
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Dependencies
################################################################################
target_link_libraries(${PROJECT_NAME} PRIVATE
    OreRenderer
    OreRenderer::ImGui
    OreRenderer::STB
    glfw
)

################################################################################
# Copy resources to output
################################################################################
ore_renderer_copy_shaders(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/imgui.ini"

    VERBATIM
)
