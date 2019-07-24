add_executable(Vuse main.c)

set_target_properties(Vuse
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(Vuse PRIVATE vulkan_renderer)