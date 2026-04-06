cmake_minimum_required(VERSION 3.28)
project(tictactoe-444 CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD 17)

find_package(SDL2 REQUIRED)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/default.metallib"
    COMMAND xcrun -sdk macosx metal -c "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders.metal"
            -o "${CMAKE_CURRENT_BINARY_DIR}/shaders.air"
    COMMAND xcrun -sdk macosx metallib "${CMAKE_CURRENT_BINARY_DIR}/shaders.air"
            -o "${CMAKE_CURRENT_BINARY_DIR}/default.metallib"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders.metal"
    COMMENT "Compiling Metal shaders"
    VERBATIM
)
add_custom_target(metal_shaders ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/default.metallib")

add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/backends/imgui_impl_sdl2.cpp
    ext/imgui/backends/imgui_impl_metal.mm
)
target_include_directories(imgui PUBLIC
    ext/imgui
    ext/imgui/backends
    ext/metal-cpp
)
target_link_libraries(imgui PUBLIC
    SDL2::SDL2-static
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
)

add_executable(tictactoe-444
    src/main.cpp
    src/metal_impl.mm
    src/Renderer.mm
    src/Game.cpp
)
target_include_directories(tictactoe-444 PRIVATE
    ext/metal-cpp
    ext/glm
    src
)
target_link_libraries(tictactoe-444 PRIVATE
    imgui
    SDL2::SDL2-static
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework Cocoa"
    "-framework IOKit"
)
add_dependencies(tictactoe-444 metal_shaders)
