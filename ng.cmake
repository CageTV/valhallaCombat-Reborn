# ValhallaCombat.dll (Reborn source) on alandtse's CommonLibSSE-NG (Skyrim 1.6.1170 and newer, including 1.7.x); included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there).
find_path(VCR_SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
file(GLOB_RECURSE VCR_SOURCES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
ng_plugin(TARGET ValhallaCombatReborn NAME ValhallaCombat VERSION 1.0.1 AUTHOR "MichelKI and CageTV" ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES ${VCR_SOURCES}
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/ng" "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/src/include" "${CMAKE_CURRENT_LIST_DIR}/src/include/lib" "${VCR_SIMPLEINI_INCLUDE_DIRS}"
    DEFINES NOMINMAX WIN32_LEAN_AND_MEAN
    OPTIONS /utf-8 /wd4200 /WX-
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h")
