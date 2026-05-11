add_library(wingz STATIC
    wingz/app.cpp
    wingz/window.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/glad/src/glad.c
)

target_include_directories(wingz
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/thirdparty/glad/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/wingz
)

target_link_libraries(wingz
    PUBLIC
        glfw
        EnTT::EnTT
        imgui::imgui
        spdlog::spdlog
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
