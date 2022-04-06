cmake_minimum_required(VERSION 3.14.0)

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${INCLUDE_DIR})

file(GLOB_RECURSE headers ${INCLUDE_DIR} "*.h" "*.hpp")

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/Shared/src" "*.h" "*.hpp" "*.cpp" "*.cc")

add_library(RaftKvShared ${src} ${headers})

target_link_libraries(RaftKvShared sharpen::sharpen)