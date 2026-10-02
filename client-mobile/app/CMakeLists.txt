# surv_app: axmol-dependent app target. Requires the axmol SDK and is only
# built when SURV_BUILD_APP=ON (see top-level CMakeLists.txt).
#
# IMPORTANT: This CMakeLists is meant to be consumed by the axmol build
# system, which is generated from the axmol template (see ../proj.android/
# README.md). Configure the axmol include/library paths via the
# SURV_AXMOL_DIR cache variable, or integrate this file into the generated
# axmol project.

set(SURV_AXMOL_DIR "" CACHE PATH "Path to the axmol engine checkout")

if(NOT SURV_AXMOL_DIR)
    message(FATAL_ERROR "SURV_AXMOL_DIR must point to an axmol checkout")
endif()

add_library(surv_app STATIC
    ../src/app/AppDelegate.cpp
    ../src/app/GameScene.cpp
    ../src/game/Game.cpp
    ../src/app/AppDelegate.h
    ../src/app/GameScene.h
    ../src/game/Game.h
    ../src/game/Game.h
    ../src/render/PixiLike.h
    ../src/ui/Touch.h
)

target_include_directories(surv_app PUBLIC
    ../src
    ${SURV_AXMOL_DIR}/axmol/include
)

target_link_libraries(surv_app PUBLIC surv_core)

# Wire the axmol Android app bootstrap (jni entry) — the axmol template
# provides JNI glue under proj.android/ that calls AppDelegate.
target_compile_features(surv_app PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(surv_app PRIVATE /W4)
else()
    target_compile_options(surv_app PRIVATE -Wall -Wextra)
endif()