set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(wingz STATIC
    wingz/app.cpp
    wingz/scene.cpp
    wingz/window.cpp
    wingz/gfx/shader.cpp
    wingz/gfx/camera.cpp
    wingz/gfx/sprite_batch.cpp
    wingz/gfx/texture.cpp
    wingz/gfx/texture_atlas.cpp
    wingz/gfx/debug_ui.cpp
    wingz/input/input_manager.cpp
    wingz/input/action_map.cpp
    wingz/physics/physics_world.cpp
    wingz/net/connection.cpp
    wingz/net/host.cpp
    wingz/net/replication.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/glad/src/glad.c
    ${IMGUI_SOURCES}
)

target_include_directories(wingz
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/thirdparty/glad/include
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui
        ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends
        ${CMAKE_SOURCE_DIR}/thirdparty/stb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/wingz
)

target_link_libraries(wingz
    PUBLIC
        glfw
        EnTT::EnTT
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        enet::enet
    PRIVATE
        ${CMAKE_DL_LIBS} 
)

# Отладочная тулза
add_executable(wingz-exe
    main.cpp
)

target_link_libraries(wingz-exe
    PRIVATE
        wingz
        glfw
)
set_target_properties(wingz-exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
