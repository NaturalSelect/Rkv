cmake_minimum_required(VERSION 3.14.0)

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

set(SERVER_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Server/include")

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(CLIENT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Client/include")

set(SHARDMASTERCLIENT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardMasterClient/include")

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardWorker/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SERVER_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${CLIENT_INCLUDE_DIR} ${SHARDMASTERCLIENT_INCLUDE_DIR} ${INCLUDE_DIR})

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/ShardWorker/src" "*.h" "*.hpp" "*.cpp" "*.cc")

file(GLOB_RECURSE headers ${INCLUDE_DIR} "*.h" "*.hpp")

add_executable(RkvWorker ${src} ${headers})

target_link_libraries(RkvWorker RkvServer RkvShared sharpen::sharpen RkvClient RkvMasterClient)