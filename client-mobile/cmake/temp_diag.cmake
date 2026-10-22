# TEMP DIAGNOSTIC: isolate the FairyGUI crash during the HUD build on device.
# Usage: .\tools\build-native.ps1 -Abis "x86_64" -ExtraArgs "-DTEMP_DIAG=1"
#
# Defines:
#   TEMP_NO_TREE_MASK    keep the FAE-1 layer mask off (no ClippingNode)
if (TEMP_DIAG)
    message(STATUS "TEMP_DIAG enabled")
    target_compile_definitions(${PROJECT_NAME} PRIVATE TEMP_NO_TREE_MASK)
endif()
