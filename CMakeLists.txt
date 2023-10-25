cmake_minimum_required(VERSION 3.26)
project(TransportCatalog CXX)

set(CMAKE_CXX_STANDARD 17)

set(${PROJECT_NAME}_SOURCES_DIR transport-catalogue)

file(GLOB ${PROJECT_NAME}_SOURCES
        ${${PROJECT_NAME}_SOURCES_DIR}/*.cpp
        ${${PROJECT_NAME}_SOURCES_DIR}/*.h)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})
