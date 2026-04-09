cmake_minimum_required(VERSION 3.20)
project(snake LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 18+: <cmath>/<cstdlib> fail to find C math functions in global namespace.
# Force-include the C runtime math/stdlib headers BEFORE any source is compiled.
# This must be set at the directory level (not target level) to affect all TUs including thirdparty.
if(MSVC)
    # Removing force includes as they break MSVC <string> and <new> STL headers
    # set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /FI\"corecrt_math.h\" /FI\"stdlib.h\"")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"corecrt_math.h\" /FI\"stdlib.h\"")
endif()

# Сборка как DLL
add_library(${PROJECT_NAME} SHARED
    # Core
    src/main.cpp
    src/hooks.cpp
    src/renderer.cpp
    src/math.cpp
    src/memory.cpp
    src/config.cpp

    # Features
    src/features/visuals.cpp
    src/features/combat.cpp
    src/features/misc.cpp

    # ImGui core
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/backends/imgui_impl_win32.cpp
    thirdparty/imgui/backends/imgui_impl_dx11.cpp

    # MinHook
    thirdparty/minhook/src/buffer.c
    thirdparty/minhook/src/hook.c
    thirdparty/minhook/src/trampoline.c
    thirdparty/minhook/src/hde/hde32.c
    thirdparty/minhook/src/hde/hde64.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends
    ${CMAKE_SOURCE_DIR}/thirdparty/minhook/include
)

# Линковка системных библиотек Windows
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    psapi
)

# Определения для Win32
# NOTE: WIN32_LEAN_AND_MEAN intentionally omitted - it breaks <cmath>/<cstdlib>
# on MSVC 18+ by stripping CRT compatibility headers from <windows.h>
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    UNICODE
    _UNICODE
)

# Суффикс .dll (по умолчанию на Windows, явно для ясности)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "snake"
    SUFFIX ".dll"
)
