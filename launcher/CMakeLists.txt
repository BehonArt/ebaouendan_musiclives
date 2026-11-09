# eba-recomp's launcher (launcher.cpp): SDL3, Dear ImGui on SDL's 2D renderer,
# rcheevos for the ROMs' hashes. tools/build_launcher.sh builds it.
cmake_minimum_required(VERSION 3.20)
project(eba_launcher C CXX)
set(EBA_VERSION "1.0.1" CACHE STRING "eba-recomp's version")
set(CMAKE_CXX_STANDARD 20)
find_package(SDL3 REQUIRED)
set(IMGUI ${CMAKE_CURRENT_LIST_DIR}/../third_party/imgui)
set(RC ${CMAKE_CURRENT_LIST_DIR}/../third_party/rcheevos)
file(GLOB RC_SOURCES ${RC}/src/*.c ${RC}/src/rapi/*.c ${RC}/src/rcheevos/*.c ${RC}/src/rhash/*.c)
list(FILTER RC_SOURCES EXCLUDE REGEX "rc_libretro\\.c$|rc_client_raintegration\\.c$")
add_library(rcheevos STATIC ${RC_SOURCES})
target_include_directories(rcheevos PUBLIC ${RC}/include)
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH RC_STATIC)
add_executable(eba-recomp WIN32
    launcher.cpp rom_music.cpp stb.cpp
    ${IMGUI}/imgui.cpp ${IMGUI}/imgui_draw.cpp ${IMGUI}/imgui_tables.cpp ${IMGUI}/imgui_widgets.cpp
    ${IMGUI}/backends/imgui_impl_sdl3.cpp ${IMGUI}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(eba-recomp PRIVATE ${IMGUI} ${IMGUI}/backends ${CMAKE_CURRENT_LIST_DIR}/../third_party/stb)
target_compile_definitions(eba-recomp PRIVATE EBA_VERSION="${EBA_VERSION}")
target_link_libraries(eba-recomp PRIVATE SDL3::SDL3 rcheevos)
if(WIN32)
    enable_language(RC)
    string(REPLACE "." "," EBA_VERSION_COMMAS "${EBA_VERSION}")
    set(EBA_MANIFEST ${CMAKE_CURRENT_LIST_DIR}/../cmake/utf8.manifest)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/../cmake/version.rc.in ${CMAKE_BINARY_DIR}/eba_version.rc @ONLY)
    target_sources(eba-recomp PRIVATE ${CMAKE_BINARY_DIR}/eba_version.rc)
endif()
