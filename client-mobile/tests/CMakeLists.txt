# Host-only test build for the ported protocol core (src/core + src/net).
# Independent of the axmol app build (see ../CMakeLists.txt).
#
#   cmake -S tests -B build-tests -G "Visual Studio 17 2022" -A x64
#   cmake --build build-tests --config Release
#   build-tests/Release/surv_tests.exe
cmake_minimum_required(VERSION 3.20)
project(surv_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(SURV_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(surv_tests
    test_main.cpp
    test_bitbuffer.cpp
    test_messages.cpp
    test_game.cpp
    test_touch_input.cpp
    test_render.cpp
    test_audio.cpp
    test_ui.cpp
    ${SURV_SRC}/net/Net.cpp
    ${SURV_SRC}/net/WsUrl.cpp
    ${SURV_SRC}/game/Game.cpp
    ${SURV_SRC}/render/Terrain.cpp
    ${SURV_SRC}/render/Renderer.cpp
    ${SURV_SRC}/render/Defs.cpp
    ${SURV_SRC}/render/GeneratedDefs.cpp
    ${SURV_SRC}/game/Map.cpp
    ${SURV_SRC}/game/GameWorld.cpp
    ${SURV_SRC}/game/Gas.cpp
    ${SURV_SRC}/game/objects/Structure.cpp
    ${SURV_SRC}/game/objects/Barns.cpp
    ${SURV_SRC}/game/objects/PlayerRender.cpp
    ${SURV_SRC}/audio/SoundDefs.cpp
    ${SURV_SRC}/audio/AudioManager.cpp
    ${SURV_SRC}/audio/Ambiance.cpp
    ${SURV_SRC}/audio/GeneratedSoundDefs.cpp
    ${SURV_SRC}/ui/Config.cpp
    ${SURV_SRC}/ui/Device.cpp
    ${SURV_SRC}/ui/Localization.cpp
    ${SURV_SRC}/ui/TeamMenu.cpp
    ${SURV_SRC}/ui/Loadout.cpp
    ${SURV_SRC}/ui/Layout.cpp
)
target_include_directories(surv_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SURV_SRC}
)
if(MSVC)
    target_compile_options(surv_tests PRIVATE /W4)
else()
    target_compile_options(surv_tests PRIVATE -Wall -Wextra)
endif()
target_compile_definitions(surv_tests PRIVATE SURV_UI_NO_AXMOL)

enable_testing()
add_test(NAME surv_tests COMMAND surv_tests)
