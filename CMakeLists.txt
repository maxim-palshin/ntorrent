cmake_minimum_required(VERSION 3.5)

project(ntorrent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)

add_executable(ntorrent main.cpp)




target_link_libraries(${PROJECT_NAME} fmt::fmt  -ltorrent-rasterbar -lboost_system -lpthread -lboost_thread)
