cmake_minimum_required(VERSION 3.14.0)

set(MASTER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardMaster/include")

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

set(SERVER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Server/include")

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SERVER_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${MASTER_INCLUDE_DIR})

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/ShardMaster" "*.h" "*.hpp" "*.cpp" "*.cc")

add_executable(RkvMaster ${src})

target_link_libraries(RkvMaster RkvServer RkvShared sharpen::sharpen)