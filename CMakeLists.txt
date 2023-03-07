cmake_minimum_required(VERSION 3.0.0)
project(Specific_grep VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(Specific_grep main.cpp)

add_subdirectory(ThreadPool)

target_link_directories(${PROJECT_NAME} PUBLIC ThreadPool/)

target_link_libraries (Specific_grep LINK_PUBLIC ThreadPool)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
