cmake_minimum_required(VERSION 3.15.7)

# Опции сборки проекта
# ---------------------------------------------------------------------------- #
# TODO
# ---------------------------------------------------------------------------- #

# Опции сборки тестов
# ---------------------------------------------------------------------------- #
# option(ENABLE_TESTING "Enable testing" OFF)
# option(BUILD_TODO_TESTS "Build tests for ..." ON)
# ---------------------------------------------------------------------------- #

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_provider.cmake
)

project(wingz-engine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(EnTT REQUIRED)
find_package(imgui REQUIRED)
find_package(spdlog REQUIRED)

add_subdirectory(src)

if(ENABLE_TESTING)
    enable_testing()
    #add_subdirectory(tests)
endif()
