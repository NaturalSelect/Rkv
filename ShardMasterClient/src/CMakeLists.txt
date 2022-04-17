cmake_minimum_required(VERSION 3.14.0)

set(SHARPEN_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/include")

set(SHARED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Shared/include")

set(CLIENT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Client/include")

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/ShardMasterClient/include")

include_directories(${SHARPEN_INCLUDE_DIR} ${SHARED_INCLUDE_DIR} ${CLIENT_INCLUDE_DIR} ${INCLUDE_DIR})

file(GLOB_RECURSE headers ${INCLUDE_DIR} "*.h" "*.hpp")

file(GLOB_RECURSE src "${PROJECT_SOURCE_DIR}/ShardMasterClient" "*.h" "*.hpp" "*.cpp" "*.cc")

add_library(RkvMasterClient ${src} ${headers})

target_link_libraries(RkvMasterClient sharpen::sharpen RkvShared RkvClient)