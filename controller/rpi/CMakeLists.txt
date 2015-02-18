project(433rc)
cmake_minimum_required(VERSION 2.6)

aux_source_directory(. SOURCES)
include_directories(${PROJECT_SOURCE_DIR})
add_definitions(-std=c++0x -O3 -ggdb)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} wiringPi)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
