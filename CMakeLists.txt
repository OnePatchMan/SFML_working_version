# Разрешаем совместимость для старых подпроектов (FreeType < 3.5) в CMake 3.31+ / 4.0
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)

cmake_minimum_required(VERSION 3.5...3.30)
project(FightingComboArena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Поиск SFML 3 в системе или автоматическая загрузка через FetchContent для Windows / чистых систем
find_package(SFML 3 COMPONENTS Graphics Window System Audio QUIET)
if(NOT SFML_FOUND)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)

    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.0
        GIT_SHALLOW    ON
        EXCLUDE_FROM_ALL SYSTEM
    )
    FetchContent_MakeAvailable(SFML)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type" FORCE)
    endif()
endif()

add_executable(FightingComboArena
    src/main.cpp
    src/core/GameApp.cpp
    src/world/Arena.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/combat/CombatSystem.cpp
    src/audio/SoundManager.cpp
    src/ui/MenuSystem.cpp
    src/graphics/VfxSystem.cpp
)

target_include_directories(FightingComboArena PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(FightingComboArena PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

# Псевдоним для обратной совместимости команд сборки
add_custom_target(Test DEPENDS FightingComboArena)

add_executable(test_directional_combos
    scratch/test_directional_combos.cpp
    src/world/Arena.cpp
    src/world/MapEditor.cpp
    src/world/MapEditorInput.cpp
    src/world/MapEditorRender.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/combat/CombatSystem.cpp
    src/audio/SoundManager.cpp
    src/graphics/VfxSystem.cpp
)

target_include_directories(test_directional_combos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_directional_combos PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

set(MAP_EDITOR_DIR "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/MapEditor/src/main.cpp")
    set(MAP_EDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools/MapEditor")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../MapEditor/src/main.cpp")
    set(MAP_EDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../MapEditor")
endif()

if(MAP_EDITOR_DIR)
    add_executable(MapEditor
        ${MAP_EDITOR_DIR}/src/main.cpp
        ${MAP_EDITOR_DIR}/src/EditorApp.cpp
        src/world/Arena.cpp
        src/entities/Player.cpp
        src/entities/Enemy.cpp
        src/combat/CombatSystem.cpp
        src/audio/SoundManager.cpp
        src/graphics/VfxSystem.cpp
    )

    target_include_directories(MapEditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${MAP_EDITOR_DIR}/src
    )

    target_link_libraries(MapEditor PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
    )
endif()

# Копирование или симлинк ассетов в директорию сборки, чтобы пути всегда работали
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    if(WIN32)
        add_custom_command(TARGET FightingComboArena POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:FightingComboArena>/assets"
        )
        if(TARGET MapEditor)
            add_custom_command(TARGET MapEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:MapEditor>/assets"
            )
        endif()
    else()
        add_custom_command(TARGET FightingComboArena POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:FightingComboArena>/assets"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:FightingComboArena>/assets"
        )
        if(TARGET MapEditor)
            add_custom_command(TARGET MapEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:MapEditor>/assets"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:MapEditor>/assets"
            )
        endif()
    endif()
endif()

