cmake_minimum_required(VERSION 3.14.0)

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

set(CLIENT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Client/include")

set(MASTERCLIENT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardClient/include")

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardClient/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${CLIENT_INCLUDE_DIR} ${MASTERCLIENT_INCLUDE_DIR} ${INCLUDE_DIR})

file(GLOB_RECURSE headers ${INCLUDE_DIR} "*.h" "*.hpp")

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/ShardClient" "*.h" "*.hpp" "*.cpp" "*.cc")

add_executable(RkvShardClient ${src} ${header})

target_link_libraries(RkvShardClient sharpen::sharpen RkvShared RkvClient)