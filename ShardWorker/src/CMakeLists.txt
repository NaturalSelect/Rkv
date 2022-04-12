cmake_minimum_required(VERSION 3.14.0)

set(WORKER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardWorker/include")

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

set(SERVER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Server/include")

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SERVER_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${WORKER_INCLUDE_DIR})

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/ShardWorker" "*.h" "*.hpp" "*.cpp" "*.cc")

add_executable(RkvWorker ${src})

target_link_libraries(RkvWorker RkvServer RkvShared sharpen::sharpen)