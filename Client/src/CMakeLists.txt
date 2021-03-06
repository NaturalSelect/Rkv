cmake_minimum_required(VERSION 3.14.0)

if(MSVC)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS=1")
endif()

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Client/include") 

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${INCLUDE_DIR} ${SHARED_INCLUDE_DIR})

file(GLOB_RECURSE headers ${INCLUDE_DIR} "*.h" "*.hpp")

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/Client/src" "*.h" "*.hpp" "*.cpp" "*.cc")

add_library(RkvClient ${src} ${headers})

target_link_libraries(RkvClient sharpen::sharpen RkvShared)