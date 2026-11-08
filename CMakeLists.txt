cmake_minimum_required(VERSION 3.24)
project(itmo-gl-hw-2026 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(third_party)

add_executable(itmo-gl-hw-2026
    main.cpp
    window.cpp
    window.h
    openglwidget.cpp
    openglwidget.h
    shaders/template.fs
    shaders/template.vs
    textures/cat.jpg
)

target_link_libraries(itmo-gl-hw-2026 PRIVATE
    imgui
    SDL3::SDL3
    glm::glm
    glad
    stb
)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:itmo-gl-hw-2026>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/textures"
        "$<TARGET_FILE_DIR:itmo-gl-hw-2026>/textures"
    VERBATIM
)
add_dependencies(itmo-gl-hw-2026 copy_assets)
