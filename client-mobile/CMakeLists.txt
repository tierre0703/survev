cmake_minimum_required(VERSION 3.20)
project(survev_mobile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---------------------------------------------------------------
# surv_core: platform-independent port of shared/ (BitBuffer, math,
# defs registry, net messages). Compiles and tests on the host.
# ---------------------------------------------------------------
add_library(surv_core STATIC
    src/net/Net.cpp
    src/core/BitBuffer.h
    src/core/Collider.h
    src/core/GameConfig.h
    src/core/MathUtil.h
    src/core/Vec2.h
    src/net/Net.h
    src/net/ObjectSerializeFns.h
    src/net/Messages.h
    src/net/generatedDefs.inc
)
target_include_directories(surv_core PUBLIC src)
if(MSVC)
    target_compile_options(surv_core PRIVATE /W4)
else()
    target_compile_options(surv_core PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------
# surv_tests: host-run protocol compatibility tests.
# ---------------------------------------------------------------
add_executable(surv_tests
    tests/test_main.cpp
    tests/test_bitbuffer.cpp
    tests/test_messages.cpp
)
target_include_directories(surv_tests PRIVATE tests)
target_link_libraries(surv_tests PRIVATE surv_core)
if(MSVC)
    target_compile_options(surv_tests PRIVATE /W4)
else()
    target_compile_options(surv_tests PRIVATE -Wall -Wextra)
endif()

enable_testing()
add_test(NAME surv_tests COMMAND surv_tests)

# ---------------------------------------------------------------
# surv_app: axmol-dependent app (game loop, renderer, UI). Built
# only when SURV_BUILD_APP=ON (requires the axmol SDK).
# ---------------------------------------------------------------
option(SURV_BUILD_APP "Build the axmol app (requires axmol SDK)" OFF)
if(SURV_BUILD_APP)
    add_subdirectory(app)
endif()