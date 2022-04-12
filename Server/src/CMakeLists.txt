cmake_minimum_required(VERSION 3.14.0)

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(SERVER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Server/include") 

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${SERVER_INCLUDE_DIR})

file(GLOB_RECURSE headers ${SERVER_INCLUDE_DIR} "*.h" "*.hpp")

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/Server/src" "*.h" "*.hpp" "*.cpp" "*.cc")

add_library(RkvServer ${src} ${headers})

target_link_libraries(RkvServer sharpen::sharpen RkvShared)