# The port's own code (EBA, Ouendan 2, Ouendan 1: src/games.cpp says which),
# linked into each game's ndsrun (ndskit's -DNDSKIT_EXTRA): its static
# initialisers register its hooks with the runtime. The timing offset needs
# the judge recompiled with a hook: python -m ndskit.recomp ... --hook 020681D8
# (EBA) or --hook 020BB2B4 (Ouendan 2).
#
# -DEBA_RELEASE=ON: the release build (what is not in the release yet stays
# in the code, its menus and calls off: src/eba.h); -DEBA_VERSION=x.y.z its
# version (the About box, the window's title, the executable's version
# resource). Both apply to the port's own sources only, so switching them
# rebuilds those and not the recompiled game.
set(EBA_VERSION "1.2.0" CACHE STRING "eba-recomp's version")
option(EBA_RELEASE "the release build" OFF)
set(EBA_IMGUI ${CMAKE_CURRENT_LIST_DIR}/../third_party/imgui)
set(EBA_RC ${CMAKE_CURRENT_LIST_DIR}/../third_party/rcheevos)
set(EBA_PORT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/games.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_paths.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_buttons.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_timing.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_play.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_fx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_skins.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_rumble.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_achievements.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_settings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_customs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_customs_ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/eba_ui.cpp)
set(EBA_DEFS EBA_VERSION="${EBA_VERSION}")
if(EBA_RELEASE)
    list(APPEND EBA_DEFS EBA_RELEASE=1)
    if(WIN32)
        target_link_options(ndsrun PRIVATE -mwindows)   # no console window (the launcher keeps the log)
    endif()
endif()
set_source_files_properties(${EBA_PORT_SOURCES} PROPERTIES COMPILE_DEFINITIONS "${EBA_DEFS}")
target_sources(ndsrun PRIVATE
    ${EBA_PORT_SOURCES}
    # the options bar: Dear ImGui (third_party/imgui, a submodule), SDL3 and OpenGL 3 backends
    ${EBA_IMGUI}/imgui.cpp
    ${EBA_IMGUI}/imgui_draw.cpp
    ${EBA_IMGUI}/imgui_tables.cpp
    ${EBA_IMGUI}/imgui_widgets.cpp
    ${EBA_IMGUI}/backends/imgui_impl_sdl3.cpp
    ${EBA_IMGUI}/backends/imgui_impl_opengl3.cpp)
target_include_directories(ndsrun PRIVATE ${EBA_IMGUI} ${EBA_IMGUI}/backends ${EBA_RC}/include)

# the executable's version resource (Properties > Details)
if(WIN32)
    enable_language(RC)
    string(REPLACE "." "," EBA_VERSION_COMMAS "${EBA_VERSION}")
    set(EBA_MANIFEST ${CMAKE_CURRENT_LIST_DIR}/utf8.manifest)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/version.rc.in ${CMAKE_BINARY_DIR}/eba_version.rc @ONLY)
    target_sources(ndsrun PRIVATE ${CMAKE_BINARY_DIR}/eba_version.rc)
endif()

# RetroAchievements: rcheevos (third_party/rcheevos, a submodule, C), as a library of its own;
# the network is WinHTTP's
enable_language(C)
file(GLOB EBA_RC_SOURCES ${EBA_RC}/src/*.c ${EBA_RC}/src/rapi/*.c ${EBA_RC}/src/rcheevos/*.c ${EBA_RC}/src/rhash/*.c)
list(FILTER EBA_RC_SOURCES EXCLUDE REGEX "rc_libretro\\.c$|rc_client_raintegration\\.c$")
add_library(rcheevos STATIC ${EBA_RC_SOURCES})
target_include_directories(rcheevos PUBLIC ${EBA_RC}/include)
target_compile_definitions(rcheevos PUBLIC RC_CLIENT_SUPPORTS_HASH RC_STATIC)
target_link_libraries(ndsrun rcheevos)                   # (plain: the generated CMakeLists links ndsrun so)
if(WIN32)
    target_link_libraries(ndsrun winhttp)
endif()
