cmake_minimum_required(VERSION 3.16)
project(MapEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(MapEditor
    src/main.cpp
    src/EditorApp.cpp
)

target_include_directories(MapEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MapEditor PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

