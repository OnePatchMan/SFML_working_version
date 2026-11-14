cmake_minimum_required(VERSION 3.16)
project(Test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Поиск SFML 3 в системе
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(Test
    src/main.cpp
    src/world/Arena.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/combat/CombatSystem.cpp
)

target_include_directories(Test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Test PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

# Копирование ассетов в директорию сборки, чтобы пути всегда работали
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET Test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Test>/assets"
    )
endif()

