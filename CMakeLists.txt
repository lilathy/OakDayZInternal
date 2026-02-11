cmake_minimum_required(VERSION 3.20)
project(DayZInternal)




add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    kernel32
    user32
    d3d11
    dxgi
)


target_compile_options(${PROJECT_NAME} PRIVATE
    /GS-              
)

target_link_options(${PROJECT_NAME} PRIVATE
    /NODEFAULTLIB
    /ENTRY:_DllMainCRTStartup
    /SAFESEH:NO
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "dayz_internal"
    SUFFIX ".dll"
)

message(STATUS "Building MINIMAL NO-CRT DLL")
